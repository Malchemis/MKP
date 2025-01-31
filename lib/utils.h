#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include <data_structure.h>

/**
 * @brief Represents the local search mode.
 */
typedef enum {
    LS_FIRST_IMPROVEMENT,
    LS_BEST_IMPROVEMENT
} LSMode;

typedef enum {
    NONE,
    INFO,
    DEBUG
} LogLevel;

/**
 * @brief Holds all user-configurable parameters parsed from the command line.
 */
typedef struct {
    const char *instance_file;   /**< The input instance file path */
    const char *out_file;        /**< The output solution file path */
    const char *method;          /**< Which method to run (LS, VND, VNS, GD, etc.) */
    int        use_gpu;          /**< 1 = GPU, 0 = CPU */
    int        num_starts;       /**< Number of random starts for multi-start */
    float      max_time;         /**< Maximum allowed time in seconds */
    float      lambda;           /**< Penalty parameter for gradient solver */
    float      learning_rate;    /**< Learning rate for gradient solver */
    int        ls_max_checks;    /**< Local search 'k' param (max_checks, etc.) */
    LSMode     ls_mode;          /**< Local search mode (first or best improvement) */
    int        max_no_improv;    /**< Max no improvement for VND/VNS : The number of iterations without improvement before stopping */
    int        k_max;            /**< Max k for VNS : the number of neighborhoods to explore */
    LogLevel   log_level;        /**< Verbosity level */
} Arguments;

/**
 * @brief Parses command-line arguments into an Arguments struct.
 *
 * Usage example:
 *   ./mkp_solver instance.txt [--cpu|--gpu]
 *       [--method=LS-FLIP|LS-SWAP|VND|VNS|GD|MULTI-GD-VNS]
 *       [--output=solution.txt]
 *       [--max_time=10.0]
 *       [--num_starts=5]
 *       [--lambda=0.01]
 *       [--lr=1e-3]
 *       [--max_iters=1000]
 *       [--ls_max_checks=500]
 *       [--max_no_improv=100]
 *       [--k_max=500]
 */
Arguments parse_cmd_args(int argc, char *argv[]);

/**
 * @brief Checks if we exceeded the max_time. Returns 1 if time is up, else 0.
 */
int time_is_up(clock_t start, float max_time);


/**
 * @brief Parse an MKP instance from a given file.
 * @param filename Path to the instance file.
 * @param prob Problem structure to fill.
 * @return 0 on success, non-zero otherwise.
 */
int parse_instance(const char *filename, Problem *prob);

/**
 * @brief Free memory allocated for a problem and set pointers to NULL.
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
 */
void repair_solution(const Problem *prob, Solution *sol, float *usage, float *cur_value);

void compute_usage_from_solution(const Problem *prob, const Solution *sol, float *usage);

#endif