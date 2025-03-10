#include <local_search.h>
#include <utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void repair_solution(const Problem *prob, Solution *sol, float *usage, float *cur_value) {
    // We can remove up to n items
    for (int iteration = 0; iteration < prob->n; iteration++) {
        // Check feasibility
        bool feasible = true;
        for (int i = 0; i < prob->m; i++) {
            if (usage[i] > prob->capacities[i]) {
                feasible = false;
                break;
            }
        }
        if (feasible) {
            sol->feasible = true;
            break;
        }

        // Not feasible => remove one "worst" item by ratio
        int worst_item = -1;
        float worst_ratio = -1e9f; // ratio = c[j] / sum_of_weights[j]

        for (int j = 0; j < prob->n; j++) {
            if (sol->x[j] < 0.5f) continue; // skip items not in the solution

            // ratio = c[j] / (sum_of_weights[j] + 1e-9f)
            const float ratio = prob->ratios[j];

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

void local_search_flip(const Problem *prob, Solution *current_sol, const int max_checks, const LSMode mode) {
    // Usage of the current solution
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

    // Only explore top-max_checks items from candidate_list
    const int limit = (max_checks <= prob->n) ? max_checks : prob->n;
    while (improved) {
        improved = false;

        // Copy "current" -> "candidate"
        copy_solution(current_sol, &candidate_sol);
        memcpy(candidate_usage, current_usage, prob->m * sizeof(float));
        const auto candidate_value = current_value;

        int   best_item = -1;
        float best_value_increase = 0.0f;
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

        // Update usage : add weights of the new item. Improvement means it cannot be an already used item, or an unused.
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
            repair_solution(prob, &candidate_sol, candidate_usage, &new_candidate_value);
        }

        // Revert if not strictly better
        if (new_candidate_value <= candidate_value) {
            // do nothing (don't apply candidate to current) => revert
        } else {
            // Accept => update current solution
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

void local_search_swap(const Problem *prob, Solution *current_sol, const int max_checks, const LSMode mode) {
    // Compute the usage of the current solution
    auto current_usage = (float*)malloc(prob->m * sizeof(float));
    if (!current_usage) {
        fprintf(stderr, "Memory allocation error in local_search_swap.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < prob->m; i++) {
        float sum_w = 0.0f;
        for (int j = 0; j < prob->n; j++) {
            sum_w += prob->weights[i * prob->n + j] * current_sol->x[j];
        }
        current_usage[i] = sum_w;
    }

    float current_value = current_sol->value;
    bool improved = true;

    // Allocate candidate solution and usage
    Solution candidate_sol;
    allocate_solution(&candidate_sol, prob->n);

    auto candidate_usage = (float*)malloc(prob->m * sizeof(float));
    if (!candidate_usage) {
        free_solution(&candidate_sol);
        free(current_usage);
        fprintf(stderr, "Memory allocation error in local_search_swap.\n");
        exit(EXIT_FAILURE);
    }

    // We only explore top-max_checks items from candidate_list
    const int limit = (max_checks <= prob->n) ? max_checks : prob->n;

    // Main local search loop
    while (improved) {
        improved = false;

        // Copy current -> candidate
        copy_solution(current_sol, &candidate_sol);
        memcpy(candidate_usage, current_usage, prob->m * sizeof(float));
        const float candidate_value = current_value;

        int best_i = -1; // item to remove
        int best_j = -1; // item to add
        float best_delta = 0.0f;

        // Explore swaps: i in solution, j not in solution (from candidate_list)
        for (int i = 0; i < prob->n; i++) {
            if (candidate_sol.x[i] < 0.5f) {
                continue; // skip items not in the solution
            }
            const float ci = prob->c[i]; // value of the item in solution

            // Iterate over the top-limit items in candidate_list as "j"
            bool break_outer_loop = false; // boolean to break when a first improvement is found
            for (int idx = 0; idx < limit; idx++) {
                const int j = (int) prob->candidate_list[idx];
                if (candidate_sol.x[j] > 0.5f) {
                    // j is already in the solution, skip
                    continue;
                }

                const float cj = prob->c[j];  // item value not in solution
                const float delta = cj - ci;  // how much we gain by removing i and adding j

                // We only consider strictly positive deltas (for a direct improvement)
                if (delta <= 0.0f) {
                    continue; // Find next j
                }

                // Improvement found
                const float new_value = candidate_value + delta;
                if (new_value > candidate_value) {
                    // We found a potential improvement
                    if (mode == LS_FIRST_IMPROVEMENT) {
                        // Record and break immediately
                        best_i = i;
                        best_j = j;
                        best_delta = delta;
                        break_outer_loop = true;
                        break;
                    }
                    if (mode == LS_BEST_IMPROVEMENT) {
                        // Track best improvement
                        if (delta > best_delta) {
                            best_i = i;
                            best_j = j;
                            best_delta = delta;
                        }
                    }
                }
            } // end of inner loop (comparing with item j)

            // If first improvement mode found an improvement, stop searching
            if (break_outer_loop) {
                break;
            }
        }

        // If no improvement found, exit the global loop
        if (best_i == -1 || best_j == -1) {
            break;
        }

        // Apply the chosen swap to candidate
        candidate_sol.x[best_i] = 0.0f;
        candidate_sol.x[best_j] = 1.0f;
        float new_candidate_value = candidate_value + best_delta;

        // Update usage
        for (int k = 0; k < prob->m; k++) {
            candidate_usage[k] = candidate_usage[k] - prob->weights[k * prob->n + best_i] + prob->weights[k * prob->n + best_j];
        }

        // Repair if infeasible
        bool infeasible = false;
        for (int k = 0; k < prob->m; k++) {
            if (candidate_usage[k] > prob->capacities[k]) {
                infeasible = true;
                break;
            }
        }
        if (infeasible) {
            repair_solution(prob, &candidate_sol, candidate_usage, &new_candidate_value);
        }

        // Accept the move only if strictly better
        if (new_candidate_value > candidate_value) {
            improved = true;
            candidate_sol.value = new_candidate_value;

            // final feasibility check
            bool feasible_sol = true;
            for (int k = 0; k < prob->m; k++) {
                if (candidate_usage[k] > prob->capacities[k]) {
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

            current_value = current_sol->value;
        }
        // otherwise, we discard candidate_sol changes and continue
    }

    // Final feasibility check on current_sol
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
