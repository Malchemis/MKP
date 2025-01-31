#ifndef GRADESC_H
#define GRADESC_H

#include <time.h>
#include <data_structure.h>
#include <utils.h>


/**
 * @brief Gradient Descent solver with momentum and optional item-freezing.
 *
 * Steps:
 *  1. Randomly initialize theta[i].
 *  2. For each iteration:
 *     a) Compute x_hat[i] = sigmoid(theta[i]) for non-frozen items.
 *     b) Compute usage.
 *     c) Compute gradient = partial derivatives of the penalized loss.
 *     d) Update velocity (momentum) and theta for non-frozen items.
 *     e) Optionally freeze items if x_hat[i] is consistently near 0 or 1.
 *  3. Convert final x_hat to a 0-1 solution with a user-defined cutoff.
 *  4. Recompute usage for that 0-1 solution, then repair if infeasible.
 *  5. Evaluate final solution.
 *
 * @param prob          Pointer to the MKP instance.
 * @param lambda        Penalty coefficient for constraints.
 * @param learning_rate The step size for gradient updates.
 * @param max_no_improvement The number of iterations without improvement before stopping.
 * @param out_sol       The output solution.
 * @param verbose       The verbosity level (NONE, INFO, DEBUG).
 * @param start         The start time for time limit.
 * @param max_time      The maximum allowed time.
 */
void gradient_solver(const Problem *prob,
                     float lambda,
                     float learning_rate,
                     int max_no_improvement,
                     Solution *out_sol,
                     LogLevel verbose,
                     clock_t start,
                     float max_time);

#endif // GRADESC_H

