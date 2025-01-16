#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

/**
 * @brief Represents the MKP problem data.
 */
typedef struct {
    int n;                  /**< Number of items */
    int m;                  /**< Number of constraints */
    float *c;               /**< Objective coefficients, length n */
    float *capacities;      /**< Capacities for each constraint, length m */
    float *weights;         /**< Weights matrix, length m*n, row-major: W[i,j] = weights[i*n+j] */
    float *sum_of_weights;  /**< length n, sum of each item's weight across all constraints */
    float *ratios;          /**< length n, ratio c[j] / sum_of_weights[j] */
    float *candidate_list;  /**< length n, indexes of items sorted by ratio */
} Problem;

/**
 * @brief Represents a candidate solution to the MKP.
 *
 * The solution vector x is stored as floats (0.0 or 1.0) to facilitate
 * matrix multiplication using BLAS/cuBLAS.
 */
typedef struct {
    int n;       /**< Number of items */
    float *x;    /**< Solution vector (array of length n) with 0.0f or 1.0f */
    float value; /**< Objective value of this solution */
    bool feasible; /**< Whether this solution is feasible or not */
} Solution;


/**
 * @brief Represents the local search mode.
 */
typedef enum {
    LS_FIRST_IMPROVEMENT,
    LS_BEST_IMPROVEMENT
} LSMode;

/**
 * @brief Allocates memory for a solution of size n.
 * @param sol The solution structure to allocate.
 * @param n Number of items.
 */
void allocate_solution(Solution *sol, int n);

/**
 * @brief Frees memory allocated for a solution.
 * @param sol The solution to free.
 */
void free_solution(Solution *sol);

/**
 * @brief Swap the solution vectors of two solutions without copying arrays.
 * @param s1 First solution
 * @param s2 Second solution
 *
 * This can be useful in local search routines where we generate a neighbor
 * and just want to swap pointers rather than copy arrays.
 */
void swap_solutions(Solution *s1, Solution *s2);

#endif
