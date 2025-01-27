#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include "data_structure.h"

/**
 * @brief Perform a local search with a 1-flip neighborhood or exchange neighborhood.
 * @param prob The problem instance.
 * @param current_sol The current solution (improved in place).
 * @param k The maximum number of items (top-k by ratio) to explore
 * @param mode LS_FIRST_IMPROVEMENT or LS_BEST_IMPROVEMENT
 */
void local_search_flip(const Problem *prob, Solution *current_sol, int k, LSMode mode);


/**
 * @brief Repairs the solution if it violates capacity constraints.
 *
 * Simple strategy: while any constraint is violated, remove one item (x_j=0)
 * that yields the smallest "value/cost" ratio (or largest weight per value).
 *
 * @param prob       The MKP problem instance
 * @param sol        The solution (possibly infeasible) to repair
 * @param usage      Current usage array of length m
 * @param cur_value  Current objective value (updated in place if items removed)
 * @param sum_of_weights Precomputed sum of weights for each item j
 */
void repair_solution(const Problem *prob, const Solution *sol, float *usage, float *cur_value, const float *sum_of_weights);

#endif

