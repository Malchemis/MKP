#include <stdio.h>
#include <stdlib.h>

#include "lib/data.h"

data* create_data(
        const int n_objects,
        const int m_constraints,
        const int* capacities,
        const int* coeffs,
        int** weights) {

    data* d = malloc(sizeof(data));
    if (!d) return nullptr;

    d->n_objects = n_objects;
    d->m_constraints = m_constraints;
    d->capacities = malloc(m_constraints * sizeof(int));
    d->coeffs = malloc(n_objects * sizeof(int*));
    d->weights = malloc(n_objects * sizeof(int*));

    // Memory allocation failed
    if (!d->capacities || !d->coeffs || !d->weights) {
        free(d->capacities);
        free(d->coeffs);
        free(d->weights);
        free(d);
        printf("Failed to allocate memory for data structure\n");
        return nullptr;
    }

    // Copy capacities
    for (int i = 0; i < m_constraints; i++) {
        d->capacities[i] = capacities[i];
    }

    // Copy coefficients
    for (int i = 0; i < m_constraints; i++) {
        d->coeffs[i] = coeffs[i];
    }

    // Allocate and copy weights
    for (int i = 0; i < n_objects; i++) {
        d->weights[i] = malloc(m_constraints * sizeof(int));
        if (!d->weights[i]) {
            for (int j = 0; j < i; j++) {
                free(d->weights[j]);
            }
            free(d->capacities);
            free(d->coeffs);
            free(d->weights);
            free(d);
            printf("Failed to allocate memory for data structure\n");
            return nullptr;
        }
        for (int j = 0; j < m_constraints; j++) {
            d->weights[i][j] = weights[i][j];
        }
    }

    return d;
}

void destroy_data(data* d) {
    for (int i = 0; i < d->n_objects; i++) {
        free(d->weights[i]);
    }
    free(d->capacities);
    free(d->coeffs);
    free(d->weights);
    free(d);
}

void print_data(const data* d) {
    printf("Number of objects: %d\n", d->n_objects);
    printf("Number of constraints: %d\n", d->m_constraints);
    printf("Capacities: ");
    for (int i = 0; i < d->m_constraints; i++) {
        printf("%d ", d->capacities[i]);
    }
    printf("\n");
    printf("Coefficients: ");
    for (int i = 0; i < d->n_objects; i++) {
        printf("%d ", d->coeffs[i]);
    }
    printf("\n");
    printf("Weights:\n");
    for (int i = 0; i < d->n_objects; i++) {
        for (int j = 0; j < d->m_constraints; j++) {
            printf("%d ", d->weights[i][j]);
        }
        printf("\n");
    }
}