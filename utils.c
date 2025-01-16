//
// Holds utility functions for the program
// - A parser for the instances of the MKP problem.
// - A checker that verifies the validity of a solution.
// - A saver that writes the solution to a file.
// - A helper for the solver functions that gives an initial solution to work with.
//
#include "lib/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

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
    prob->c = (float*)malloc(prob->n * sizeof(float));
    prob->capacities = (float*)malloc(prob->m * sizeof(float));
    prob->weights = (float*)malloc(prob->m * prob->n * sizeof(float));

    // Check for allocation errors
    if (!prob->c || !prob->capacities || !prob->weights) {
        fprintf(stderr, "Memory allocation error.\n");
        fclose(fin);
        return -1;
    }

    // Read data
    if (read_array(fin, prob->c, prob->n) != 0) { fclose(fin); return -1; }
    if (read_array(fin, prob->capacities, prob->m) != 0) { fclose(fin); return -1; }
    if (read_array(fin, prob->weights, prob->m * prob->n) != 0) { fclose(fin); return -1; }

    fclose(fin);
    return 0;
}

void free_problem(Problem *prob) {
    if(!prob) return;
    free(prob->c); prob->c = nullptr;
    free(prob->capacities); prob->capacities = nullptr;
    free(prob->weights); prob->weights = nullptr;
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

    srand((unsigned)time(nullptr));

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

void copy_solution(const Solution *src, Solution *dst) {
    dst->value = src->value;
    dst->feasible = src->feasible;
    dst->n = src->n;

    if (src->x != NULL && src->n > 0) {
        if (dst->x == NULL) {
            // Handle allocation failure (e.g., exit or return an empty solution)
            perror("Failed to allocate memory for solution vector");
            exit(EXIT_FAILURE);
        }
        memcpy(dst->x, src->x, src->n * sizeof(float));
    } else {
        dst->x = NULL;
    }
}