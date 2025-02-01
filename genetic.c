#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "genetic.h"
#include "data_structure.h"
#include "utils.h"

#define ELITE_PERCENTAGE 0.05
#define TOURNAMENT_SIZE 5
#define PENALTY_FACTOR 1.0f

void genetic_algorithm(const Problem *prob,
                       Solution *best_sol,
                       const int population_size,
                       const int max_generations,
                       const float mutation_rate,
                       const clock_t start,
                       const float max_time,
                       const LogLevel verbose)
{
    void (*eval_func)(const Problem*, Solution*) = evaluate_solution_cpu;

    // Allocate population
    Individual *population = malloc(population_size * sizeof(Individual));
    Individual *new_population = malloc(population_size * sizeof(Individual));

    // Allocate memory for solutions within each Individual
    for(int i = 0; i < population_size; i++) {
        allocate_solution(&population[i].sol, prob->n);
        allocate_solution(&new_population[i].sol, prob->n);
    }

    // Initialize population
    ga_init_population(prob, population, population_size, eval_func);

    // Track best solution
    int best_index = 0;
    float best_fitness;

    /* GA main loop */
    for(int gen = 0; gen < max_generations; gen++) {
        // Keep best 5% of the population
        int elite_count = (int)ceil(ELITE_PERCENTAGE * population_size);
        if (elite_count < 1) {
            elite_count = 1;
        }

        // Create an auxiliary array of pointers to individuals.
        Individual **sorted_population = malloc(population_size * sizeof(Individual *));
        if (!sorted_population) {
            fprintf(stderr, "Memory allocation error for sorted_population.\n");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < population_size; i++) {
            sorted_population[i] = &population[i];
        }

        // Sort the pointers in descending order by fitness.
        qsort(sorted_population, population_size, sizeof(Individual *), cmp_individual_ptrs_desc);

        // Copy the elite individuals (the best elite_count) into new_population.
        for (int i = 0; i < elite_count; i++) {
            ga_copy_individual(sorted_population[i], &new_population[i]);
        }
        free(sorted_population);

        // Fill the rest with new_population
        for(int i = 1; i < population_size; i++) {
            //Selection
            Individual parent1, parent2;
            allocate_solution(&parent1.sol, prob->n);
            allocate_solution(&parent2.sol, prob->n);

            ga_tournament_selection(population, population_size, TOURNAMENT_SIZE, &parent1, &parent2);

            //Crossover
            ga_single_point_crossover(prob, &parent1, &parent2, &new_population[i]);

            //Mutation
            ga_mutation(prob, &new_population[i], mutation_rate);

            //Repair & Evaluate new offspring
            ga_repair(prob, &new_population[i]);
            ga_evaluate_individual(prob, &new_population[i], eval_func);

            // Free parent's solution memory
            free_solution(&parent1.sol);
            free_solution(&parent2.sol);
        }

        // Swap populations for the next generation
        for(int i = 0; i < population_size; i++) {
            ga_swap_individuals(&population[i], &new_population[i]);
        }

        // Check time limit
        if (time_is_up(start, max_time)) {
            if (verbose == INFO || verbose == DEBUG) {
                printf("[GA] Time limit reached at generation %d.\n", gen);
            }
            break;
        }

        // Print progress
        if (verbose == DEBUG && (gen % 100 == 0)) {
            printf("[GA] Generation %d: best fitness = %.2f\n", gen, population[best_index].fitness);
        }
    }

    // Find best again after the last generation
    best_index = 0;
    best_fitness = population[0].fitness;
    for(int i = 1; i < population_size; i++) {
        if (population[i].fitness > best_fitness) {
            best_fitness = population[i].fitness;
            best_index = i;
        }
    }
    copy_solution(&population[best_index].sol, best_sol);

    // Clean up
    for(int i = 0; i < population_size; i++) {
        free_solution(&population[i].sol);
        free_solution(&new_population[i].sol);
    }
    free(population);
    free(new_population);
}

/* ------------------------------------------------------
 * GA Helper functions
 * ------------------------------------------------------ */

void ga_init_population(const Problem *prob,
                               Individual *population,
                               const int population_size,
                               void (*eval_func)(const Problem*, Solution*))
{
    for (int i = 0; i < population_size; i++) {
        // Random init: each item has 50% chance of being included
        for (int j = 0; j < prob->n; j++) {
            population[i].sol.x[j] = rand() / (float)RAND_MAX < 0.5f ? 1.0f : 0.0f;
        }
        // Evaluate the solution
        eval_func(prob, &population[i].sol);

        // Compute fitness: use value for feasible solutions, penalize infeasible ones
        if (population[i].sol.feasible) {
            population[i].fitness = population[i].sol.value;
        } else {
            // Penalization seems to give worse results
            population[i].fitness = 0.0f;
            //population[i].fitness = population[i].sol.value - compute_penalty(prob, &population[i].sol, PENALTY_FACTOR);
        }
    }
}

void ga_evaluate_individual(const Problem *prob,
                                   Individual *ind,
                                   void (*eval_func)(const Problem*, Solution*))
{
    eval_func(prob, &ind->sol);
    if (ind->sol.feasible) {
        ind->fitness = ind->sol.value;
    } else {
        ind->fitness = 0.0f;
        //ind->fitness = ind->sol.value - compute_penalty(prob, &ind->sol, PENALTY_FACTOR);
    }
}


void ga_tournament_selection(const Individual *population,
                               const int population_size,
                               const int tournament_size,
                               Individual *parent1,
                               Individual *parent2)
{
    // Ensure at least 2 candidates are selected
    int const t_size = tournament_size < 2 ? 2 : tournament_size;

    int best_index = -1, second_best_index = -1;
    float best_fitness = -INFINITY;
    float second_best_fitness = -INFINITY;

    for (int i = 0; i < t_size; i++) {
        const int idx = rand() % population_size;
        const float candidate_fitness = population[idx].fitness;
        if (candidate_fitness > best_fitness) {
            // Update second best with the old best
            second_best_fitness = best_fitness;
            second_best_index = best_index;

            // Update best with current candidate
            best_fitness = candidate_fitness;
            best_index = idx;
        } else if (candidate_fitness > second_best_fitness) {
            // Candidate is only better than the current second best.
            second_best_fitness = candidate_fitness;
            second_best_index = idx;
        }
    }

    // Failsafe: if no best was found, just pick the first one
    if (best_index < 0) {
        best_index = 0;
    }
    if (second_best_index < 0) {
        second_best_index = 0;
    }

    // Copy the selected individuals
    ga_copy_individual(&population[best_index], parent1);
    ga_copy_individual(&population[second_best_index], parent2);
}

void ga_single_point_crossover(const Problem *prob,
                                      const Individual *p1,
                                      const Individual *p2,
                                      const Individual *child)
{
    const int point = rand() % prob->n; // random crossover point

    for (int j = 0; j < point; j++) {
        child->sol.x[j] = p1->sol.x[j];
    }
    for (int j = point; j < prob->n; j++) {
        child->sol.x[j] = p2->sol.x[j];
    }
}

float compute_penalty(const Problem *prob, const Solution *sol, const float penalty_factor)
{
    float *usage = calloc(prob->m, sizeof(float));
    if (!usage) {
        fprintf(stderr, "Error allocating memory for usage.\n");
        exit(EXIT_FAILURE);
    }

    compute_usage_from_solution(prob, sol, usage);

    float penalty = 0.0f;
    for (int i = 0; i < prob->m; i++) {
        if (usage[i] > prob->capacities[i]) {
            penalty += penalty_factor * (usage[i] - prob->capacities[i]);
        }
    }

    free(usage);
    return penalty;
}

void ga_mutation(const Problem *prob,
                        const Individual *ind,
                        const float mutation_rate)
{
    for (int j = 0; j < prob->n; j++) {
        const float r = rand() / (float)RAND_MAX;
        if (r < mutation_rate) {
            ind->sol.x[j] = ind->sol.x[j] > 0.5f ? 0.0f : 1.0f;
        }
    }
}

void ga_repair(const Problem *prob,
                                Individual *ind)
{
    if (!check_feasibility(prob, &ind->sol)) {
        float *usage = calloc(prob->m, sizeof(float));
        compute_usage_from_solution(prob, &ind->sol, usage);

        float cur_value = ind->sol.value; /* might be negative or 0 if infeasible, but we'll just pass it in */

        repair_solution(prob, &ind->sol, usage, &cur_value);

        free(usage);
    }
}

void ga_copy_individual(const Individual *src, Individual *dst)
{
    copy_solution(&src->sol, &dst->sol);
    dst->fitness = src->fitness;
}

void ga_swap_individuals(Individual *i1, Individual *i2)
{
    swap_solutions(&i1->sol, &i2->sol);
    const float temp_fitness = i1->fitness;
    i1->fitness = i2->fitness;
    i2->fitness = temp_fitness;
}

int cmp_individual_ptrs_desc(const void *a, const void *b) {
    const Individual *ia = *(const Individual * const *)a;
    const Individual *ib = *(const Individual * const *)b;
    if (ia->fitness < ib->fitness)
        return 1;
    if (ia->fitness > ib->fitness)
        return -1;
    return 0;
}