//
// Variable Neighborhood Search.
// Uses VND and a perturbation method.
//

#include "lib/vns.h"

#include <local_search.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include <vnd.h>

void vns(const Problem *prob, Solution *sol, const int max_no_improvement, const int k_max, const int ls_k, const LSMode ls_mode, const clock_t start, const float max_time) {
    int iter = 0;
    int k = 0;
    int no_improvement = 0;

    Solution candidate_sol;
    allocate_solution(&candidate_sol, prob->n);
    copy_solution(sol, &candidate_sol);

    while (no_improvement < max_no_improvement) {
        k = 0;
        bool improved = false;
        while (k <= k_max) {
            // Shake
            shake(prob, sol, &candidate_sol, k);

            // Search for a better solution
            vnd(prob, &candidate_sol, 5, ls_k, ls_mode);

            // Update best solution
            if (candidate_sol.value > sol->value) {
                improved = true;
                copy_solution(&candidate_sol, sol);
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

void shake(const Problem *p, const Solution *s, Solution *candidate, const int k) {
    memcpy(candidate->x, s->x, p->n * sizeof(float));
    candidate->value = s->value;

    const int n = p->n;
    // If k > n, there's no point flipping more than n unique indices:
    const int flips = (k < n) ? k : n;

    // Create an array of all indices
    int *indices = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        indices[i] = i;
    }

    // Randomize the order of indices
    for (int i = n - 1; i > 0; i--) {
        const int j = rand() % (i + 1);
        // Swap
        const int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    // Flip the first 'flips' distinct indices
    for (int i = 0; i < flips; i++) {
        const int idx = indices[i];

        const float old_val   = candidate->x[idx];
        const float new_val   = (old_val > 0.5f) ? 0.0f : 1.0f;
        const float delta_val = (new_val > old_val) ? p->c[idx] : -p->c[idx];

        candidate->x[idx] = new_val;
        candidate->value  += delta_val;
    }

    free(indices);

    // Check feasibility
    if (!check_feasibility(p, candidate)) {
        const auto usage   = (float*)malloc(p->m * sizeof(float));
        compute_usage_from_solution(p, candidate, usage);
        compute_usage_from_solution(p, candidate, usage);
        repair_solution(p, candidate, usage, &candidate->value);
    }
}