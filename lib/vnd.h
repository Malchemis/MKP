#ifndef VND_H
#define VND_H

#include <time.h>
#include <utils.h>

#include "data_structure.h"

/**
 * @brief Variable Neighborhood Descent routine.
 * Uses two local search procedures and systematically changes neighborhoods.
 * @param prob                  The problem instance.
 * @param sol                   The solution (improved in place if a better solution is found).
 * @param max_no_improvement    Maximum number of iterations without improvement before stopping.
 * @param k_max                 Maximum number of neighborhoods to try.
 * @param ls_mode               The local search mode (first or best improvement).
 * @param ls_k                  The number of items to consider in local search.
 * @param start                 The start time for time limit.
 * @param max_time              The maximum allowed time.
 */
void vnd(const Problem *prob, Solution *sol, int max_no_improvement, int k_max, int ls_k, LSMode ls_mode, clock_t start, float max_time);

#endif
