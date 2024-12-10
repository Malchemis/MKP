//
// Holds utility functions for the program
// - A parser for the instances of the MKP problem.
// - A checker that verifies the validity of a solution.
// - A saver that writes the solution to a file.
// - A helper for the solver functions that gives an initial solution to work with.
//

#include <stdio.h>
#include <stdlib.h>

#include "lib/utils.h"

data* parse_instance(const char* filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    printf("Error opening file %s\n", filename);
    return nullptr;
  }

  printf("Parsing file %s\n", filename);

  // Read n_objects and m_constraints
  int n_objects, m_constraints;
  fscanf(file, "%d %d", &n_objects, &m_constraints);

  // Allocate memory for the data structure
  int* capacities = malloc(m_constraints * sizeof(int));
  int* coeffs = malloc(n_objects * sizeof(int));
  int** weights = malloc(n_objects * sizeof(int*));

  // Read coefficients
  for (int i = 0; i < n_objects / 10; i++) {
    for (int j = 0; j < 10; j++) {
      fscanf(file, "%d", &coeffs[i * 10 + j]);
    }
  }

  // Read capacities
  for (int i = 0; i < m_constraints; i++) {
    fscanf(file, "%d", &capacities[i]);
  }

  // Allocate memory for weights
  for (int i = 0; i < n_objects; i++) {
    weights[i] = malloc(m_constraints * sizeof(int)); // Allocate each row
  }

  // Read weights
  for (int i = 0; i < m_constraints; i++) {
    for (int j = 0; j < n_objects / 10; j++) {
      for (int k = 0; k < 10; k++) {
        fscanf(file, "%d", &weights[j * 10 + k][i]);
      }
    }
  }

  fclose(file);
  printf("File %s parsed successfully\n", filename);
  return create_data(n_objects, m_constraints, capacities, coeffs, weights);
}