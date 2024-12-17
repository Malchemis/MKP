#include "lib/data_structure.h"
#include <stdlib.h>
#include <stdio.h>

void allocate_solution(Solution *sol, int n) {
    sol->n = n;
    sol->x = (float*)malloc(n * sizeof(float));
    if (!sol->x) {
        fprintf(stderr, "Failed to allocate solution.\n");
        exit(EXIT_FAILURE);
    }
    sol->value = 0.0f;
    sol->feasible = false;
}

void free_solution(Solution *sol) {
    if (!sol) return;
    free(sol->x);
    sol->x = nullptr;
}

void swap_solutions(Solution *s1, Solution *s2) {
    // Swap pointers and metadata
    int temp_n = s1->n;
    s1->n = s2->n;
    s2->n = temp_n;

    float *temp_x = s1->x;
    s1->x = s2->x;
    s2->x = temp_x;

    float temp_val = s1->value;
    s1->value = s2->value;
    s2->value = temp_val;

    bool temp_feas = s1->feasible;
    s1->feasible = s2->feasible;
    s2->feasible = temp_feas;
}
