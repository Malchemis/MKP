#ifndef VNS_H
#define VNS_H

#include <time.h>
#include <utils.h>
#include "data_structure.h"

/**
 * @brief Variable Neighborhood Search:
 * - Uses VND and a perturbation procedure to escape local minima.
 * @param prob                  The problem instance.
 * @param sol                   The solution to improve.
 * @param max_no_improvement    Maximum number of iterations without improvement before stopping.
 * @param k_max                 Maximum number of neighborhoods to try.
 * @param ls_k                  Number of items to consider in local search.
 * @param ls_mode               The local search mode (first or best improvement).
 * @param start                 The start time for time limit.
 * @param max_time              The maximum allowed time.
 * @param prob The problem instance.
 * @param sol The solution to improve.
 * @param max_no_improvement Maximum number of iterations without improvement before stopping.
 * @param ls_k Number of items to consider in local search.
 * @param ls_mode The local search mode (first or best improvement).
 */
void vns(const Problem *prob, Solution *sol, int max_no_improvement, int k_max, int ls_k, LSMode ls_mode, clock_t start, float max_time);

void shake(const Problem *p, const Solution *s, Solution *candidate, int k);

#endif
