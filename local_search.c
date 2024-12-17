//
// Local search solver for MKP.
// Uses LS-Flip or Exchange neighborhood.
//
#include "lib/local_search.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/**
 * @brief Perform an LS-FLIP local search with best-improvement strategy.
 *
 * We attempt to flip each bit:
 * - Compute the potential objective change.
 * - Compute the delta of constraints usage but do not immediately apply to current_usage.
 * - Check feasibility using current_usage + delta.
 * If a better feasible solution is found, remember it as the best candidate.
 * After checking all neighbors, if a best-improvement was found, apply it and repeat.
 * Stop when no improvement is found.
 */
void local_search(const Problem *prob, Solution *current_sol)
{
    // Allocate current usage
    const auto current_usage = (float*)malloc(prob->m * sizeof(float));
    if (!current_usage) {
        fprintf(stderr, "Memory allocation error in local_search.\n");
        exit(EXIT_FAILURE);
    }

    // Compute initial usage
    for (int i = 0; i < prob->m; i++) {
        float sum_w = 0.0f;
        const float *row = &prob->weights[i * prob->n];
        for (int j = 0; j < prob->n; j++) {
            sum_w += row[j] * current_sol->x[j];
        }
        current_usage[i] = sum_w;
    }

    float current_value = current_sol->value;
    bool improved = true;

    // Temporary array to store the best move's delta usage
    const auto best_delta_usage = (float*)malloc(prob->m * sizeof(float));
    if (!best_delta_usage) {
        fprintf(stderr, "Memory allocation error in local_search.\n");
        free(current_usage);
        exit(EXIT_FAILURE);
    }

    while (improved) {
        improved = false;
        int best_item = -1;
        float best_value_change = 0.0f;
        bool best_flip_to_one = false;

        // Check all neighbors (one-bit flips)
        for (int j = 0; j < prob->n; j++) {
            const float old_xj = current_sol->x[j];
            const float new_xj = (old_xj > 0.5f) ? 0.0f : 1.0f;

            // Compute the value change
            const float value_change = (new_xj > old_xj) ? prob->c[j] : -prob->c[j];
            const float new_value = current_value + value_change;

            // Only consider neighbors that improve the objective
            if (new_value <= current_value)
                continue;

            // Compute delta_usage for this flip
            // delta_usage[i] = how much current_usage[i] would change if we apply this flip
            float *delta_usage = alloca(prob->m * sizeof(float));
            for (int i = 0; i < prob->m; i++) {
                float w_ij = prob->weights[i*prob->n + j];
                delta_usage[i] = (new_xj > old_xj) ? w_ij : -w_ij;
            }

            // Check feasibility using current_usage + delta_usage
            bool feasible = true;
            for (int i = 0; i < prob->m; i++) {
                if (current_usage[i] + delta_usage[i] > prob->capacities[i]) {
                    feasible = false;
                    break;
                }
            }
            if (!feasible) {// Not a feasible move, skip
                continue;
            }

            // Feasible and better objective
            const float improvement = new_value - current_value;
            if (improvement > best_value_change) {
                // Update best improvement
                best_value_change = improvement;
                best_item = j;
                best_flip_to_one = (new_xj > old_xj);
                // Store this best delta_usage
                for (int i = 0; i < prob->m; i++) {
                    best_delta_usage[i] = delta_usage[i];
                }
            }
        } // end for each item

        // If we found a best-improvement, apply it
        if (best_item != -1) {
            // Apply best_delta_usage to current_usage
            for (int i = 0; i < prob->m; i++) {
                current_usage[i] += best_delta_usage[i];
            }

            // Update solution vector, value, feasibility
            current_sol->x[best_item] = best_flip_to_one ? 1.0f : 0.0f;
            current_value += best_value_change;
            current_sol->value = current_value;
            current_sol->feasible = true; // verified above

            improved = true; // We improved, so we attempt another iteration
        }
    }

    free(best_delta_usage);
    free(current_usage);
}
