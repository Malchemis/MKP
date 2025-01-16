#include "lib/vnd.h"
#include <local_search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

void vnd(const Problem *prob, Solution *sol, const int max_iter, const int k_max) {
    int iter = 0;
    int k = 0;
    Solution candidate_sol;
    allocate_solution(&candidate_sol, prob->n);
    copy_solution(sol, &candidate_sol);

    while (iter < max_iter) {
        k = 0;

        while (k < k_max && k < prob->n) {
            memcpy(candidate_sol.x, sol->x, prob->n * sizeof(float));
            candidate_sol.value = sol->value;

            // Shake
            const float old_xj = sol->x[k];
            const float new_xj = (old_xj > 0.5f) ? 0.0f : 1.0f;
            const float value_change = (new_xj > old_xj) ? prob->c[k] : -prob->c[k];
            const float new_value = sol->value + value_change;
            candidate_sol.x[k] = new_xj;
            candidate_sol.value = new_value;

            // Check feasibility
            const bool feasible = check_feasibility(prob, &candidate_sol);
            if (!feasible) {
                k++;
                continue;
            }

            // Search for a better solution
            local_search_flip(prob, &candidate_sol);

            // Update best solution
            if (candidate_sol.value > sol->value) {
                copy_solution(&candidate_sol, sol);
                k = 0;
            }
            else {
                k++;
            }
        }
        iter++;
    }
    free_solution(&candidate_sol);
}