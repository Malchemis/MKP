#include "lib/local_search.h"
#include "lib/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void repair_solution(const Problem *prob,
                     const Solution *sol,
                     float *usage,
                     float *cur_value,
                     const float *sum_of_weights)
{
    // Count how many items are currently 1
    int items_in_solution = 0;
    for (int j = 0; j < prob->n; j++) {
        if (sol->x[j] > 0.5f) {
            items_in_solution++;
        }
    }

    // Remove up to 'items_in_solution' items if infeasible
    for (int iteration = 0; iteration < items_in_solution; iteration++) {
        // Check feasibility
        bool feasible = true;
        for (int i = 0; i < prob->m; i++) {
            if (usage[i] > prob->capacities[i]) {
                feasible = false;
                break;
            }
        }
        if (feasible) {
            break; // done
        }

        // Not feasible => remove one "worst" item by ratio
        int worst_item = -1;
        float worst_ratio = -1e9f; // ratio = c[j] / sum_of_weights[j]

        for (int j = 0; j < prob->n; j++) {
            if (sol->x[j] < 0.5f) continue; // skip items not in the solution

            // ratio = c[j] / (sum_of_weights[j] + 1e-9f)
            const float ratio = prob->c[j] / (sum_of_weights[j] + 1e-9f);

            if (ratio < worst_ratio || worst_item == -1) {
                worst_ratio = ratio;
                worst_item = j;
            }
        }

        if (worst_item == -1) {
            break; // can't repair further
        }

        // Remove this worst-ratio item
        sol->x[worst_item] = 0.0f;
        *cur_value -= prob->c[worst_item];

        for (int i = 0; i < prob->m; i++) {
            usage[i] -= prob->weights[i * prob->n + worst_item];
        }
    }
}

void local_search_flip(const Problem *prob,
                       Solution *current_sol,
                       int k,
                       LSMode mode)
{
    // We'll use prob->sum_of_weights in repair
    const float *sum_w = prob->sum_of_weights;

    // usage of the current solution
    auto current_usage = (float*)malloc(prob->m * sizeof(float));
    if (!current_usage) {
        fprintf(stderr, "Memory allocation error in local_search.\n");
        exit(EXIT_FAILURE);
    }

    // Compute initial usage
    for (int i = 0; i < prob->m; i++) {
        float weighted_sum = 0.0f;
        for (int j = 0; j < prob->n; j++) {
            weighted_sum += prob->weights[i * prob->n + j] * current_sol->x[j];
        }
        current_usage[i] = weighted_sum;
    }

    float current_value = current_sol->value;
    bool improved = true;

    // Candidate solution + usage
    Solution candidate_sol;
    allocate_solution(&candidate_sol, prob->n);

    float *candidate_usage = malloc(prob->m * sizeof(float));
    if (!candidate_usage) {
        free_solution(&candidate_sol);
        free(current_usage);
        fprintf(stderr, "Memory allocation error in local_search.\n");
        exit(EXIT_FAILURE);
    }

    while (improved) {
        improved = false;

        // Copy "current" -> "candidate"
        for (int j = 0; j < prob->n; j++) {
            candidate_sol.x[j] = current_sol->x[j];
        }
        for (int i = 0; i < prob->m; i++) {
            candidate_usage[i] = current_usage[i];
        }
        const auto candidate_value = current_value;

        int   best_item = -1;
        float best_value_increase = 0.0f;

        // Only explore top-k items from candidate_list
        const int limit = (k <= prob->n) ? k : prob->n;

        for (int idx = 0; idx < limit; idx++) {
            const int j = (int)prob->candidate_list[idx];
            // Skip items already in the solution (we only do 0 -> 1)
            if (candidate_sol.x[j] > 0.5f) {
                continue;
            }

            // Proposed flip => from 0 to 1
            const float delta_value = prob->c[j];
            const float new_value   = candidate_value + delta_value;

            // If new_value is strictly better
            if (new_value > candidate_value) {
                // First improvement => break on first better
                if (mode == LS_FIRST_IMPROVEMENT) {
                    best_item = j;
                    break;
                }
                // Best improvement => track maximum
                if (mode == LS_BEST_IMPROVEMENT) {
                    if (delta_value > best_value_increase) {
                        best_item = j;
                        best_value_increase = delta_value;
                    }
                }
            }
        }

        // If we found no improvement, stop
        if (best_item == -1) {
            break;
        }

        // Apply flip to candidate
        candidate_sol.x[best_item] = 1.0f;
        float new_candidate_value = candidate_value + prob->c[best_item];

        // Update usage
        for (int i = 0; i < prob->m; i++) {
            const float w_j = prob->weights[i * prob->n + best_item];
            candidate_usage[i] += w_j;
        }

        // Repair if infeasible
        bool infeasible = false;
        for (int i = 0; i < prob->m; i++) {
            if (candidate_usage[i] > prob->capacities[i]) {
                infeasible = true;
                break;
            }
        }
        if (infeasible) {
            repair_solution(prob, &candidate_sol, candidate_usage, &new_candidate_value, sum_w);
        }

        // Revert if not strictly better
        if (new_candidate_value <= candidate_value) {
            // do nothing => revert
        } else {
            // Accept
            improved = true;
            candidate_sol.value = new_candidate_value;

            // Check final feasibility after repair
            bool feasible_sol = true;
            for (int i = 0; i < prob->m; i++) {
                if (candidate_usage[i] > prob->capacities[i]) {
                    feasible_sol = false;
                    break;
                }
            }
            candidate_sol.feasible = feasible_sol;

            // Swap solutions & usage
            swap_solutions(current_sol, &candidate_sol);
            float *tmp_usage = current_usage;
            current_usage = candidate_usage;
            candidate_usage = tmp_usage;

            // Update local
            current_value = current_sol->value;
        }
    }

    // Final feasibility check
    bool feasible = true;
    for (int i = 0; i < prob->m; i++) {
        if (current_usage[i] > prob->capacities[i]) {
            feasible = false;
            break;
        }
    }
    current_sol->feasible = feasible;

    // Cleanup
    free(candidate_usage);
    free_solution(&candidate_sol);
    free(current_usage);
}
