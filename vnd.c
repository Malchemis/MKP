#include "lib/vnd.h"
#include <local_search.h>
#include <stdio.h>

void vnd(const Problem *prob,
        Solution *sol,
        const int max_no_improvement,
        const int ls_k,
        const LSMode ls_mode,
        const clock_t start,
        const float max_time) {

    int no_improvement = 0;

    // Allocate candidate solution once
    Solution candidate_sol;
    allocate_solution(&candidate_sol, prob->n);

    // Repeat until we reach the maximum allowed iterations without improvement
    while (no_improvement < max_no_improvement && !time_is_up(start, max_time)) {
        bool improved = false;

        // Flip first
        copy_solution(sol, &candidate_sol);
        local_search_flip(prob, &candidate_sol, ls_k, ls_mode);

        if (candidate_sol.value > sol->value) {
            copy_solution(&candidate_sol, sol);
            improved = true;
        }
        else {
            // Swap
            copy_solution(sol, &candidate_sol);
            local_search_swap(prob, &candidate_sol, ls_k, ls_mode);
            if (candidate_sol.value > sol->value) {
                copy_solution(&candidate_sol, sol);
                improved = true;
            }
        }

        // Track consecutive iterations with no improvement
        if (improved) {
            no_improvement = 0;
        } else {
            no_improvement++;
        }
    }

    // Free candidate solution after finishing
    free_solution(&candidate_sol);
}
