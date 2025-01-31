#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include "data_structure.h"

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
 */
void local_search_flip(const Problem *prob, Solution *current_sol, int max_checks, LSMode mode);

#endif

