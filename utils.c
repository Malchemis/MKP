//
// Holds utility functions for the program
// - A parser for the instances of the MKP problem.
// - A checker that verifies the validity of a solution.
// - A saver that writes the solution to a file.
// - A helper for the solver functions that gives an initial solution to work with.
//
#include <utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>


Arguments parse_cmd_args(const int argc, char *argv[]) {
    Arguments args;
    // Defaults
    args.instance_file   = nullptr;
    args.out_file        = "solutions/solution.txt";
    args.method          = "LS-FLIP";
    args.use_gpu         = 0;
    args.num_starts      = 5;
    args.max_time        = 60.0f;   // 1 minute default
    args.lambda          = 1e-2f;
    args.learning_rate   = 1e-2f;
    args.max_iters       = 1000;
    args.ls_max_checks   = 500;
    args.ls_mode         = LS_BEST_IMPROVEMENT;
    args.max_no_improv   = 100;
    args.k_max           = 500;

    if (argc < 2) {
        fprintf(stderr,
            "Usage: %s <instance_file> [--cpu|--gpu] "
            "[--method=LS-FLIP|LS-SWAP|VND|VNS|GD|MULTI-GD-VNS] "
            "[--output=solution.txt] "
            "[--max_time=seconds] "
            "[--num_starts=N] "
            "[--lambda=L] "
            "[--lr=LR] "
            "[--max_iters=MI] "
            "[--ls_max_checks=K] "
            "[--max_no_improv=NI] "
            "[--max_no_improv=NI] "
            "[--k_max=KM]\n",
            argv[0]
        );
        exit(EXIT_FAILURE);
    }
    args.instance_file = argv[1];

    // parse optional arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--gpu") == 0) {
            args.use_gpu = 1;
        } else if (strcmp(argv[i], "--cpu") == 0) {
            args.use_gpu = 0;
        } else if (strncmp(argv[i], "--method=", 9) == 0) {
            args.method = argv[i] + 9;
        } else if (strncmp(argv[i], "--output=", 9) == 0) {
            args.out_file = argv[i] + 9;
        } else if (strncmp(argv[i], "--max_time=", 10) == 0) {
            args.max_time = atof(argv[i] + 10);
        } else if (strncmp(argv[i], "--num_starts=", 13) == 0) {
            args.num_starts = atoi(argv[i] + 13);
        } else if (strncmp(argv[i], "--lambda=", 9) == 0) {
            args.lambda = atof(argv[i] + 9);
        } else if (strncmp(argv[i], "--lr=", 5) == 0) {
            args.learning_rate = atof(argv[i] + 5);
        } else if (strncmp(argv[i], "--max_iters=", 11) == 0) {
            args.max_iters = atoi(argv[i] + 11);
        } else if (strncmp(argv[i], "--ls_max_checks=", 6) == 0) {
            args.ls_max_checks = atoi(argv[i] + 6);
        } else if (strncmp(argv[i], "--ls_mode=", 10) == 0) {
            args.ls_mode = (strcmp(argv[i] + 10, "first") == 0) ? LS_FIRST_IMPROVEMENT : LS_BEST_IMPROVEMENT;
        } else if (strncmp(argv[i], "--max_no_improv=", 9) == 0) {
            args.max_no_improv = atoi(argv[i] + 9);
        } else if (strncmp(argv[i], "--k_max=", 8) == 0) {
            args.k_max = atoi(argv[i] + 8);
        }
    }
    return args;
}


int time_is_up(const clock_t start, const float max_time) {
    const double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    return (elapsed >= (double)max_time) ? 1 : 0;
}

/* Internal helper to read arrays in the required format */
static int read_array(FILE *fin, float *arr, const int count) {
    for (int i = 0; i < count; i++) {
        if (fscanf(fin, "%f", &arr[i]) != 1) {
            fprintf(stderr, "Error reading data.\n");
            return -1;
        }
    }
    return 0;
}

/* Internal helper to compare ratios in descending order */
int compare_ratios_descending(const void *a, const void *b) {
    const auto fa = (const float*)a;
    const auto fb = (const float*)b;
    return (fa[0] < fb[0]) - (fa[0] > fb[0]); // returns -1, 0, 1 for a < b, a == b, a > b
}

int parse_instance(const char *filename, Problem *prob) {
    // Read instance file, and check for errors
    FILE *fin = fopen(filename, "r");
    if (!fin) {
        fprintf(stderr, "Cannot open instance file %s.\n", filename);
        return -1;
    }

    // Read n and m, the number of items and constraints
    if (fscanf(fin, "%d %d", &prob->n, &prob->m) != 2) {
        fprintf(stderr, "Error reading n and m.\n");
        fclose(fin);
        return -1;
    }

    // Allocate memory for problem data
    prob->c              = (float*)malloc(prob->n * sizeof(float));
    prob->capacities     = (float*)malloc(prob->m * sizeof(float));
    prob->weights        = (float*)malloc(prob->m * prob->n * sizeof(float));
    prob->sum_of_weights = (float*)calloc(prob->n, sizeof(float));
    prob->ratios         = (float*)calloc(prob->n, sizeof(float));
    prob->candidate_list = (float*)malloc(prob->n * sizeof(float));

    // Check for allocation errors
    if (!prob->c || !prob->capacities || !prob->weights || !prob->sum_of_weights || !prob->ratios || !prob->candidate_list) {
        fprintf(stderr, "Memory allocation error.\n");
        fclose(fin);
        return -1;
    }

    // Read data
    if (read_array(fin, prob->c, prob->n) != 0) { fclose(fin); return -1; }
    if (read_array(fin, prob->capacities, prob->m) != 0) { fclose(fin); return -1; }
    if (read_array(fin, prob->weights, prob->m * prob->n) != 0) { fclose(fin); return -1; }

    // Precompute for each item j, the sum of weights w_ij and ratio c_j/w_ij
    for (int j = 0; j < prob->n; j++) {
        for (int i = 0; i < prob->m; i++) {
            prob->sum_of_weights[j] += prob->weights[i * prob->n + j];
        }
        prob->ratios[j] = prob->c[j] / prob->sum_of_weights[j];
    }

    // Fill candidate_list : Using quicksort, sort the items by decreasing ratio.
    for (int j = 0; j < prob->n; j++) {
        prob->candidate_list[j] = (float)j;
    }
    qsort(prob->candidate_list, prob->n, sizeof(float), compare_ratios_descending);

    fclose(fin);
    return 0;
}

void free_problem(Problem *prob) {
    if(!prob) return;
    free(prob->c); prob->c = nullptr;
    free(prob->capacities); prob->capacities = nullptr;
    free(prob->weights); prob->weights = nullptr;
    free(prob->sum_of_weights); prob->sum_of_weights = nullptr;
    free(prob->ratios); prob->ratios = nullptr;
    free(prob->candidate_list); prob->candidate_list = nullptr;
}

bool check_feasibility(const Problem *prob, const Solution *sol) {
    for (int i = 0; i < prob->m; i++) {
        float sum_w = 0.0f;
        const float *row = &prob->weights[i * prob->n];
        for (int j = 0; j < prob->n; j++) {
            sum_w += row[j] * sol->x[j];
        }
        if (sum_w > prob->capacities[i]) {
            return false;
        }
    }
    return true;
}

void evaluate_solution_cpu(const Problem *prob, Solution *sol) {
    // Objective = c^T x
    float val = 0.0f;
    for (int j = 0; j < prob->n; j++) {
        val += prob->c[j] * sol->x[j];
    }
    sol->value = val;

    // Check feasibility
    sol->feasible = check_feasibility(prob, sol);
}

void evaluate_solution_gpu(const Problem *prob, Solution *sol) {
    // Placeholder: Real code would allocate device memory, copy data, and use cublasSgemv.
    // For now, we just call CPU version.
    evaluate_solution_cpu(prob, sol);
}

void construct_initial_solution(const Problem *prob, Solution *sol,
                                void (*eval_func)(const Problem*, Solution*),
                                const int num_starts) {
    Solution best;
    allocate_solution(&best, prob->n);
    best.value = -INFINITY;
    best.feasible = false;

    for (int s = 0; s < num_starts; s++) {
        Solution candidate;
        allocate_solution(&candidate, prob->n);
        for (int j = 0; j < prob->n; j++) {
            candidate.x[j] = (rand() % 2 == 0) ? 1.0f : 0.0f;
        }
        eval_func(prob, &candidate);

        // Swap if the candidate is better (feasible when best is not, or higher value)
        if ((candidate.feasible && !best.feasible) ||
            (candidate.feasible == best.feasible && candidate.value > best.value)) {
            // Swap to make candidate the best
            swap_solutions(&best, &candidate);
        }

        free_solution(&candidate);
    }

    // Copy best to sol
    swap_solutions(&best, sol);
    free_solution(&best);

    // Modify solution to be feasible
    if (!sol->feasible) {
        for (int j = 0; j < prob->n; j++) {
            if (sol->x[j] > 0.5f) {
                sol->x[j] = 0.0f;
                eval_func(prob, sol);
                if (sol->feasible) break;
            }
        }
    }
}

void save_solution(const char *filename, const Solution *sol) {
    // Open file for writing and check for errors
    FILE *fout = fopen(filename, "w");
    if (!fout) {
        fprintf(stderr, "Error opening output file %s.\n", filename);
        return;
    }

    // Count the number of selected items
    int count_selected = 0;
    for (int j = 0; j < sol->n; j++) {
        if (sol->x[j] > 0.5f) count_selected++;
    }

    // Write to file the value and the number of selected items, then the list of selected items
    fprintf(fout, "%d %d\n", (int)sol->value, count_selected);
    for (int j = 0; j < sol->n; j++) {
        if (sol->x[j] > 0.5f) fprintf(fout, "%d ", j+1);
    }
    fprintf(fout, "\n");

    fclose(fout);
}

/**
 * @brief Computes usage array from a 0-1 solution x (stored in out_sol->x).
 *
 * usage[j] = sum_i weights[j*n + i] * out_sol->x[i].
 *
 * @param prob   The MKP instance
 * @param sol    The (binary) solution
 * @param usage  Array of length m to fill in
 */
void compute_usage_from_solution(const Problem *prob, const Solution *sol, float *usage) {
    for (int j = 0; j < prob->m; j++) {
        float sum_w = 0.0f;
        for (int i = 0; i < prob->n; i++) {
            sum_w += prob->weights[j * prob->n + i] * sol->x[i];
        }
        usage[j] = sum_w;
    }
}