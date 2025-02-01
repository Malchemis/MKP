#ifndef GENETIC_H
#define GENETIC_H

#include "data_structure.h"
#include "utils.h"

/**
 * @brief Runs a Genetic Algorithm (GA) to solve the MKP.
 *
 * Steps:
 * - Initialize the population and evaluate each individual.
 * - Loop :
 *    - Identify and save the best individuals from the current population so they survive.
 *    - For each new offspring to be generated, select its parents, apply crossover and mutation.
 *    - Repair the offspring if necessary.
 *    - Evaluate the offspring.
 *    - Place the offspring in the new population.
 *
 * @param prob            The MKP problem instance.
 * @param best_sol        Output: the best solution found by the GA.
 * @param population_size The number of individuals in the population.
 * @param max_generations The maximum number of generations to run.
 * @param mutation_rate   Probability of mutating each bit (gene) in an offspring.
 * @param start           The start time (to check against max_time).
 * @param max_time        The maximum allowed time in seconds.
 * @param verbose         Verbosity level (NONE, INFO, DEBUG).
 *
 * @note On completion, best_sol will hold the best solution found.
 */
void genetic_algorithm(const Problem *prob,
                       Solution *best_sol,
                       int population_size,
                       int max_generations,
                       float mutation_rate,
                       clock_t start,
                       float max_time,
                       LogLevel verbose);

/**
 * @brief Randomly initialize the population, evaluate each individual.
 */
void ga_init_population(const Problem *prob,
                               Individual *population,
                               int population_size,
                               void (*eval_func)(const Problem*, Solution*));

/**
 * @brief Evaluate an individual's solution (updates fitness).
 */
void ga_evaluate_individual(const Problem *prob,
                                   Individual *ind,
                                   void (*eval_func)(const Problem*, Solution*));

/**
 * @brief Tournament selection (size=2 for simplicity).
 *
 * @param population       The current population.
 * @param population_size  Size of the population.
 * @param tournament_size  Number of individuals to consider in the tournament. Lower is more exploitative.
 * @param parent1          Output: first selected parent
 * @param parent2          Output: second selected parent
 */
void ga_tournament_selection(const Individual *population,
                               int population_size,
                               int tournament_size,
                               Individual *parent1,
                               Individual *parent2);

/**
 * @brief Single-point crossover.
 *
 * @param prob  The MKP problem instance
 * @param p1    Parent 1
 * @param p2    Parent 2
 * @param child Output: child
 */
void ga_single_point_crossover(const Problem *prob, const Individual *p1, const Individual *p2, Individual *child);

/**
 * @brief Bit-flip mutation.
 *
 * @param prob          The problem instance (n dimension).
 * @param ind           The individual to mutate.
 * @param mutation_rate Probability of flipping each bit.
 */
void ga_mutation(const Problem *prob, Individual *ind, float mutation_rate);

/**
 * @brief Computes a penalty for a solution based on constraint violations.
 *
 * For each constraint, if the usage exceeds the capacity, the penalty is:
 *    penalty += penalty_factor * (usage - capacity)
 *
 * @param prob           The MKP problem instance.
 * @param sol            The solution to evaluate.
 * @param penalty_factor The coefficient to weight constraint violations.
 * @return The computed penalty.
 */
float compute_penalty(const Problem *prob, const Solution *sol, const float penalty_factor);

/**
 * @brief Repair if the solution is infeasible.
 *
 * A simple approach: remove items with the worst "value/cost" ratio until feasible.
 */
void ga_repair(const Problem *prob,
                                Individual *ind);

/**
 * @brief Copy Individual (solution + fitness).
 */
void ga_copy_individual(const Individual *src, Individual *dst);

/**
 * @brief Swap the data between two Individuals
 */
void ga_swap_individuals(Individual *i1, Individual *i2);

/**
 * @brief Compare two Individuals by fitness (ascending order).
 */
static int cmp_individual_ptrs_desc(const void *a, const void *b);

#endif // GENETIC_H