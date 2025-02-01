#include "lib/data_structure.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void allocate_solution(Solution *sol, const int n) {
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
    const int temp_n = s1->n;
    s1->n = s2->n;
    s2->n = temp_n;

    float *temp_x = s1->x;
    s1->x = s2->x;
    s2->x = temp_x;

    const float temp_val = s1->value;
    s1->value = s2->value;
    s2->value = temp_val;

    const bool temp_feas = s1->feasible;
    s1->feasible = s2->feasible;
    s2->feasible = temp_feas;
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
        dst->x = nullptr;
    }
}

void print_problem(const Problem *p) {
    printf("Number of objects: %d\n", p->n);
    printf("Number of constraints: %d\n", p->m);
    printf("Capacities: ");
    for (int i = 0; i < p->m; i++) {
        printf("%d ", (int) p->capacities[i]);
    }
    printf("\n");
    printf("Coefficients: ");
    for (int i = 0; i < p->n; i++) {
        printf("%d ", (int) p->weights[i]);
    }
    printf("\n");
    printf("Weights:\n");
    for (int i = 0; i < p->n; i++) {
        for (int j = 0; j < p->m; j++) {
            printf("%d ", (int) p->weights[i * p->m + j]);
        }
        printf("\n");
    }
}

void print_solution(const Solution *s) {
    printf("Value: %f\n", s->value);
    printf("Feasible: %s\n", s->feasible ? "Yes" : "No");
    printf("Selected items: ");
    for (int i = 0; i < s->n; i++) {
        if (s->x[i] > 0.5f) {
            printf("%d ", i);
        }
    }
    printf("\n");
}