#ifndef VND_H
#define VND_H

#include "data_structure.h"

/**
 * @brief Variable Neighborhood Descent routine.
 * Uses two local search procedures and systematically changes neighborhoods.
 * @param prob The problem instance.
 * @param sol The solution (improved in place if a better solution is found).
 * @param eval_func CPU or GPU evaluation function.
 */
void vnd(const Problem *prob, Solution *sol, void (*eval_func)(const Problem*, Solution*));

#endif
