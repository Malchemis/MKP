#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include <time.h>
#include <utils.h>

/**
 * @brief Perform a local search using a flip-based neighborhood.
 *
 * This local search attempts to flip items from 0->1 to find an improving move.
 * If a 0->1 flip causes infeasibility, the repair procedure is called.
 *
 * @param prob        Pointer to the MKP problem instance.
 * @param current_sol Pointer to the current solution (will be modified in place).
 * @param max_checks  Maximum number of flips to try (or number of items to explore).
 * @param mode        Local search mode: LS_FIRST_IMPROVEMENT or LS_BEST_IMPROVEMENT.
 * @param start       Start time for time limit.
 * @param max_time    Maximum allowed time.
 */
void local_search_flip(const Problem *prob, Solution *current_sol, int max_checks, LSMode mode, clock_t start, float max_time);


/**
 * @brief Local Search (Swap) neighborhood:
 *
 * Tries swapping one item in the solution (1) with one item not in the solution (0).
 * If the swap leads to a higher profit (and can be repaired if infeasible), we accept it and repeat.
 *
 * @param prob        The MKP problem instance
 * @param current_sol The current solution (will be modified in place)
 * @param max_checks  How many items to check from candidate_list
 * @param mode        LS_FIRST_IMPROVEMENT or LS_BEST_IMPROVEMENT
 * @param start       Start time for time limit
 * @param max_time    Maximum allowed time
 */
void local_search_swap(const Problem *prob, Solution *current_sol, int max_checks, LSMode mode, clock_t start, float max_time);

#endif

