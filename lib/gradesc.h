#ifndef GRADESC_H
#define GRADESC_H

#include "data_structure.h"


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
 * @param max_iters     Number of gradient descent iterations.
 * @param out_sol       Output solution (allocated by caller).
 */
void gradient_solver(const Problem *prob,
                     float lambda,
                     float learning_rate,
                     int max_iters,
                     Solution *out_sol);

#endif // GRADESC_H

