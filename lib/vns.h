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
void vns(const Problem *prob, Solution *sol, const int max_no_improvement, const int k_max, const int ls_k, const LSMode ls_mode);

void shake(const Problem *p, const Solution *s, Solution *candidate, const int k);

#endif
