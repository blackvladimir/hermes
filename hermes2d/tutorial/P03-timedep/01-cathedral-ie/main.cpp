#define HERMES_REPORT_ALL
#define HERMES_REPORT_FILE "application.log"
#include "hermes2d.h"
#include "runge_kutta.h"

//  This example shows the simplest way to solve a linear time-dependent
//  PDE in Hermes using the implicit Euler method in time. The model describes 
//  in a naive approximation how the St. Vitus Cathedral in Prague 
//  (http://en.wikipedia.org/wiki/St._Vitus_Cathedral) responds to changes 
//  in the surrounding air temperature during one 24-hour cycle. 
//
//  PDE: non-stationary heat transfer equation
//  dT/dt - LAMBDA / (HEATCAP * RHO) * Laplace T = 0.
//
//  Domain: St. Vitus cathedral (file cathedral.mesh).
//
//  IC:  T = TEMP_INIT.
//  BC:  T = TEMP_INIT on the bottom edge ... Dirichlet,
//       LAMBDA * dT/dn = ALPHA*(t_exterior(time) - T) ... Newton, time-dependent.
//
//  Time-stepping: implicit Euler method.
//
//  The following parameters can be changed:

const int P_INIT = 2;                             // Polynomial degree of all mesh elements.
const int INIT_REF_NUM = 1;                       // Number of initial uniform mesh refinements.
const int INIT_REF_NUM_BDY = 3;                   // Number of initial uniform mesh refinements towards the boundary.
const double time_step = 300.0;                   // Time step in seconds.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
                                                  // SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.

// Boundary markers.
const std::string BDY_GROUND = "Boundary ground";
const std::string BDY_AIR = "Boundary air";

// Problem parameters.
const double TEMP_INIT = 10;       // Temperature of the ground (also initial temperature).
const double ALPHA = 10;           // Heat flux coefficient for Newton's boundary condition.
const double LAMBDA = 1e2;         // Thermal conductivity of the material.
const double HEATCAP = 1e2;        // Heat capacity.
const double RHO = 3000;           // Material density.
const double T_FINAL = 86400;      // Length of time interval (24 hours) in seconds.

// Weak forms.
#include "definitions.cpp"

int main(int argc, char* argv[])
{
  // Load the mesh.
  Mesh mesh;
  H2DReader mloader;
  mloader.load("cathedral.mesh", &mesh);

  // Perform initial mesh refinements.
  for(int i = 0; i < INIT_REF_NUM; i++) mesh.refine_all_elements();
  mesh.refine_towards_boundary(BDY_AIR, INIT_REF_NUM_BDY);
  mesh.refine_towards_boundary(BDY_GROUND, INIT_REF_NUM_BDY);

  // Previous time level solution (initialized by the external temperature).
  Solution tsln(&mesh, TEMP_INIT);

  // Initialize the weak formulation.
  double current_time = 0;
  CustomWeakFormHeatRK1 wf(BDY_AIR, ALPHA, LAMBDA, HEATCAP, RHO, time_step, 
                           &current_time, TEMP_INIT, T_FINAL, &tsln);
  
  // Initialize boundary conditions.
  DefaultEssentialBCConst bc_essential(BDY_GROUND, TEMP_INIT);
  EssentialBCs bcs(&bc_essential);

  // Create an H1 space with default shapeset.
  H1Space space(&mesh, &bcs, P_INIT);
  int ndof = space.get_num_dofs();
  info("ndof = %d", ndof);
 
  // Initialize the FE problem.
  bool is_linear = true;
  DiscreteProblem dp(&wf, &space, is_linear);

  // Set up the solver, matrix, and rhs according to the solver selection.
  SparseMatrix* matrix = create_matrix(matrix_solver);
  Vector* rhs = create_vector(matrix_solver);
  Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);
  solver->set_factorization_scheme(HERMES_REUSE_FACTORIZATION_COMPLETELY);

  // Initialize views.
  ScalarView Tview("Temperature", new WinGeom(0, 0, 450, 600));
  Tview.set_min_max_range(0,20);
  Tview.fix_scale_width(30);

  // Time stepping:
  int ts = 1;
  do 
  {
    info("---- Time step %d, time %3.5f s", ts, current_time);

    // First time assemble both the stiffness matrix and right-hand side vector,
    // then just the right-hand side vector.
    wf.set_current_time(current_time);
    if (ts == 1) {
      info("Assembling the stiffness matrix and right-hand side vector.");
      dp.assemble(matrix, rhs);
    }
    else {
      info("Assembling the right-hand side vector (only).");
      dp.assemble(NULL, rhs);
    }

    // Solve the linear system and if successful, obtain the solution.
    info("Solving the matrix problem.");
    if(solver->solve()) Solution::vector_to_solution(solver->get_solution(), 
                                                     &space, &tsln);
    else error ("Matrix solver failed.\n");

    // Visualize the solution.
    char title[100];
    sprintf(title, "Time %3.2f s", current_time);
    Tview.set_title(title);
    Tview.show(&tsln);

    // Increase current time and time step counter.
    current_time += time_step;
    ts++;
  }
  while (current_time < T_FINAL);

  // Wait for the view to be closed.
  View::wait();
  return 0;
}
