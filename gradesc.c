#include <gradesc.h>
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
 *                + 0.5 * lambda * \sum_j max(0, (usage[j] - capacity[j]))
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
        if (diff > 0.0f) loss += 0.5f * lambda * diff;
    }
    return loss;
}


/**
 * @brief Compute usage[j] = \sum_i weights[j*n + i] * x_hat[i]
 */
static void compute_usage(const Problem *prob, const float *x_hat, float *usage) {
    for (int j = 0; j < prob->m; j++) {
        float sum_w = 0.0f;
        for (int i = 0; i < prob->n; i++) {
            sum_w += prob->weights[j * prob->n + i] * x_hat[i];
        }
        usage[j] = sum_w;
    }
}

static void freeze_highest_thetas(const Problem *prob, float *theta, bool *frozen) {
    int best_idx = -1;
    float best_val = -1e9f;

    for (int i = 0; i < prob->n; i++) {
        if (!frozen[i] && theta[i] > best_val) {
            best_val = theta[i];
            best_idx = i;
        }
    }
    if (best_idx != -1) {
        frozen[best_idx] = true;
        theta[best_idx]  = 1.0f;  // force "in" (sigma(100) ~ 1.0)
    }
}


void gradient_solver(const Problem *prob,
                    const float lambda,
                    const float learning_rate,
                    const int max_no_improvement,
                    Solution *out_sol,
                    const LogLevel verbose,
                    const clock_t start,
                    const float max_time) {

    const int n = prob->n;
    const int m = prob->m;

    // 1) Allocate theta, velocity, etc.
    const auto theta   = (float*)malloc(n * sizeof(float));
    const auto v       = (float*)calloc(n, sizeof(float));  // velocity for momentum
    const auto x_hat   = (float*)malloc(n * sizeof(float));
    const auto usage   = (float*)malloc(m * sizeof(float));
    const auto grad    = (float*)malloc(n * sizeof(float));
    const auto frozen  = (bool*)calloc(n, sizeof(bool)); // we freeze iteratively the highest theta
    // Check for allocation errors
    if (!theta || !x_hat || !usage || !grad || !v || !frozen) {
        fprintf(stderr, "Allocation error in gradient_solver.\n");
        free(theta); free(x_hat); free(usage); free(grad); free(v);
        exit(EXIT_FAILURE);
    }

    // Randomly initialize theta
    for (int i = 0; i < n; i++) {
        theta[i] = (float)rand() / (float)RAND_MAX;
    }

    int no_improvement = 0;
    int iter = 0;
    float previous_loss = 1e9f;

    // Main loop
    while (no_improvement < max_no_improvement && !time_is_up(start, max_time)) {
        constexpr int n_warmup_iters = 10;
        // Compute x_hat
        for (int i = 0; i < n; i++) {
            if (frozen[i]) {// if frozen, interpret sign to fix it in or out
                x_hat[i] = (theta[i] > 0.0f) ? 1.0f : 0.0f;
            } else {
                x_hat[i] = sigmoid(theta[i]);
            }
        }

        // Compute usage
        compute_usage(prob, x_hat, usage);

        // Compute gradient
        for (int i = 0; i < n; i++) {
            if (frozen[i]) {
                grad[i] = 0.0f;
            } else {
                const float s  = x_hat[i];
                const float ds = s * (1.0f - s);
                float g  = -prob->c[i] * ds;  // objective part

                // penalty part
                for (int j = 0; j < m; j++) {
                    const float diff = usage[j] - prob->capacities[j];
                    const float w_ij = prob->weights[j * n + i];
                    if (diff > 0.0) g += lambda * w_ij * ds;
                }
                grad[i] = g;
            }
        }

        // Momentum update
        for (int i = 0; i < n; i++) {
            if (!frozen[i]) {
                constexpr float momentum = 0.95f;
                v[i] = momentum * v[i] + (1.0f - momentum) * grad[i];
                theta[i] -= learning_rate * v[i];
            }
        }

        // Freeze the highest theta after a few iterations
        if (iter > n_warmup_iters) freeze_highest_thetas(prob, theta, frozen);

        // Compute loss
        const float L = compute_loss(prob, lambda, x_hat, usage);
        if (L >= previous_loss) {
            no_improvement++;
        } else {
            no_improvement = 0;
        }
        previous_loss = L;

        // Print every few iterations
        if (verbose == DEBUG && (iter % 100 == 0)) {
            // approximate objective
            float approx_obj = 0.0f;
            for (int i = 0; i < n; i++) {
                approx_obj += prob->c[i] * x_hat[i];
            }
            // count how many items are frozen
            int count_frozen = 0;
            for (int i = 0; i < n; i++) {
                if (frozen[i]) count_frozen++;
            }
            printf("Iter %3d: Loss=%.2f, approx_obj=%.2f, frozen=%d\n",
                   iter, L, approx_obj, count_frozen);
        }
        iter++;
    }

    // Now convert final x_hat to a 0-1 solution in out_sol
    for (int i = 0; i < n; i++) {
        constexpr float cutoff = 0.5f;
        const float val = sigmoid(theta[i]);
        out_sol->x[i] = (val >= cutoff) ? 1.0f : 0.0f;
    }

    // Recompute usage from the final integer solution
    compute_usage(prob, out_sol->x, usage);

    // Evaluate objective and feasibility
    evaluate_solution_cpu(prob, out_sol);
    out_sol->feasible = check_feasibility(prob, out_sol);
    if (verbose == DEBUG) {
        printf("\n--- After Gradient Descent ---\n");
        printf("Value: %.2f\n", out_sol->value);
        printf("Feasible: %s\n", out_sol->feasible ? "Yes" : "No");
    }

    // Repair if infeasible
    if (!out_sol->feasible) {
        repair_solution(prob, out_sol, usage, &out_sol->value);
        // Re-evaluate after repair
        evaluate_solution_cpu(prob, out_sol);
        out_sol->feasible = check_feasibility(prob, out_sol);
        if (verbose == DEBUG) {
            printf("--- After Repair ---\n");
            printf("Value: %.2f\n", out_sol->value);
            printf("Feasible: %s\n", out_sol->feasible ? "Yes" : "No");
        }
    }

    // Cleanup
    free(theta);
    free(x_hat);
    free(usage);
    free(grad);
    free(v);
    free(frozen);
}
