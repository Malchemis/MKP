#include <gradesc.h>
#include <local_search.h>       // for local_search_flip and repair
#include <vnd.h>                // for vnd
#include <data_structure.h>
#include <utils.h>              // for evaluate_solution_cpu, etc.
#include <math.h>               // for expf
#include <stdlib.h>             // for malloc, free, rand
#include <stdio.h>              // for fprintf

#define CLAMP_VALUE 1.0f

/**
 * @brief Simple sigmoid function with clamping to avoid overflow.
 *
 * @param z The input value (can be negative or positive).
 * @return Sigmoid(z) = 1 / (1 + exp(-z)) clamped to avoid large exponents.
 */
static float sigmoid(float z) {
    if (z > CLAMP_VALUE)  z = CLAMP_VALUE;
    if (z < -CLAMP_VALUE) z = -CLAMP_VALUE;
    return 1.0f / (1.0f + expf(-z));
}

/**
 * @brief Compute the penalized loss (to minimize) for a continuous solution x_hat.
 *
 *  loss(x_hat) = - \sum_i c[i] x_hat[i]
 *                + 0.5 * lambda * \sum_j (usage[j] - capacity[j])^2
 *
 * Where usage[j] = \sum_i weights[j*n + i] * x_hat[i].
 *
 * @param prob      The MKP instance
 * @param lambda    The penalty coefficient
 * @param x_hat     The current continuous solution, length n
 * @param usage     The resource usage for each constraint, length m
 * @return The scalar loss value.
 */
float compute_loss(const Problem *prob, const float lambda, const float *x_hat, const float *usage) {
    float loss = 0.0f;

    // Negative profit part
    for (int i = 0; i < prob->n; i++) {
        loss -= prob->c[i] * x_hat[i];
    }

    // Penalty part
    for (int j = 0; j < prob->m; j++) {
        const float diff = usage[j] - prob->capacities[j];
        loss += 0.5f * lambda * diff * diff;
    }
    return loss;
}


void gradient_solver(const Problem *prob, const float lambda, const float learning_rate,
                     const int max_iters, Solution *out_sol){
    const int n = prob->n;
    const int m = prob->m;

    // Hyperparameters for momentum
    // Optional freezing parameters
    const auto frozen    = (bool*) calloc(n, sizeof(bool)); // track which items are frozen
    const auto freeze_ct = (int*) calloc(n, sizeof(int));   // how many consecutive times x_hat is near 0 or 1

    // 1) Allocate theta, velocity, etc.
    const auto theta   = (float*)malloc(n * sizeof(float));
    const auto v       = (float*)calloc(n, sizeof(float));  // velocity for momentum
    const auto x_hat   = (float*)malloc(n * sizeof(float));
    const auto usage   = (float*)malloc(m * sizeof(float));
    const auto grad    = (float*)malloc(n * sizeof(float));
    // Check for allocation errors
    if (!theta || !x_hat || !usage || !grad || !v || !frozen || !freeze_ct) {
        fprintf(stderr, "Allocation error in gradient_solver.\n");
        free(theta); free(x_hat); free(usage); free(grad); free(v);
        free(frozen); free(freeze_ct);
        exit(EXIT_FAILURE);
    }

    // 2) Randomly initialize theta
    for (int i = 0; i < n; i++) {
        theta[i] = 0.01f * (float)rand() / (float)RAND_MAX;
    }

    // 3) Main gradient loop
    for (int iter = 0; iter < max_iters; iter++) {
        // (a) Compute x_hat[i] = sigmoid(theta[i]) for non-frozen items
        //     If frozen, keep x_hat in {0, 1} based on the sign of theta[i].
        for (int i = 0; i < n; i++) {
            if (frozen[i]) {
                // If frozen, interpret sign of theta as final 0 or 1
                x_hat[i] = (theta[i] > 0.0f) ? 1.0f : 0.0f;
            } else {
                x_hat[i] = sigmoid(theta[i]);
            }
        }

        // (b) Compute usage
        for (int j = 0; j < m; j++) {
            float sum_w = 0.0f;
            for (int i = 0; i < n; i++) {
                sum_w += prob->weights[j * n + i] * x_hat[i];
            }
            usage[j] = sum_w;
        }

        // (c) Compute gradient
        for (int i = 0; i < n; i++) {
            if (frozen[i]) {
                grad[i] = 0.0f; // do not update frozen items
                continue;
            }
            const float s  = x_hat[i];
            const float ds = s * (1.0f - s);  // derivative of sigmoid
            float g  = -prob->c[i] * ds;  // from objective

            // Add penalty part
            for (int j = 0; j < m; j++) {
                const float w_ij = prob->weights[j * n + i];
                g += lambda * (usage[j] - prob->capacities[j]) * w_ij * ds;
            }
            grad[i] = g;
        }

        // (d) Update each theta[i] with momentum
        for (int i = 0; i < n; i++) {
            if (!frozen[i]) {
                constexpr float momentum = 0.9f;
                // Velocity update
                v[i] = momentum * v[i] + (1.0f - momentum) * grad[i];
                // Gradient descent step
                theta[i] -= learning_rate * v[i];
            }
        }

        // (e) Optionally freeze items if x_hat[i] is consistently near 0 or 1
        for (int i = 0; i < n; i++) {
            if (!frozen[i]){
                constexpr float freeze_threshold = 0.9f;
                constexpr int freeze_limit = 10;
                if (x_hat[i] > freeze_threshold) {
                    freeze_ct[i]++;
                    if (freeze_ct[i] >= freeze_limit) {
                        frozen[i] = true;   // freeze "in"
                        theta[i]  = +1.0f;  // store sign
                    }
                } else if (x_hat[i] < (1.0f - freeze_threshold)) {
                    freeze_ct[i]++;
                    if (freeze_ct[i] >= freeze_limit) {
                        frozen[i] = true;   // freeze "out"
                        theta[i]  = -1.0f;  // store sign
                    }
                } else {
                    // reset if not near boundary
                    freeze_ct[i] = 0;
                }
            }
        }

        // Print info every 2 iterations
        if (iter % 2 == 0) {
            const float L = compute_loss(prob, lambda, x_hat, usage);
            float approx_obj = 0.0f;
            for (int i = 0; i < n; i++) {
                approx_obj += prob->c[i] * x_hat[i];
            }

            printf("Iter %4d: Loss=%.2f, approx_obj=%.2f, frozen_items=", iter, L, approx_obj);
            // Count how many items are frozen
            int count_frozen = 0;
            for (int k = 0; k < n; k++) {
                if (frozen[k]) count_frozen++;
            }
            printf("%d\n", count_frozen);
        }
    }

    // 4) Now convert final x_hat to a 0-1 solution in out_sol
    for (int i = 0; i < n; i++) {
        constexpr float cutoff = 0.5f;
        const float val = sigmoid(theta[i]);
        out_sol->x[i] = (val >= cutoff) ? 1.0f : 0.0f;
    }

    // Recompute usage from the final integer solution
    compute_usage_from_solution(prob, out_sol, usage);

    // Evaluate objective and feasibility
    evaluate_solution_cpu(prob, out_sol);
    out_sol->feasible = check_feasibility(prob, out_sol);

    printf("\n--- After Gradient Descent Rounding ---\n");
    printf("Value: %.2f\n", out_sol->value);
    printf("Feasible: %s\n\n", out_sol->feasible ? "Yes" : "No");

    // 5) Repair if infeasible
    if (!out_sol->feasible) {
        repair_solution(prob, out_sol, usage, &out_sol->value);
        // Re-evaluate after repair
        evaluate_solution_cpu(prob, out_sol);
        out_sol->feasible = check_feasibility(prob, out_sol);

        printf("--- After Repair ---\n");
        printf("Value: %.2f\n", out_sol->value);
        printf("Feasible: %s\n\n", out_sol->feasible ? "Yes" : "No");
    }

    // 6) [Optional] Local search // VND // VNS
    local_search_flip(prob, out_sol, prob->n, LS_BEST_IMPROVEMENT);
    //vnd(prob, out_sol, 1000, prob->n, prob->n, LS_BEST_IMPROVEMENT);
    evaluate_solution_cpu(prob, out_sol);

    // Final print
    printf("--- Final Gradient Solution ---\n");
    printf("Value: %.2f\n", out_sol->value);
    printf("Feasible: %s\n", out_sol->feasible ? "Yes" : "No");

    // Cleanup
    free(theta);
    free(x_hat);
    free(usage);
    free(grad);
    free(v);
    free(frozen);
    free(freeze_ct);
}
