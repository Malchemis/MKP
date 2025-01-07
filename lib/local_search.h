#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include "data_structure.h"
#include "utils.h"

/**
 * @brief Perform a local search with a 1-flip neighborhood or exchange neighborhood.
 * @param prob The problem instance.
 * @param current_sol The current solution (improved in place).
 */
void local_search_flip(const Problem *prob, Solution *current_sol);

#endif

