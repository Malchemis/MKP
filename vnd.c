#include "lib/vnd.h"
#include <local_search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

void vnd(const Problem *prob, Solution *sol, const int max_no_improvement, const int k_max, const int ls_k, const LSMode ls_mode) {
    int iter = 0;
    int k = 0;
    int no_improvement = 0;

    Solution candidate_sol;
    allocate_solution(&candidate_sol, prob->n);
    copy_solution(sol, &candidate_sol);

    while (no_improvement < max_no_improvement) {
        k = 0;
        bool improved = false;

        while (k < k_max && k < prob->n) {
            // Reset candidate solution
            memcpy(candidate_sol.x, sol->x, prob->n * sizeof(float));
            candidate_sol.value = sol->value;

            // Search for a better solution
            local_search_flip(prob, &candidate_sol, ls_k, ls_mode);
            local_search_swap(prob, &candidate_sol, ls_k, ls_mode);

            // Update best solution
            if (candidate_sol.value > sol->value) {
                copy_solution(&candidate_sol, sol);
                improved = true;
                k = 0;
            }
            else {
                k++;
            }
        }
        if (improved) {
            no_improvement = 0;
        } else {
            no_improvement++;
        }
        iter++;
    }
    free_solution(&candidate_sol);
}