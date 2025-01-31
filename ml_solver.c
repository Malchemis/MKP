#include "ml_solver.h"
#include "local_search.h"       // for local_search_flip and repair
#include "lib/data_structure.h"
#include "lib/utils.h"          // for evaluate_solution_cpu, etc.
#include <math.h>               // for expf
#include <stdlib.h>             // for malloc, free, rand
#include <stdio.h>              // for fprintf
// #include <vnd.h>
// #include <vns.h>

#define CLAMP_VALUE 20.0f

/**
 * @brief Simple sigmoid function with clamping to avoid overflow.
 *
 * @param z The input value
*/
static float sigmoid(float z) {
    z = fmaxf(fminf(z, CLAMP_VALUE), -CLAMP_VALUE);
    return 1.0f / (1.0f + expf(-z));
}

/**
 * @brief Compute the loss of the current solution.
 *
 * @param prob The MKP instance
 * @param lambda The penalty coefficient
 * @param x_hat The current solution (0-1) that went through the sigmoid
 * @param usage The current usage of each constraint
 * @return The loss value
 */
float compute_loss(const Problem *prob, const float lambda, const float *x_hat, const float *usage) {
    float loss = 0.0f;

    // negative profit part. We solve as a minimization problem
    for (int i = 0; i < prob->n; i++) {
        loss -= prob->c[i] * x_hat[i];
    }

    // Squared penalty part introduced in the loss
    for (int j = 0; j < prob->m; j++) {
        // Lambda / 2 is to not have to care about the squared term that becomes 2*lambda
        loss += 0.5f * lambda * (usage[j] - prob->capacities[j]) * (usage[j] - prob->capacities[j]);
    }
    return loss;
}


void gradient_solver(const Problem *prob, const float lambda, const float learning_rate, const int max_iters, Solution *out_sol) {
    const int n = prob->n; // number of items
    const int m = prob->m; // number of constraints

    // 1) Allocate parameter array theta. Thetas are the parameters to learn.
    const auto theta = (float*)malloc(n * sizeof(float));
    if (!theta) {
        fprintf(stderr, "Failed to allocate theta.\n");
        exit(EXIT_FAILURE);
    }

    // 2) Randomly initialize theta
    srand(42); // We seed the random number generator for reproducibility
    for (int i = 0; i < n; i++) {
        // We start with small values
        theta[i] = 0.01f * ((float)rand() / (float)RAND_MAX);
    }

    // Temporary arrays for the gradient steps
    const auto x_hat = (float*)malloc(n * sizeof(float));  // stores sigmoid(theta[i])
    const auto usage = (float*)malloc(m * sizeof(float));  // stores usage for each constraint
    const auto grad  = (float*)malloc(n * sizeof(float));  // stores gradient for each theta[i]

    if (!x_hat || !usage || !grad) {
        fprintf(stderr, "Allocation error in gradient_solver.\n");
        free(theta);
        if (x_hat) free(x_hat);
        if (usage) free(usage);
        if (grad)  free(grad);
        exit(EXIT_FAILURE);
    }

    // 3) Main gradient descent loop
    for (int iter = 0; iter < max_iters; iter++) {
        // (a) Compute x_hat[i] = sigmoid(theta[i])
        for (int i = 0; i < n; i++) {
            x_hat[i] = sigmoid(theta[i]); // sigmoid(theta[i]) = 1 / (1 + exp(-theta[i]))
        }

        // (b) Compute usage[j] = sum_i ( W[j*n + i] * x_hat[i] )
        for (int j = 0; j < m; j++) {
            float sum_over_i = 0.0f;
            for (int i = 0; i < n; i++) {
                sum_over_i += prob->weights[j * n + i] * x_hat[i];
            }
            usage[j] = sum_over_i;
        }

        // (c) Compute gradient for each theta[i]
        for (int i = 0; i < n; i++) {
            const float s   = x_hat[i];          // sigmoid(theta[i])
            const float ds  = s * (1.0f - s);    // derivative of sigmoid = s * (1 - s)
            float gradient  = -prob->c[i] * ds;  // gradient from objective part (negative profit)

            // Add penalty part
            for (int j = 0; j < m; j++) {
                // derivative = lambda * e * W[j*n + i] * ds
                const float w_ij = prob->weights[j * prob->n + i];
                gradient += lambda * (usage[j] - prob->capacities[j]) * w_ij * ds;
            }
            grad[i] = gradient;
        }

        // (d) Update each theta[i]
        for (int i = 0; i < n; i++) {
            theta[i] -= learning_rate * grad[i];
        }

        // [Optional] every N iterations or every iteration:
        if (iter % 100 == 0) { // say we print every 100 steps
            const float L = compute_loss(prob, lambda, x_hat, usage);

            // Compute approximate objective value
            float approx_obj = 0.0f;
            for (int i = 0; i < prob->n; i++) {
                approx_obj += prob->c[i] * x_hat[i];
            }

            printf("Iter %4d: Loss = %.4f, approx obj = %.4f\n", iter, L, approx_obj);
            fflush(stdout); // flush so we see the output immediately
        }
    }

    evaluate_solution_cpu(prob, out_sol);

    // 4) Build a 0-1 solution from final theta
    for (int i = 0; i < n; i++) {
        const float val = sigmoid(theta[i]);
        out_sol->x[i] = (val >= 0.5f) ? 1.0f : 0.0f;
    }

    // 5) Evaluate the solution
    evaluate_solution_cpu(prob, out_sol);

    // 6) If it's infeasible, repair // local-search // VND ? VNS ?
    if (!out_sol->feasible) {
        repair_solution(prob, out_sol, usage, &out_sol->value);
    }

    local_search_flip(prob, out_sol, prob->n, LS_BEST_IMPROVEMENT);
    // vnd(prob, out_sol, 1000, prob->n, prob->n, LS_BEST_IMPROVEMENT);

    if (!out_sol->feasible) {
        repair_solution(prob, out_sol, usage, &out_sol->value);
    }

    // Cleanup
    free(theta);
    free(x_hat);
    free(usage);
    free(grad);
}
