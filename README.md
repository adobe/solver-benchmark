# Linear Solver Benchmark

This library is a benchmark for implementations of sparse cholesky factorization methods.

## Table of Contents
- [Linear Solver Benchmark](#linear-solver-benchmark)
  - [Table of Contents](#table-of-contents)
  - [Solvers](#solvers)
  - [Compilation](#compilation)
  - [Adding New Test Data](#adding-new-test-data)
  - [Running The Benchmark](#running-the-benchmark)
  - [Generating Interactive Altair Plot](#generating-interactive-altair-plot)
  - [Benchmark Structure](#benchmark-structure)
  - [Additional Reference Documentation](#additional-reference-documentation)
  - [Contributing](#contributing)
  - [Licensing](#licensing)


## Solvers
The following solvers are benchmarked:

- [Eigen::SimplicialLDLT](https://eigen.tuxfamily.org/dox/classEigen_1_1SimplicialLDLT.html)
- [Eigen::CholmodSupernodalLLT](https://eigen.tuxfamily.org/dox/classEigen_1_1CholmodSupernodalLLT.html)
- [Eigen::CholmodSimplicialLLT](https://eigen.tuxfamily.org/dox/classEigen_1_1CholmodSimplicialLLT.html)
- [Eigen::AccelerateLLT](https://eigen.tuxfamily.org/dox/classAccelerateLLT.html) Apple Silicon only
- [Eigen::AccelerateLDLT](https://eigen.tuxfamily.org/dox/classAccelerateLDLT.html) Apple Silicon only
- [Sympiler](https://www.sympiler.com/) Apple only
- [Eigen::PardisoLLT](https://eigen.tuxfamily.org/dox/classEigen_1_1PardisoLLT.html) Windows only

All solvers are accessed using the [Polysolve](https://github.com/polyfem/polysolve) wrapper library.

## Compilation

All compile dependencies are handled by CMake, so the following should work on any platform:

```bash
cd <repo>
mkdir build
cd build
cmake ..
cmake --build . -j 8
```

Before the benchmark can be ran, there must be linear systems to benchmark on in the `/data` directory. These systems are stored in a compressed json `.zst` format.

## Adding New Test Data

To save matrices in the correct format, perform the following:

1. Download the [save_problem.h](https://gist.github.com/jdumas/4a0b9bbf5a25af5004f51ee78ffe1af9) header file, and add it to your application. It only depends on Eigen and the STL.

2. Save the linear system within your application
    ```c++
    #include "save_problem.h"

    // Fill matrices for linear system A*x = b
    Eigen::SparseMatrix<double> A;
    Eigen::MatrixXd b;

    // A = ...
    // b = ...

    // Describe metadata about the problem
    benchy::io::Problem<double> problem;
    problem.A = A;
    problem.b = b;
    problem.is_symmetric_positive_definite = 1;
    problem.is_sequence_of_problems = 0;
    problem.dimension = 3;
    problem.description = "Linear elasticity simulation in 3D";
    problem.project_url = "https://github.com/polyfem/polyfem/";
    problem.contact_email = "my.name@gmail.com";

    // Save the problem to a json file
    benchy::io::save_problem("my_problem.json", problem);
    ```
    If you have multiple rhs, save them as multiple columns of a single rhs matrix `b.mtx`.

2. Compile and run our utility to convert and compress the data
    ```
    <build>/tools/benchy_convert my_problem.json my_problem.zst
    ```
    **Tip**: For faster compilation, compile the tool only with `cmake -DBENCHY_TOOLS=ON -DBENCHY_TESTS=OFF ..`.

3. Copy the compressed linear system to the corresponding problem folder in `data/`.
    ```
    cp my_problem.zst <repo>/data/my_project
    ```

## Running The Benchmark

To run the benchmark, first ensure there are systems in the execute
```c++
./build/tools/benchmark_cli
```
The benchmark command-line interface exposes three parameters:

1. `--input` A directory to the dataset to be benchmarked on. Defaults to `./data`. See [Adding New Test Data](#adding-new-test-data)
2. `--regex` The paths of all `.zst` files in the input directory are collected and then filtered using the regex. For example, to access only the systems in the `harmonic` subdirectory, use `./build/tools/benchmark_cli --regex '.*/harmonic/.*'`. Defaults to `.*.zst`
3. `--output` Directory to output the benchmark csv data to. Defaults to `./output`.

Depending on the number of systems and solvers, the benchmark could take a long time to run.

## Generating Interactive Altair Plot

Benchy comes with an interactive plot, written with [Altair](https://altair-viz.github.io/index.html), that can be used to explore the results of the benchmark.

After running, the benchmark should output a file, `output/<date>_<time>_benchmark_data.csv`. To generate the interactive plot, execute the following from the solver-benchmark directory:
```bash
conda env create -f scripts/benchy-analysis.yml
conda activate benchy-analysis
python scripts/analysis.py --input ./path/to/benchmark_data.csv
```
This should output a file `polars-composite.html` which can be viewed in a browser.

## Benchmark Structure

For each solver listed above, the benchmark times the following:
- Analyze phase: symbolic factorization of $A$
- Factorize phase: finding $L$ such that $A = LL^T$
- Solve phase: Solving $Ax = b$ using the factorization

The rough structure of the benchmark is as follows:
```cpp
for (Each System in Systems)
{
    for (Each Solver)
    {
        for (Each Phase : [Analyze, Factorize, Solve])
        {
            time Solver Phase for System
        }
    }
}
```

The main dependency of the project is [Celero](https://github.com/DigitalInBlue/Celero). Celero provides the ability to consistently time pieces of code. Celero actually runs each benchmark multiple times to gather reliable data. See [Celero Program Flow](https://github.com/DigitalInBlue/Celero#general-program-flow) for more details.

## Additional Reference Documentation

To contribute a solver to the benchmark, a `Polysolve::LinearSolver` wrapper subclass must be written. See the reference documentation for more details on the internals of the benchmark. To build documentation, complete the following:
1. Configure cmake with the `BENCHY_DOCS` on, by e.g
```sh
mkdir build
cd build
cmake -DBENCHY_DOCS=ON ..
```
2. Build the documentation
```sh
cmake --build . --target doc -j8
```
3. View the `index.html` file
```sh
firefox html/index.html
```

## Contributing

Contributions are welcomed! Read the [Contributing Guide](./.github/CONTRIBUTING.md) for more information.

## Licensing

This project is licensed under the Apache 2.0 License. See [LICENSE](LICENSE) for more information.
