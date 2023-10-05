# Linear Solver Benchmark Documentation 

Here you'll find the Doxygen-generated documentation for the Solver Benchmark library. For an overview of the project, consult the main [README](https://git.corp.adobe.com/noether/solver-benchmark) in the project github repository. 

The important sections of the code are made up of two modules.

## Benchmark module

This module contains code for running the linear solver benchmarks. The main class, `SolverFixture`, is in `benchmark.h` and is a subclass of Celero's `TestFixture` class. This class contains all the benchmarks for each phase of computation of every solver.

All solvers are contained in the `solver_structs.h` file. They are all implement the `Polysolve::LinearSolver` interface. To add a new solver to the benchmark, first implement the `LinearSolver` interface from [Polysolve](https://github.com/polyfem/polysolve/blob/main/src/polysolve/LinearSolver.hpp) and then create a struct with a `create` method. Lastly, add the new benchmarks to the bottom of `benchmark.cpp`

## io module

This handles the input and output of linear systems into the benchmark, as well as the `Benchy::io::Problem` format struct in `save_problem.h`.