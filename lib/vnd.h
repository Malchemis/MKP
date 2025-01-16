#ifndef VND_H
#define VND_H

#include "data_structure.h"

/**
 * @brief Variable Neighborhood Descent routine.
 * Uses two local search procedures and systematically changes neighborhoods.
 * @param prob The problem instance.
 * @param sol The solution (improved in place if a better solution is found).
 * @param max_iter Maximum number of iterations.
 * @param k_max Maximum number of neighborhoods to try.
 */
void vnd(const Problem *prob, Solution *sol, const int max_iter, const int k_max);

#endif
