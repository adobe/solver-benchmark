#
# Copyright 2023 Adobe. All rights reserved.
# This file is licensed to you under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
# OF ANY KIND, either express or implied. See the License for the specific language
# governing permissions and limitations under the License.
#
from argparse import ArgumentParser, BooleanOptionalAction
import polars as pl
import altair as alt
import time
import logging
import math


def default_args():
    default_results = ""
    default_out = "../output/"

    return default_results, default_out


def get_args():
    (
        def_results,
        def_out,
    ) = default_args()

    parser = ArgumentParser()
    parser.add_argument(
        "-i",
        "--input",
        default=def_results,
        type=str,
        help="Input celero CSV data file, or files if comparing runs",
        nargs="*",
    )
    parser.add_argument(
        "-n",
        "--names",
        type=str,
        help="(Optional) names of benchmark runs, if comparing multiple runs",
        nargs="*",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=def_out,
        type=str,
        help="Directory to output html figure to",
    )
    parser.add_argument(
        "-d",
        "--debug",
        help="Prints csvs showing failures. Sets logging output to debug (very verbose)",
        action="store_const",
        dest="loglevel",
        const=logging.DEBUG,
        default=logging.WARNING,
    )
    parser.add_argument(
        "-v",
        "--verbose",
        help="Sets logging output to info (verbose)",
        action="store_const",
        dest="loglevel",
        const=logging.INFO,
    )

    return parser.parse_args()


def load_dataframe(benchmark_csv):
    """Loads csv file from Celero into polars dataframe

    Args:
        benchmark_csv: Path to csv file from Celero benchmark
    Outputs:
        A polars dataframe with a subset of the columns of the csv
    """
    cols_to_load = [
        "Group",
        "Experiment",
        "Problem Space",
        "Iterations/sec",
        "us/Iteration",
        "Numerical Failure Mean",
        "Residual Mean",
        "R Mean (us)",
        "Physical Memory (b) Mean",
        "System Name",
        "Dataset",
        "Size",
    ]
    dtypes_to_load = [
        pl.Utf8,
        pl.Utf8,
        pl.Int64,
        pl.Float32,
        pl.Float32,
        pl.Float32,
        pl.Float32,
        pl.Float32,
        pl.Float32,
        pl.Utf8,
        pl.Utf8,
        pl.Int64,
    ]

    df = pl.read_csv(benchmark_csv, columns=cols_to_load, dtypes=dtypes_to_load)
    logging.info(f"Loaded {benchmark_csv} with {df.select(pl.count()).item()} rows")

    return df


def write_nan_inf(df):
    """Writes columns contain a nan or an inf to a csv file

    This function is only called if the script is run with the --debug flag
    Args:
        df: Benchmark dataframe
    """
    rows_nan = df.filter(pl.any(pl.col(pl.NUMERIC_DTYPES).is_nan()))
    rows_inf = df.filter(pl.any(pl.col(pl.NUMERIC_DTYPES).is_infinite()))
    rows_inf_nan = pl.concat([rows_nan, rows_inf])
    if rows_inf_nan.is_empty():
        logging.debug(f"No inf/nan found. nan_inf.csv not written")
    else:
        logging.debug(f"Writing inf/nan to {output + name}_nan_inf.csv")
        rows_inf_nan.write_csv(output + name + "_nan_inf.csv")


def write_failures(df):
    """Writes columns of df that contain a failure to a csv file

    This function is only called if the script is run with the --debug flag
    Args:
        df: Benchmark dataframe
    """
    logging.debug(f"Writing failures to {output + name}_failures.csv")
    rows_failed = df.filter(pl.col("Failed"))
    if rows_failed.is_empty():
        logging.debug("No failures found. failures.csv not written")
    else:
        logging.debug(f"Writing failure rows to {output + name}_failures.csv")
        rows_failed.write_csv(output + name + "_failures.csv")


def remove_nan_inf(df):
    """Removes systems for which any solver gives a NaN or an Inf in Any numeric field

    If any solver gave an inf or NaN value on a system, all rows containing that system
    are removed. This removal is for the sake of fair comparison on visualization. These
    NaN or inf systems can be written to a csv by calling the script with the --debug
    flag.

    Args:
        df: Benchmark dataframe
    Returns:
        A filtered dataframe, with rows pertaining to solvers that had nan or inf
        removed
    """
    systems_nan_value = (
        df.groupby("System Name")
        .agg(pl.col(pl.NUMERIC_DTYPES).is_nan().any())
        .with_columns(
            pl.sum_horizontal(pl.col(pl.Boolean)).cast(pl.Boolean).alias("is_nan")
        )
        .select(["System Name", "is_nan"])
    )
    systems_inf_value = (
        df.groupby("System Name")
        .agg(pl.col(pl.NUMERIC_DTYPES).is_infinite().any())
        .with_columns(
            pl.sum_horizontal(pl.col(pl.Boolean)).cast(pl.Boolean).alias("is_infinite")
        )
        .select(["System Name", "is_infinite"])
    )
    systems_not_nan_and_not_inf = (
        systems_nan_value.join(systems_inf_value, on="System Name")
        .with_columns(
            (pl.col("is_nan") | pl.col("is_infinite")).alias("is_nan or is_infinite")
        )
        .filter(~pl.col("is_nan or is_infinite"))
        .select("System Name")
        .to_series()
    )
    return df.filter(pl.col("System Name").is_in(systems_not_nan_and_not_inf))


def remove_failures(df):
    """Removes systems for which any solver failed

    If any solver failed on a system, this method removes all rows containing that
    system are removed. This removal is for the sake of fair comparison on
    visualization. The solver which failed on a system can be written to a csv by
    calling the script with the --debug flag

    Args:
        df: Benchmark dataframe
    Returns:
        A filtered dataframe, with rows pertaining to solvers that had failures removed
    """
    logging.debug("Removing failures for dataframe")
    systems_succeeded = (
        df.groupby(["System Name"])
        .agg(pl.col("Failed").any())
        .filter(~pl.col("Failed"))
        .select("System Name")
        .to_series()
    )
    return df.filter(pl.col("System Name").is_in(systems_succeeded))


def create_all_phase(df):
    """Creates a ficticious "all" phase of computation in dataframe.

    The benchmark gets timing data for the Analyze, Factorize, and Solve phases of the
    solver computation seperately. This method adds rows to the dataframe that are
    aggregated over these phases, where the timing in the "all" phase is the sum of
    times in the analyze, factorize, and solve phases.

    Args:
        df: Benchmark dataframe
    Returns:
        A dataframe with rows added containing the "all" phase, with info aggregated
        over analyze, factorize, and solve phases.
    """
    df_all = (
        df.groupby(["Solver", "System Name"])
        .agg(
            pl.col("us/Iteration").sum(),
            pl.col("Virtual Memory (b)").sum(),
            pl.col("Physical Memory (b)").sum(),
            pl.col("Dataset").first(),
            pl.col("#Nonzeros").first(),
            pl.col("Failed").any(),
            pl.col("s/Iteration").sum(),
            pl.col("Residual").max(),
        )
        .with_columns(
            pl.lit("All").alias("Phase"),
            (pl.col("s/Iteration") / pl.col("#Nonzeros")).alias("Normalized Time"),
        )
    ).select(
        [
            "Phase",
            "Solver",
            "us/Iteration",
            "Virtual Memory (b)",
            "Physical Memory (b)",
            "System Name",
            "Dataset",
            "#Nonzeros",
            "Failed",
            "Normalized Time",
            "s/Iteration",
            "Residual",
        ]
    )
    return pl.concat([df, df_all])


def clean_dataframe(df, name=None, output=None):
    """Cleans benchmark dataframe

    Performs data cleaning operations on dataframe from celero benchmark. If any solver
    failed on a system, that system is removed entirely from the dataframe, in order to
    have a fair comparison when aggregating over all systems. Similarly, systems where
    a solver gave an inf or nan value are removed entirely.

    If the script is run with --debug, then this method writes two csvs: failures.csv
    and nan_inf.csv. failures.csv contains which solvers failed on which systems, while
    nan_inf.csv contains which solvers gave nan/inf values. If none are found then they
    aren't written.

    Args:
        df: Dataframe containing benchmark data
        name: (Optional) name of benchmark run
        output: (Optional) destination to output debug csv files

    Returns:
        A dataframe where each row represents the benchmark data associated with one
        solver, one phase, and one system. Format for a row is: (Phase, Solver,
        us/Iteration, [Physical Memory (b), Virtual Memory (b)], Residual, System,
        Dataset, #Nonzeros, Failed, Normalized Time)
    """
    logging.info("Cleaning dataframe")

    # Cosmetic renaming from Celero defaults to sensible names
    df = df.rename(
        {
            "Experiment": "Solver",
            "Group": "Phase",
            "Size": "#Nonzeros",
            "Residual Mean": "Residual",
            "R Mean (us)": "Virtual Memory (b)",
            "Physical Memory (b) Mean": "Physical Memory (b)",
        }
    )

    # Benchmark failure is stored as an int, this converts to boolean
    logging.debug("Converting failure column to boolean")
    df = df.with_columns(
        pl.col("Numerical Failure Mean").cast(pl.Boolean).alias("Failed")
    )

    if logging.getLogger().getEffectiveLevel() == logging.DEBUG:
        write_nan_inf(df)

    if logging.getLogger().getEffectiveLevel() == logging.DEBUG:
        write_failures(df)

    df = remove_nan_inf(df)

    # Dropping extraneous columns
    df = df.drop(["Numerical Failure Mean", "Problem Space", "Iterations/sec"])

    # Creates time per nonzero column, and converts microseconds to seconds
    df = df.with_columns(
        (pl.col("us/Iteration") / pl.col("#Nonzeros")).alias("Normalized Time"),
        (pl.col("us/Iteration") / 1e6).alias("s/Iteration"),
    )

    # Removes extraneous baseline data
    df = df.filter(pl.col("Solver") != "Base")

    df = remove_failures(df)

    # The residual is only defined in the solve phase. This adds the residual
    # from the solve phase to the analyze and factorize phases.
    df_residual = df.groupby(["Solver", "System Name"]).agg(pl.col("Residual").max())
    df = df.join(df_residual, on=["Solver", "System Name"])
    df = df.drop(["Residual"])
    df = df.rename({"Residual_right": "Residual"})

    df = create_all_phase(df)
    df = df.rename(
        {"Normalized Time": "s/Iteration per Nonzero", "#Nonzeros": "Nonzeros"}
    )

    return df


def combine_df_runs(dfs, names):
    """Combines dataframes across multiple benchmark runs into one

    Args:
        dfs: List of polars dataframes
        names: List of benchmark run names

    Returns:
        A new dataframe with a "Run Name" column, where the rows from dfs[i] have "Run
        Name" equal to names[i]
    """

    # Black formatter makes this look uglier than it needs to
    assert len(dfs) == len(
        names
    ), f"#Of csvs = {len(dfs)}, does not # of names = {len(names)}"

    df_concat = pl.DataFrame()
    for pair in zip(dfs, names):
        df = pair[0]
        name = pair[1]
        df = df.with_columns(pl.lit(name).alias("Run Name"))
        df_concat = pl.concat([df_concat, df])

    return df_concat


def composite_chart(df, num_runs):
    """Generates interactive plot of benchmark data

    The scatterplot plots size vs time-to-solve for each system and each solver. The
    time is summed over all computation phases, and failures are removed.

    Additionally, a barchart of timing data aggregated over all systems for each solver
    is created. Similar plots for residuals and for memory usage data are made.

    A number of interactive features are added. This allows the user to select various
    subsets of the data to analyze, and have that propogate into the plots. See
    https://git.corp.adobe.com/pages/noether/solver-benchmark-data/ for a demonstration
    of the interactive features.

    Args:
        df: Clean dataframe from celero benchmark

    Returns:
        Altair chart object
    """
    logging.info("Creating plot")

    # Option to select y-axis
    time_dropdown = alt.binding_select(
        options=[
            "s/Iteration",
            "s/Iteration per Nonzero",
        ],
        name="Y-axis column ",
    )
    ycol_param = alt.param(value="s/Iteration", bind=time_dropdown)

    # Option to select dataset to plot
    datasets = df["Dataset"].unique().to_list()
    options = datasets
    labels = [option + " " for option in options]
    input_dropdown = alt.binding_select(
        options=[None] + options, labels=["All"] + labels, name="Dataset: "
    )
    dropdown_selection = alt.selection_point(fields=["Dataset"], bind=input_dropdown)

    # Selection for interactive legend to choose solver
    selection = alt.selection_point(fields=["Solver"], bind="legend")
    custom_scale = alt.Scale(domain=df.select("Solver").unique().to_series().to_list())

    # Brush to highlight a selection of scatterplot points
    brush = alt.selection_interval(encodings=["x"])

    # Slider to select residual cutoff
    residual_slider = alt.binding_range(
        min=math.ceil(
            math.log10(
                df.filter(pl.col("Phase") == "All").select("Residual").min().item()
            )
        ),
        max=math.floor(
            math.log10(
                df.filter(pl.col("Phase") == "All").select("Residual").max().item()
            )
        ),
        step=1,
        name="Residual Cutoff (Log): ",
    )
    residual_param = alt.param(value=0, bind=residual_slider)

    # Slider to select phase of computation
    phase_dropdown = alt.binding_select(
        options=[
            "All",
            "Analyze",
            "Factorize",
            "Solve",
        ],
        name="Phase: ",
    )
    phase_param = alt.param(value="All", bind=phase_dropdown)

    # Dropdown to select which run to display
    if num_runs > 1:
        runs = df.select("Run Name").unique().to_series().to_list()
        run_radio = alt.binding_radio(
            options=runs,
            name="Benchmark Run: ",
        )
        run_param = alt.param(value=runs[0], bind=run_radio)

    scatterplot = (
        alt.Chart(df)
        .mark_point(size=50, opacity=0.7)
        .encode(
            # x=alt.X("Nonzeros:Q").scale(type="log").title("Log(Nonzeros)"),
            x=alt.X("Nonzeros:Q").scale(type="log").title("Log(#Nonzeros)"),
            y=alt.Y("y:Q")
            # .title("Dropdown Menu -- Log(Time) or Log(Normalized Time)")
            .title("Log(seconds)").scale(type="log"),
            color=alt.condition(
                brush,
                alt.Color("Solver:N", scale=custom_scale),
                alt.value("lightgray"),
            ),
            shape="Dataset:N",
            tooltip=["Solver:N", "Nonzeros:Q", "Residual:Q", "System Name:N"],
        )
        .transform_calculate(y=f"datum[{ycol_param.name}]")
        .add_params(
            selection,
            dropdown_selection,
            ycol_param,
            brush,
            residual_param,
            phase_param,
        )
        # 0.434=log_10(e) is for change of log, since I want log_10 but can only take natural log
        .transform_filter(0.434 * alt.expr.log(alt.datum.Residual) < residual_param)
        .transform_filter(selection)
        .transform_filter(dropdown_selection)
        .transform_filter(alt.FieldEqualPredicate(field="Phase", equal=phase_param))
        .properties(
            # title="Benchmark Scatterplot: Log Total Time Per System vs Log Nonzeros",
            title="Benchmark Scatterplot: Log Total Time vs Log Nonzeros",
            width=800,
            height=400,
        )
    )

    barchart_timing = (
        alt.Chart(df)
        .mark_bar()
        .encode(
            x=alt.X("sum(s/Iteration):Q"),
            y=alt.Y("Solver:O").sort("-x").title(""),
            color=alt.Color("Solver:N", scale=custom_scale),
        )
        .transform_filter(brush)
        .transform_filter(0.434 * alt.expr.log(alt.datum.Residual) < residual_param)
        .transform_filter(selection)
        .transform_filter(dropdown_selection)
        .transform_filter(alt.FieldEqualPredicate(field="Phase", equal=phase_param))
        .properties(width=200, height=80)
    )

    barchart_residual = (
        alt.Chart(df)
        .mark_bar()
        .encode(
            x=alt.X("sum(Residual):Q"),
            y=alt.Y("Solver:O").sort("-x").title(""),
            color=alt.Color("Solver:N", scale=custom_scale),
        )
        .transform_filter(brush)
        .transform_filter(0.434 * alt.expr.log(alt.datum.Residual) < residual_param)
        .transform_filter(selection)
        .transform_filter(dropdown_selection)
        .transform_filter(alt.FieldEqualPredicate(field="Phase", equal=phase_param))
        .properties(width=200, height=80)
    )

    barchart_memory = (
        alt.Chart(df)
        .mark_bar()
        .encode(
            x=alt.X(field="Physical Memory (b)", aggregate="sum", type="quantitative"),
            y=alt.Y("Solver:O").sort("-x").title(""),
            color=alt.Color("Solver:N", scale=custom_scale),
        )
        .transform_filter(brush)
        .transform_filter(0.434 * alt.expr.log(alt.datum.Residual) < residual_param)
        .transform_filter(selection)
        .transform_filter(dropdown_selection)
        .transform_filter(alt.FieldEqualPredicate(field="Phase", equal=phase_param))
        .properties(width=200, height=80)
    )

    boxplot_timing = (
        alt.Chart(df)
        .mark_boxplot()
        .encode(
            x=alt.X("s/Iteration:Q"),
            y=alt.Y("Solver:O").title(""),
            color=alt.Color("Solver:N", scale=custom_scale),
        )
        .transform_filter(brush)
        .transform_filter(0.434 * alt.expr.log(alt.datum.Residual) < residual_param)
        .transform_filter(selection)
        .transform_filter(dropdown_selection)
        .transform_filter(alt.FieldEqualPredicate(field="Phase", equal=phase_param))
        .properties(width=200, height=80)
    )

    boxplot_residual = (
        alt.Chart(df)
        .mark_boxplot()
        .encode(
            x=alt.X("Residual:Q"),
            y=alt.Y("Solver:O").title(""),
            color=alt.Color("Solver:N", scale=custom_scale),
        )
        .transform_filter(brush)
        .transform_filter(0.434 * alt.expr.log(alt.datum.Residual) < residual_param)
        .transform_filter(selection)
        .transform_filter(dropdown_selection)
        .transform_filter(alt.FieldEqualPredicate(field="Phase", equal=phase_param))
        .properties(width=200, height=80)
    )

    boxplot_memory = (
        alt.Chart(df)
        .mark_boxplot()
        .encode(
            x=alt.X(field="Physical Memory (b)", type="quantitative"),
            y=alt.Y("Solver:O").title(""),
            color=alt.Color("Solver:N", scale=custom_scale),
        )
        .transform_filter(brush)
        .transform_filter(0.434 * alt.expr.log(alt.datum.Residual) < residual_param)
        .transform_filter(selection)
        .transform_filter(dropdown_selection)
        .transform_filter(alt.FieldEqualPredicate(field="Phase", equal=phase_param))
        .properties(width=200, height=80)
    )

    if num_runs > 1:
        runs = df.select("Run Name").unique().to_series().to_list()
        run_radio = alt.binding_radio(
            options=runs,
            name="Benchmark Run: ",
        )
        run_param = alt.param(value=runs[0], bind=run_radio)

        scatterplot = scatterplot.add_params(run_param).transform_filter(
            alt.FieldEqualPredicate(field="Run Name", equal=run_param)
        )
        barchart_timing = barchart_timing.transform_filter(
            alt.FieldEqualPredicate(field="Run Name", equal=run_param)
        )
        barchart_residual = barchart_residual.transform_filter(
            alt.FieldEqualPredicate(field="Run Name", equal=run_param)
        )
        barchart_memory = barchart_memory.transform_filter(
            alt.FieldEqualPredicate(field="Run Name", equal=run_param)
        )
        boxplot_timing = boxplot_timing.transform_filter(
            alt.FieldEqualPredicate(field="Run Name", equal=run_param)
        )
        boxplot_residual = boxplot_residual.transform_filter(
            alt.FieldEqualPredicate(field="Run Name", equal=run_param)
        )
        boxplot_memory = boxplot_memory.transform_filter(
            alt.FieldEqualPredicate(field="Run Name", equal=run_param)
        )

    return (
        scatterplot
        & (barchart_timing | barchart_residual | barchart_memory)
        & (boxplot_timing | boxplot_residual | boxplot_memory)
    )


def main():
    start = time.time()
    args = get_args()
    logging.basicConfig(level=args.loglevel)

    df_list = []
    for fname in args.input:
        df_list.append(load_dataframe(fname))

    if len(df_list) == 1:
        num_runs = 1
        df = clean_dataframe(df_list[0])
    else:
        assert args.names is not None, (
            f"Passed in {len(df_list)} csvs, but no names. Please call with "
            + "python analysis.py --input 1.csv 2.csv --names name1 name2"
        )
        clean_df_list = []
        assert len(df_list) == len(
            args.names
        ), f"Passed in {len(df_list)} csvs, but only {len(args.names)} name(s)"
        for df, name in zip(df_list, args.names):
            clean_df_list.append(clean_dataframe(df, name, args.output))
        num_runs = len(clean_df_list)
        df = combine_df_runs(clean_df_list, args.names)

    chart_composite = composite_chart(df, num_runs)
    composite_fname = args.output + "polars-composite.html"
    logging.debug(f"Writing composite plot to {composite_fname}")
    chart_composite.save(composite_fname)
    end = time.time()
    logging.debug(f"polars-analysis.py completed in {end-start} s")


if __name__ == "__main__":
    main()
