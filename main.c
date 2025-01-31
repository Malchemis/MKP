#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <data_structure.h>
#include <utils.h>
#include <local_search.h>
#include <vnd.h>
#include <vns.h>
#include <gradesc.h>

/**
 * @brief Main entry point.
 * Usage: ./mkp_solver <instance_file> [--cpu|--gpu] [--method=LS|VND|VNS] [--output=solution.txt]
 */
int main(const int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <instance_file> [--cpu|--gpu] [--method=LS|VND|VNS] [--output=solution.txt]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    int use_gpu = 0;
    const char *method = "LS";     // default method: local search
    const char *out_file = "solutions/solution.txt";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--gpu") == 0) use_gpu = 1;
        else if (strcmp(argv[i], "--cpu") == 0) use_gpu = 0;
        else if (strncmp(argv[i], "--method=", 9) == 0) method = argv[i]+9;
        else if (strncmp(argv[i], "--output=", 9) == 0) out_file = argv[i]+9;
    }

    Problem prob;
    if (parse_instance(filename, &prob) != 0) {
        return EXIT_FAILURE;
    }

    // Choose evaluation function
    void (*eval_func)(const Problem*, Solution*) = use_gpu ? evaluate_solution_gpu : evaluate_solution_cpu;

    // We seed the random number generator for reproducibility
    srand(42);

    // Keep track of time
    const clock_t start = clock();

    // Construct initial solution
    Solution sol;
    allocate_solution(&sol, prob.n);
    construct_initial_solution(&prob, &sol, eval_func, 10);

    printf("--- MKP Solver ---\n");
    printf("Initial Solution:\n");
    printf("Value: %.2f\n", sol.value);
    printf("Feasible: %s\n\n", sol.feasible ? "Yes" : "No");

    constexpr int k = 500;
    constexpr LSMode mode = LS_BEST_IMPROVEMENT;
    // constexpr LSMode mode = LS_FIRST_IMPROVEMENT;

    // Apply method
    if (strcmp(method, "LS-FLIP") == 0){
        printf("Using Local Search (Flip) method.\n");
        local_search_flip(&prob, &sol, k, mode);
    } else if (strcmp(method, "LS-SWAP") == 0) {
        printf("Using Local Search (Flip) method.\n");
        local_search_swap(&prob, &sol, k, mode);
    } else if (strcmp(method, "VND") == 0) {
        printf("Using VND method.\n");
        vnd(&prob, &sol, 100, 500, k, mode);
    } else if (strcmp(method, "VNS") == 0) {
        printf("Using VNS method.\n");
        vns(&prob, &sol, 30, 500, k, mode);
    } else if (strcmp(method, "GD") == 0) {
        constexpr int max_iters = 600;
        constexpr float learning_rate = 5e-3f;
        constexpr float lambda = 5e-3f;
        printf("Using Gradient Descent method.\n");
        gradient_solver(&prob, lambda, learning_rate, max_iters, &sol);
    }
    else {
        fprintf(stderr, "Unknown method %s. Using LS.\n", method);
        local_search_flip(&prob, &sol, k, mode);
    }

    const clock_t end = clock();
    const double cpu_time_used = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\nFinal Solution:\n");
    printf("Value: %.2f\n", sol.value);
    printf("Feasible: %s\n", sol.feasible ? "Yes" : "No");
    printf("Time: %f seconds\n", cpu_time_used);

    // Save solution
    save_solution(out_file, &sol);

    free_solution(&sol);
    free_problem(&prob);

    return EXIT_SUCCESS;
}