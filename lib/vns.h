#ifndef VNS_H
#define VNS_H

#include "data_structure.h"

/**
 * @brief Variable Neighborhood Search:
 * - Uses VND and a perturbation procedure to escape local minima.
 * @param prob The problem instance.
 * @param sol The solution to improve.
 * @param max_no_improvement Maximum number of iterations without improvement before stopping.
 * @param k_max Maximum number of neighborhoods to try.
 * @param ls_k Number of items to consider in local search.
 * @param ls_mode The local search mode (first or best improvement).
 */
void vns(const Problem *prob, Solution *sol, const int max_no_improvement, const int k_max, const int ls_k, const LSMode ls_mode);

void shake(const Problem *p, const Solution *s, Solution *candidate, const int k);

#endif
