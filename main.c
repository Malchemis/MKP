#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <data_structure.h>
#include <utils.h>          // parse args, parse_instance, free_problem,...
#include <local_search.h>
#include <vnd.h>
#include <vns.h>
#include <gradesc.h>
#include <genetic.h>


/* Multi-start approach: for each random init, we run GD, then VNS, keep the best solution */
static void multi_start_gd_vns(const Problem *prob, const Arguments *args,
                               void (*eval_func)(const Problem*, Solution*),
                               Solution *best_sol) {
    Solution candidate;
    allocate_solution(&candidate, prob->n);

    // We can keep track of time
    const clock_t start_time = clock();

    best_sol->value = -INFINITY;
    best_sol->feasible = false;

    // For multiple starts, we do random init => GD => VNS => compare
    for (int s = 0; s < args->num_starts; s++) {
        if (time_is_up(start_time, args->max_time)) break;

        // Construct a random solution
        for (int j = 0; j < prob->n; j++) {
            candidate.x[j] = (rand() % 2) ? 1.0f : 0.0f;
        }
        eval_func(prob, &candidate);

        // Run gradient descent if time remains
        if (!time_is_up(start_time, args->max_time)) {
            gradient_solver(prob,
                            args->lambda,
                            args->learning_rate,
                            args->max_no_improv,
                            &candidate,
                            args->log_level,
                            start_time, args->max_time);
        }

        // Run VNS if time remains
        if (!time_is_up(start_time, args->max_time)) {
            vns(prob,
                &candidate,
                args->max_no_improv,
                args->k_max,
                args->ls_max_checks,
                LS_BEST_IMPROVEMENT,
                start_time,
                args->max_time,
                args->log_level);
        }

        // Evaluate or re-check feasibility if needed
        eval_func(prob, &candidate);
        candidate.feasible = check_feasibility(prob, &candidate);

        // Compare with best
        if ((candidate.feasible && !best_sol->feasible) ||
            (candidate.feasible == best_sol->feasible && candidate.value > best_sol->value)) {
            copy_solution(&candidate, best_sol);
            if (args->log_level >= INFO) {
                printf("New best solution: %.2f\n", best_sol->value);
            }
        }
    }

    free_solution(&candidate);
}

/**
 * @brief Main entry point
 */
int main(const int argc, char *argv[]) {
    // Parse the command-line
    const Arguments args = parse_cmd_args(argc, argv);
    if (!args.instance_file) {
        return EXIT_FAILURE;
    }

    // Read the MKP instance
    Problem prob;
    if (parse_instance(args.instance_file, &prob) != 0) {
        return EXIT_FAILURE;
    }

    // Choose evaluation function
    void (*eval_func)(const Problem*, Solution*) =
        args.use_gpu ? evaluate_solution_gpu : evaluate_solution_cpu;

    // Seed RNG
    srand(42);

    // Keep track of overall time
    const clock_t start = clock();

    // Allocate a solution structure
    Solution sol;
    allocate_solution(&sol, prob.n);

    printf("--- MKP Solver ---\n");
    printf("Instance: %s\n", args.instance_file);
    printf("Method:   %s\n", args.method);
    printf("Max Time: %.2f sec\n", args.max_time);
    printf("Verbosity: %s\n", args.log_level == NONE ? "NONE" : args.log_level == INFO ? "INFO" : "DEBUG");

    // Decide which approach to run
    if (strcmp(args.method, "MULTI-GD-VNS") == 0) {
        printf("\nStarting Multi-start GD-VNS with these parameters:\n");
        printf("Num starts: %d\n", args.num_starts);
        printf("Lambda: %f\n", args.lambda);
        printf("Learning rate: %f\n", args.learning_rate);
        printf("Max no improvement: %d\n", args.max_no_improv);
        printf("K max: %d\n", args.k_max);
        printf("LS k: %d\n", args.ls_max_checks);
        printf("LS mode: %s\n", args.ls_mode == LS_FIRST_IMPROVEMENT ? "First" : "Best");
        multi_start_gd_vns(&prob, &args, eval_func, &sol);
    }
    else if (strcmp(args.method, "LS-FLIP") == 0) {
        printf("\nStarting LS-FLIP with these parameters:\n");
        printf("LS max checks: %d\n", args.ls_max_checks);
        printf("Num starts: %d\n", args.num_starts);
        construct_initial_solution(&prob, &sol, eval_func, args.num_starts);
        local_search_flip(&prob, &sol, args.ls_max_checks, LS_BEST_IMPROVEMENT);
    }
    else if (strcmp(args.method, "LS-SWAP") == 0) {
        printf("\nStarting LS-SWAP with these parameters:\n");
        printf("LS max checks: %d\n", args.ls_max_checks);
        printf("Num starts: %d\n", args.num_starts);
        construct_initial_solution(&prob, &sol, eval_func, args.num_starts);
        local_search_swap(&prob, &sol, args.ls_max_checks, LS_BEST_IMPROVEMENT);
    }
    else if (strcmp(args.method, "GD") == 0) {
        printf("\nStarting Gradient descent with these parameters:\n");
        printf("Lambda: %f\n", args.lambda);
        printf("Learning rate: %f\n", args.learning_rate);
        printf("Max no improvement: %d\n", args.max_no_improv);
        construct_initial_solution(&prob, &sol, eval_func, args.num_starts);
        gradient_solver(&prob,
            args.lambda,
            args.learning_rate,
            args.max_no_improv,
            &sol,
            args.log_level,
            start,
            args.max_time);
    }
    else if (strcmp(args.method, "VNS") == 0) {
        printf("\nStarting Variable Neighborhood Search with these parameters:\n");
        printf("Max no improvement: %d\n", args.max_no_improv);
        printf("K max: %d\n", args.k_max);
        printf("LS k: %d\n", args.ls_max_checks);
        printf("LS mode: %s\n", args.ls_mode == LS_FIRST_IMPROVEMENT ? "First" : "Best");
        construct_initial_solution(&prob, &sol, eval_func, args.num_starts);
        vns(&prob,
            &sol,
            args.max_no_improv,
            args.k_max,
            args.ls_max_checks,
            LS_BEST_IMPROVEMENT,
            start,
            args.max_time,
            args.log_level);
    }
    else if (strcmp(args.method, "VND") == 0) {
        printf("\nStarting Variable Neighborhood Descent with these parameters:\n");
        printf("Max no improvement: %d\n", args.max_no_improv);
        printf("LS k: %d\n", args.ls_max_checks);
        printf("LS mode: %s\n", args.ls_mode == LS_FIRST_IMPROVEMENT ? "First" : "Best");
        construct_initial_solution(&prob, &sol, eval_func, args.num_starts);
        vnd(&prob, &sol, args.max_no_improv, args.ls_max_checks, LS_BEST_IMPROVEMENT, start, args.max_time);
    }
    else if (strcmp(args.method, "GA") == 0) {
        printf("\nStarting Genetic Algorithm with these parameters:\n");
        printf("Population size: %d\n", args.population_size);
        printf("Max generations: %d\n", args.max_generations);
        printf("Mutation rate: %.2f\n", args.mutation_rate);
        construct_initial_solution(&prob, &sol, eval_func, args.num_starts);
        genetic_algorithm(&prob,
            &sol,
            args.population_size,
            args.max_generations,
            args.mutation_rate,
            start,
            args.max_time,
            args.log_level);
    }
    else {
        fprintf(stderr, "Unknown method %s. Using LS-FLIP.\n", args.method);
        construct_initial_solution(&prob, &sol, eval_func, args.num_starts);
        local_search_flip(&prob, &sol, args.ls_max_checks, LS_BEST_IMPROVEMENT);
    }

    // Measure elapsed time
    const clock_t end = clock();
    const double cpu_time_used = (double)(end - start) / CLOCKS_PER_SEC;

    // Print final solution info
    printf("\nFinal Solution:\n");
    printf("Value: %.2f\n", sol.value);
    printf("Feasible: %s\n", sol.feasible ? "Yes" : "No");
    printf("Time: %f seconds\n", cpu_time_used);

    // Save solution
    save_solution(args.out_file, &sol);

    // Cleanup
    free_solution(&sol);
    free_problem(&prob);

    return EXIT_SUCCESS;
}
