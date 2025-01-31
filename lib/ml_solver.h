#ifndef ML_SOLVER_H
#define ML_SOLVER_H

#include "data_structure.h"


/**
 * @brief Uses gradient descent on a continuous relaxation of the MKP
 *        to produce a 0-1 solution (via sigmoid + penalty).
 *
 * @param prob         The MKP instance
 * @param lambda       Penalty coefficient (controls constraint violation cost)
 * @param learning_rate Gradient descent step size
 * @param max_iters    Number of gradient updates
 * @param out_sol      Output solution (allocated by caller)
 */
void gradient_solver(const Problem *prob,
                     float lambda,
                     float learning_rate,
                     int max_iters,
                     Solution *out_sol);

#endif // ML_SOLVER_H

