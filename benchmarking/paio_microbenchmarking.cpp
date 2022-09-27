/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <filesystem>
#include <gflags/gflags.h>
#include <iomanip>
#include <paio/interface/posix_layer.hpp>
#include <paio/stage/paio_stage.hpp>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

// Struct to store temporary results of each worker thread.
struct ThreadResults {
    double m_iops;
    double m_throughput;
};

// Struct that stores the cumulative IOPS and throughput results (of all worker threads) of a given
// run.
struct MergedResults {
    uint32_t m_run_id;
    std::vector<double> m_iops;
    std::vector<double> m_throughput;
    double m_cumulative_iops;
    double m_cumulative_throughput;
};

// Struct that stores the average and standard deviation of IOPS and throughput results of all runs.
struct SetupResults {
    double m_avg_cumulative_iops;
    double m_stdev_cumulative_iops;
    double m_avg_cumulative_throughput;
    double m_stdev_cumulative_throughput;
};

/**
 * stress_test: continuously submit operation to the PAIO data plane stage in a close-loop.
 * @param instance Pointer to the PosixLayer interface to submit the request.
 * @param workflow_id Identifier of the workflow.
 * @param operation_size Size of the operation to be generated and submitted.
 * @param total_ops Number of operations to be submitted in the execution.
 * @param print_report Boolean that defines if the execution report is to be printed to the stdout.
 * @return Returns a ThreadResults object containing the performance results, namely IOPS and
 * throughput, of the execution.
 */
ThreadResults stress_test (FILE* fd,
    paio::PosixLayer* instance,
    const uint32_t& workflow_id,
    const ssize_t& operation_size,
    const uint64_t& total_ops,
    bool print_report)
{
    // allocate memory for the request's buffer
    auto* message = new unsigned char[operation_size];
    if (operation_size == 0) {
        message = nullptr;
    }

    auto start = std::chrono::high_resolution_clock::now ();
    for (uint64_t i = 1; i <= total_ops; i++) {
        // create a predetermined context object, with a fixed workflow id, operation type
        // (PAIO_GENERAL::Noop), operation (PAIO_GENERAL::Noop), operation size, and total
        // operations
        paio::core::Context context { (workflow_id * 1000),
            static_cast<int> (paio::core::PAIO_GENERAL::no_op),
            static_cast<int> (paio::core::PAIO_GENERAL::no_op),
            static_cast<uint64_t> (operation_size),
            1 };

        // memset message with zeros
        if (operation_size != 0) {
            memset (message, 0x00, sizeof (unsigned char) * operation_size);
        }

        // enforce message through posix_base method
        ssize_t result = instance->posix_base (message, operation_size, context);

        // validate if message was successfully enforced
        if (result == -1) {
            std::cerr << "Error: posix_noop failed (" << context.to_string () << ")" << std::endl;
        }
        assert (result != -1);
    }

    // calculate elapsed time
    auto end = std::chrono::high_resolution_clock::now ();
    std::chrono::duration<double> elapsed_seconds = end - start;

    // free message buffer
    delete[] message;

    // store performance results of the worker thread
    ThreadResults perf_result { .m_iops
        = static_cast<double> (total_ops) / elapsed_seconds.count () / 1000,
        .m_throughput = (static_cast<double> (total_ops)
                            * (static_cast<double> (operation_size) / 1024 / 1024 / 1024))
            / elapsed_seconds.count () };

    // print to stdout the execution report
    if (print_report) {
        std::fprintf (fd, "\n------------------------------------------------------------------\n");
        std::fprintf (fd, "\n Microbenchmark Throughput Test\n");
        std::fprintf (fd, "\n------------------------------------------------------------------\n");

        std::fprintf (fd,
            "Ops:\t%" PRIu64 "\t\tDuration:%f\n",
            total_ops,
            elapsed_seconds.count ());
        std::fprintf (fd,
            "IOPS:\t%lf.3 KOps/s\n",
            static_cast<double> (total_ops) / elapsed_seconds.count () / 1000);
        std::fprintf (fd, "------------------------------------------------------------------\n");

        std::fprintf (fd,
            "Thr:\t%lf.3 MiB/s\t%lf.3 GiB/s\n",
            (static_cast<double> (total_ops) * (static_cast<double> (operation_size) / 1024 / 1024))
                / elapsed_seconds.count (),
            (static_cast<double> (total_ops)
                * (static_cast<double> (operation_size) / 1024 / 1024 / 1024))
                / elapsed_seconds.count ());
        std::fprintf (fd,
            "Bw:\t%lf.3 MiB\t%lf.3 GiB\n",
            (static_cast<double> (total_ops)
                * (static_cast<double> (operation_size) / 1024 / 1024)),
            (static_cast<double> (total_ops)
                * (static_cast<double> (operation_size) / 1024 / 1024 / 1024)));
        std::fprintf (fd, "------------------------------------------------------------------\n\n");
    }

    return perf_result;
}

/**
 * record_stress_test_results: Store the results of a single worker stress test in a MergedResults
 * object.
 * @param results Pointer to a shared MergedResults object, that contains the results of all
 * threads.
 * @param threaded_results Const reference to the results' object of the single worker stress test.
 */
void record_stress_test_results (MergedResults* results, const ThreadResults& threaded_results)
{
    results->m_iops.emplace_back (threaded_results.m_iops);
    results->m_throughput.emplace_back (threaded_results.m_throughput);
    results->m_cumulative_iops += threaded_results.m_iops;
    results->m_cumulative_throughput += threaded_results.m_throughput;
}

/**
 * stdout_results: Print performance report of the MergedResults object to a given file (including
 * stdout). If detailed flag is enabled, the method also logs the performance results (IOPS and
 * throughput) of each worker.
 * @param fd Pointer to a FILE object to write the performance report.
 * @param results Performance results to be logged.
 * @param print_detailed Flag that defines if the performance results of each worker thread should
 * be also printed to file.
 */
void log_results (FILE* fd, const MergedResults& merged_results, bool print_detailed)
{
    std::fprintf (fd, "Run: %u\n", merged_results.m_run_id);
    std::fprintf (fd, "\tIOPS (KOps/s):\t%.3lf\n", merged_results.m_cumulative_iops);
    std::fprintf (fd, "\tThr (GiB/s):\t%.3lf\n", merged_results.m_cumulative_throughput);
    std::fprintf (fd, "----------------------------------\n");

    // log performance results of each worker thread
    if (print_detailed) {
        for (unsigned int i = 0; i < merged_results.m_iops.size (); i++) {
            std::fprintf (fd, "Thread-%d:\t", i);
            std::fprintf (fd,
                "%.3lf KOps/s; %.3lf GiB/s\n",
                merged_results.m_iops[i],
                merged_results.m_throughput[i]);
        }
    }

    std::fflush (fd);
}

/**
 * log_final_results: Record the results of the overall execution (all runs).
 * @param fd Pointer to a FILE object to write the performance report.
 * @param results Performance results of the overall benchmark execution, which includes the average
 * of all runs.
 * @param setup_name Name of the benchmarking setup.
 */
void log_final_results (FILE* fd, const SetupResults& results, const std::string& setup_name)
{
    std::fprintf (fd, "----------------------------------\n");
    std::fprintf (fd, "Setup results: %s\n", setup_name.c_str ());
    std::fprintf (fd, "\tIOPS (KOps/s):\t%.3lf\n", results.m_avg_cumulative_iops);
    std::fprintf (fd, "\tThr (GiB/s):\t%.3lf\n", results.m_avg_cumulative_throughput);
    std::fprintf (fd, "\tstdev-iops:\t%.3lf\n", results.m_stdev_cumulative_iops);
    std::fprintf (fd, "\tstdev-thr:\t%.3lf\n", results.m_stdev_cumulative_throughput);
    std::fprintf (fd, "----------------------------------\n");
}

/**
 * compute_stdev: Calculate standard deviation of a given sample.
 * @param sample Object that contains all values of the sample.
 * @return Returns the standard deviation value of the sample.
 */
double compute_stdev (const std::vector<double>& sample)
{
    double sum;
    double mean;
    double stdev = 0;

    int i;
    int sample_size = static_cast<int> (sample.size ());

    sum = std::accumulate (sample.begin (), sample.end (), 0.0);
    mean = sum / sample_size;

    for (i = 0; i < sample_size; i++)
        stdev += pow (sample[i] - mean, 2);

    return sqrt (stdev / sample_size);
}

/**
 * merge_final_results: Merge the performance results of all runs into a single object, which will
 * report the average and standard deviation values of the cumulative IOPS and throughput.
 * @param results Container with the performance results of all runs.
 * @return Returns a SetupResults object with the cumulative performance result of all runs.
 */
SetupResults merge_final_results (const std::vector<MergedResults>& results)
{
    SetupResults final_results {};
    int total_runs = static_cast<int> (results.size ());

    double cumulative_iops = 0;
    double cumulative_throughput = 0;
    std::vector<double> iops_sample_stdev {};
    std::vector<double> throughput_sample_stdev {};

    // compute cumulative IOPS and throughput
    for (int i = 0; i < total_runs; i++) {
        cumulative_iops += results[i].m_cumulative_iops;
        cumulative_throughput += results[i].m_cumulative_throughput;
        iops_sample_stdev.push_back (results[i].m_cumulative_iops);
        throughput_sample_stdev.push_back (results[i].m_cumulative_throughput);
    }

    // compute average and standard deviation values and store them in the SetupResults object
    final_results.m_avg_cumulative_iops = (cumulative_iops / total_runs);
    final_results.m_avg_cumulative_throughput = (cumulative_throughput / total_runs);
    final_results.m_stdev_cumulative_iops = compute_stdev (iops_sample_stdev);
    final_results.m_stdev_cumulative_throughput = compute_stdev (throughput_sample_stdev);

    return final_results;
}

/**
 * execute_run: method for performing the actual operations over the data plane stage. The method
 * spawns as much threads as the number of channels (parameter), each of which invokes the
 * stress_test call and stores the results in a ThreadResults shared object.
 * @param fd Pointer to a FILE object to write the performance report.
 * @param run_id Unique identifier of the current run.
 * @param channels Number of channels and threads to be used in the current run. This is both used
 * to initialize the PAIO data plane stage, and the spawn N worker threads to perform the actual
 * stress test.
 * @param create_default_enf_objects Flag to indicate whether the default enforcement objects should
 * be created at data plane stage initialization.
 * @param stage_name Name of the data plane stage.
 * @param total_operations Total number of operations to be performed by each worker thread.
 * @param operation_size Size of each operation.
 * @return Returns a MergedResults object with the results of the stress test.
 */
MergedResults execute_run (FILE* fd,
    uint32_t run_id,
    unsigned int channels,
    bool create_default_enf_objects,
    std::string stage_name,
    uint64_t total_ops,
    ssize_t op_size)
{
    // create object to store cumulative performance results
    MergedResults results { .m_run_id = (run_id + 1),
        .m_iops = {},
        .m_throughput = {},
        .m_cumulative_iops = 0,
        .m_cumulative_throughput = 0 };

    // create a shared PaioStage object
    std::shared_ptr<paio::PaioStage> stage = nullptr;

    if (paio::options::option_default_communication_type
        == paio::options::CommunicationType::none) {
        stage
            = std::make_shared<paio::PaioStage> (channels, create_default_enf_objects, stage_name);
    } else {
        stage = std::make_shared<paio::PaioStage> (channels,
            create_default_enf_objects,
            stage_name,
            paio::options::CommunicationType::none,
            paio::options::option_default_socket_name (),
            paio::options::option_default_port);
    }

    std::cout << stage->stage_info_to_string () << std::endl;

    // initialization of the Posix Instance layer
    paio::PosixLayer posix_instance (stage);

    std::cerr << "Operation size: " << op_size << std::endl;

    // enable I/O transformation setting for content-based operations
    if (op_size > 0) {
        posix_instance.set_io_transformation (true);
    }

    std::thread workers[channels];
    std::mutex lock;

    // lambda function for each thread to execute
    auto func = ([&lock, &results] (FILE* fd,
                     paio::PosixLayer* instance,
                     const uint32_t& flow_id,
                     const ssize_t& op_size,
                     const long& total_ops,
                     bool print) {
        // execute stress test
        ThreadResults thread_results
            = stress_test (fd, instance, flow_id, op_size, total_ops, print);
        {
            std::unique_lock<std::mutex> unique_lock (lock);
            record_stress_test_results (&results, thread_results);
        }
    });

    // spawn worker threads
    for (unsigned int i = 1; i <= channels; i++) {
        workers[i - 1] = std::thread (func, fd, &posix_instance, i, op_size, total_ops, false);
        std::cerr << "Starting worker thread #" << i << " (" << workers[i - 1].get_id () << ") ..."
                  << std::endl;
    }

    // join worker threads
    for (unsigned int i = 1; i <= channels; i++) {
        std::thread::id joining_thread_id = workers[i - 1].get_id ();
        workers[i - 1].join ();
        std::cerr << "Joining worker thread #" << i << " (" << joining_thread_id << ") ..."
                  << std::endl;
    }

    return results;
}

/**
 * trim: auxiliary method for trimming a string.
 * @param value String to be trimmed.
 * @return Returns the trimmed string.
 */
std::string trim (std::string value)
{
    unsigned int start = 0;

    while (start < value.size () && isspace (value[start])) {
        start++;
    }
    auto limit = static_cast<unsigned int> (value.size ());
    while (limit > start && isspace (value[limit - 1])) {
        limit--;
    }
    return { value.data () + start, limit - start };
}

/**
 * print_server_info: Print benchmark header message to a given file.
 * @param fd File descriptor to print to.
 */
void print_server_info (FILE* fd)
{
    std::fprintf (fd,
        "PAIO:      version %d.%d.%d\n",
        paio::options::kMajorVersion,
        paio::options::kMinorVersion,
        paio::options::kPatchVersion);

    std::stringstream time;
    const std::time_t t_c
        = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now ());
    time << "Date:      " << std::put_time (std::localtime (&t_c), "%F %T");
    std::fprintf (fd, "%s\n", time.str ().c_str ());

#if defined(__linux)
    FILE* cpuinfo = fopen ("/proc/cpuinfo", "r");
    if (cpuinfo != nullptr) {
        char line[1000];
        int num_cpus = 0;
        std::string cpu_type;
        std::string cache_size;

        while (std::fgets (line, sizeof (line), cpuinfo) != nullptr) {
            const char* sep = std::strchr (line, ':');
            if (sep == nullptr) {
                continue;
            }

            std::string key = trim (std::string (line, sep - 1 - line));
            std::string val = trim (std::string (sep + 1, std::strlen (sep + 1)));
            if (key == "model name") {
                ++num_cpus;
                cpu_type = val;
            } else if (key == "cache size") {
                cache_size = val;
            }
        }

        std::fclose (cpuinfo);
        std::fprintf (fd, "CPU:       %d * %s\n", num_cpus, cpu_type.c_str ());
        std::fprintf (fd, "CPUCache:  %s\n", cache_size.c_str ());
        std::fprintf (fd, "------------------------------------\n");
    }
#endif
}

DEFINE_uint32 (runs, 3, "Defines the number of runs to be conducted.");

DEFINE_uint32 (wtime, 10, "Defines the waiting time, in seconds, between runs.");

DEFINE_uint32 (threads, 1, "Number of concurrent worker threads to run.");

DEFINE_uint64 (ops, 10000000, "Defines the number of operations for each worker thread to submit.");

DEFINE_uint64 (size, 0, "Defines the block size of each operation.");

DEFINE_bool (store_run_perf_report,
    false,
    "Defines if the performance report of each run is persisted in a file or logged to stdout.");

DEFINE_bool (store_perf_report,
    false,
    "Defines if the performance report of the overall benchmark execution is persisted in a file or"
    " logged to stdout.");

DEFINE_string (result_path,
    "/tmp/paio-results/microbenchmarks-perf-results/",
    "Defines the path to store the performance results.");

/**
 * Notes:
 *  - Options header:
 *    - option_default_context_type = ContextType::PAIO_GENERAL;
 *    - option_create_default_channels = true
 */
int main (int argc, char** argv)
{
    // print header and node information to stdout
    print_server_info (stdout);

    // parse flags from stdin
    gflags::ParseCommandLineFlags (&argc, &argv, true);

    // benchmark setup
    std::vector<MergedResults> run_results;
    bool print_detailed = false;

    // data plane setup
    auto channels = static_cast<unsigned int> (FLAGS_threads);
    bool create_default_enf_objects = true;
    std::string stage_name { "microbenchmark-stage" };

    // application setup
    uint64_t total_operations = FLAGS_ops;
    auto operation_size = static_cast<ssize_t> (FLAGS_size);

    // create directory to store performance results
    if (FLAGS_store_perf_report && !FLAGS_result_path.empty ()) {
        fs::path path = FLAGS_result_path;

        // verify if directory already exists
        if (!fs::exists (path)) {
            bool mkdir_value = fs::create_directories (path);

            if (!mkdir_value) {
                std::cerr << "Error while creating " << path
                          << " directory: " << std::strerror (errno) << "\n";
            }
        }

        FLAGS_result_path = path.string ();
    }

    // File name
    fs::path filename;
    if (!FLAGS_result_path.empty ()) {
        filename = (FLAGS_result_path + "micro-perf-results-" + std::to_string (channels) + "-"
            + std::to_string (operation_size));
    }

    for (uint32_t run = 0; run < FLAGS_runs; run++) {
        FILE* fd_run_report;
        fs::path filename_run_perf_report = filename.string () + "-" + std::to_string (run + 1);

        // open file to store the performance report of the current run
        if (FLAGS_store_run_perf_report) {
            fd_run_report = std::fopen (filename_run_perf_report.string ().c_str (), "w");

            if (fd_run_report == nullptr) {
                std::cerr << "Error on open (" << filename_run_perf_report
                          << "): " << std::strerror (errno) << "\n";
                std::cerr << "Writing to stdout ..." << std::endl;
                fd_run_report = stdout;
            }
        } else {
            fd_run_report = stdout;
        }

        // execute run
        MergedResults results = execute_run (fd_run_report,
            run,
            channels,
            create_default_enf_objects,
            stage_name,
            total_operations,
            operation_size);

        // log results to file or stdout
        log_results (fd_run_report, results, print_detailed);

        // store MergedResults object in container
        run_results.emplace_back (results);

        // close file descriptor of the performance report file of the current run
        if (FLAGS_store_run_perf_report) {
            int close_value = std::fclose (fd_run_report);

            if (close_value != 0) {
                std::cerr << "Error on close (" << filename_run_perf_report
                          << "): " << std::strerror (errno) << std::endl;
            }
        }

        // sleep before going to the next run
        std::this_thread::sleep_for (std::chrono::seconds (FLAGS_wtime));
    }

    FILE* fd_perf_report;
    // merge final performance results
    SetupResults final_results = merge_final_results (run_results);

    // open file to store the final performance report (file or stdout)
    if (FLAGS_store_perf_report) {
        fd_perf_report = std::fopen (filename.string ().c_str (), "w");
        if (fd_perf_report == nullptr) {
            std::cerr << "Error on open (" << filename << "): " << std::strerror (errno) << "\n";
            std::cerr << "Writing to stdout ..." << std::endl;
            fd_perf_report = stdout;
        }
    } else {
        fd_perf_report = stdout;
    }

    // record final results in a given file (file or stdout)
    log_final_results (fd_perf_report, final_results, filename.string ());

    // close file descriptor of the performance report file
    if (FLAGS_store_perf_report) {
        int close_value = std::fclose (fd_perf_report);

        if (close_value != 0) {
            std::cerr << "Error on close (" << filename << "): " << std::strerror (errno)
                      << std::endl;
        }
    }
}
