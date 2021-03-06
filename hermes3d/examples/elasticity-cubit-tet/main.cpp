#define HERMES_REPORT_ALL
#include "config.h"
#include <hermes3d.h>

// This example shows how to solve a linear elasticity problem using 
// a tetrahedral mesh in Exodus format.
//
// PDE: Lame equations of linear elasticity.
//
// BC: u_x = u_y = 0 
//     du_z/dn = f on top of the L-beam
//     du_x/dn = du_y/dn = du_z/dn = 0 elsewhere
//
// The following parameters can be changed:

const int P_INIT = 2;                             // Initial polynomial degree of all mesh elements.
bool solution_output = true;                      // Generate output files (if true).
const char* iterative_method = "bicgstab";        // Name of the iterative method employed by AztecOO (ignored
                                                  // by the other solvers). 
                                                  // Possibilities: gmres, cg, cgs, tfqmr, bicgstab.
const char* preconditioner = "jacobi";            // Name of the preconditioner employed by AztecOO (ignored by
                                                  // the other solvers). 
                                                  // Possibilities: none, jacobi, neumann, least-squares, or a
                                                  // preconditioner from IFPACK (see solver/aztecoo.h).
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
                                                  // SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.

// Problem parameters. 
const double E  = 200e9; 		// Young modulus for steel: 200 GPa.
const double nu = 0.3;			// Poisson ratio. 
const double f_x  = 0;   		// x-direction load force (N). 
const double f_y  = 1e4;   		// y-direction load force (N). 
const double f_z  = 0;   		// z-direction load force (N). 
const double lambda = (E * nu) / ((1 + nu) * (1 - 2*nu));
const double mu = E / (2*(1 + nu));

// Boundary markers.
int bdy_fixed = 1;
int bdy_force = 2;

// Boundary condition types. 
BCType bc_types_x(int marker) 
{
  return (marker == bdy_fixed) ? BC_ESSENTIAL : BC_NATURAL;
}

BCType bc_types_y(int marker) 
{
  return (marker == bdy_fixed) ? BC_ESSENTIAL : BC_NATURAL;
}

BCType bc_types_z(int marker) 
{
  return (marker == bdy_fixed) ? BC_ESSENTIAL : BC_NATURAL;
}

// Essential (Dirichlet) boundary condition values. 
scalar essential_bc_values(int ess_bdy_marker, double x, double y, double z)
{
  return 0;
}

#include "definitions.cpp"

int main(int argc, char **args) 
{
  // Time measurement.
  TimePeriod cpu_time;
  cpu_time.tick();

  // Load the mesh. 
  Mesh mesh;
  ExodusIIReader mloader;
  mloader.load("brick_with_holes_tet.e", &mesh);

  // Create H1 space with default shapeset for x-displacement component. 
  H1Space xdisp(&mesh, bc_types_x, essential_bc_values, Ord3(P_INIT));
  
  // Create H1 space with default shapeset for y-displacement component. 
  H1Space ydisp(&mesh, bc_types_y, essential_bc_values, Ord3(P_INIT));
  
  // Create H1 space with default shapeset for z-displacement component. 
  H1Space zdisp(&mesh, bc_types_z, essential_bc_values, Ord3(P_INIT));
  
  // Initialize weak formulation.
  WeakForm wf(3);
  wf.add_matrix_form(0, 0, callback(bilinear_form_0_0), HERMES_SYM);
  wf.add_matrix_form(0, 1, callback(bilinear_form_0_1), HERMES_SYM);
  wf.add_matrix_form(0, 2, callback(bilinear_form_0_2), HERMES_SYM);
  wf.add_vector_form_surf(0, callback(surf_linear_form_x), bdy_force);

  wf.add_matrix_form(1, 1, callback(bilinear_form_1_1), HERMES_SYM);
  wf.add_matrix_form(1, 2, callback(bilinear_form_1_2), HERMES_SYM);
  wf.add_vector_form_surf(1, callback(surf_linear_form_y), bdy_force);

  wf.add_matrix_form(2, 2, callback(bilinear_form_2_2), HERMES_SYM);
  wf.add_vector_form_surf(2, callback(surf_linear_form_z), bdy_force);

  // Initialize discrete problem.
  bool is_linear = true;
  DiscreteProblem dp(&wf, Hermes::vector<Space *>(&xdisp, &ydisp, &zdisp), is_linear);

  // Set up the solver, matrix, and rhs according to the solver selection.
  SparseMatrix* matrix = create_matrix(matrix_solver);
  Vector* rhs = create_vector(matrix_solver);
  Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);

  // Initialize the preconditioner in the case of SOLVER_AZTECOO.
  if (matrix_solver == SOLVER_AZTECOO) 
  {
    ((AztecOOSolver*) solver)->set_solver(iterative_method);
    ((AztecOOSolver*) solver)->set_precond(preconditioner);
    // Using default iteration parameters (see solver/aztecoo.h).
  }

  // Assemble stiffness matrix and load vector.
  info("Assembling the linear problem (ndof: %d).", Space::get_num_dofs(Hermes::vector<Space *>(&xdisp, &ydisp, &zdisp)));
  dp.assemble(matrix, rhs);

  // Solve the linear system. If successful, obtain the solution.
  info("Solving the linear problem.");
  Solution xsln(xdisp.get_mesh());
  Solution ysln(ydisp.get_mesh());
  Solution zsln(zdisp.get_mesh());
  if(solver->solve()) Solution::vector_to_solutions(solver->get_solution(), 
                      Hermes::vector<Space *>(&xdisp, &ydisp, &zdisp), Hermes::vector<Solution *>(&xsln, &ysln, &zsln));
  else error ("Matrix solver failed.\n");

  // Output all components of the solution.
  if (solution_output) out_fn_vtk(&xsln, &ysln, &zsln, "sln");
  
  // Time measurement.
  cpu_time.tick();

  // Print timing information.
  info("Solutions saved. Total running time: %g s.", cpu_time.accumulated());

  // Clean up.
  delete matrix;
  delete rhs;
  delete solver;
  
  return 0;
}
