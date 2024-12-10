#ifndef DATA_H
#define DATA_H

typedef struct data {
    int n_objects;      // Number of objects
    int m_constraints;  // Number of constraints
    int* capacities;    // Constraint capacities
    int* coeffs;        // Coefficients of the objects
    int** weights;       // 3D array of weights
} data;

// Function to create a data structure
data* create_data(int n_objects, int m_constraints, const int* capacities, const int* coeffs, int** weights);

// Function to destroy a data structure
void destroy_data(data* d);

void print_data(const data* d);

#endif // DATA_H
