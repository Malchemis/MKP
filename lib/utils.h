#ifndef UTILS_H
#define UTILS_H

#include "data_structure.h"

/**
 * @brief Parse an MKP instance from a given file.
 * @param filename Path to the instance file.
 * @param prob Problem structure to fill.
 * @return 0 on success, non-zero otherwise.
 */
int parse_instance(const char *filename, Problem *prob);

/**
 * @brief Free memory allocated for a problem.
 * @param prob The problem to free.
 */
void free_problem(Problem *prob);

/**
 * @brief Evaluate a solution on CPU using BLAS (if available) or a manual matrix multiply.
 * @param prob The problem instance.
 * @param sol The solution to evaluate. On return, value and feasibility are updated.
 */
void evaluate_solution_cpu(const Problem *prob, Solution *sol);

/**
 * @brief Evaluate a solution on GPU using cuBLAS (placeholder).
 * @param prob The problem instance.
 * @param sol The solution to evaluate. On return, value and feasibility are updated.
 */
void evaluate_solution_gpu(const Problem *prob, Solution *sol);

/**
 * @brief Generate an initial solution. Could be random, greedy, etc.
 * @param prob The problem instance.
 * @param sol The solution to initialize.
 * @param eval_func Pointer to evaluation function (CPU or GPU).
 * @param num_starts Number of random starts or attempts.
 */
void construct_initial_solution(const Problem *prob, Solution *sol,
                                void (*eval_func)(const Problem*, Solution*),
                                int num_starts);

/**
 * @brief Check feasibility of a solution (called after evaluation to confirm constraint satisfaction).
 * @param prob The problem instance.
 * @param sol The solution to check.
 * @return true if feasible, false otherwise.
 */
bool check_feasibility(const Problem *prob, const Solution *sol);

/**
 * @brief Save the best solution found to a file, as specified by the problem's output format.
 * Which is :
 * - Line 1: solution_value number_of_selected_items
 * - Line 2: list_of_selected_items (1-based indexing)
 * @param filename The output file path.
 * @param sol The solution to save.
 */
void save_solution(const char *filename, const Solution *sol);

#endif
