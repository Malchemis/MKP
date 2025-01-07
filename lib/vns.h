#ifndef VNS_H
#define VNS_H

#include "data_structure.h"

/**
 * @brief Variable Neighborhood Search:
 * - Uses VND and a perturbation procedure to escape local minima.
 * @param prob The problem instance.
 * @param sol The solution to improve.
 * @param eval_func Evaluation function (CPU/GPU).
 */
void vns(const Problem *prob, Solution *sol, void (*eval_func)(const Problem*, Solution*));

#endif
