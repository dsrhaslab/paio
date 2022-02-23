/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <cinttypes>
#include <paio/interface/posix_layer.hpp>
#include <paio/stage/paio_stage.hpp>

namespace paio {

/**
 * PosixLayerTest class.
 * Class dedicated for testing the functionality of the PosixLayer class.
 */
class PosixLayerTest {

private:
    FILE* m_fd { stdout };
    std::atomic<uint64_t> m_mean_ops { 0 };
    std::atomic<uint64_t> m_mean_bytes { 0 };
    std::atomic<uint64_t> m_total_ops { 0 };
    std::atomic<uint64_t> m_total_bytes { 0 };
    std::atomic<bool> m_has_finished { false };

    /**
     * record_ops: Records the number of operations and bytes, both mean and absolute value,
     * performed by the stage.
     * @param ops Number of operations to record.
     * @param bytes Number of bytes to record.
     */
    void record_ops (const int& ops, const uint64_t& bytes)
    {
        this->m_mean_ops.fetch_add (ops);
        this->m_mean_bytes.fetch_add (bytes);
        this->m_total_ops.fetch_add (ops);
        this->m_total_bytes.fetch_add (bytes);
    }

    /**
     * compute_mean_performance_report: Computes the mean IOPS and mean throughput performance.
     * @param stage_identifier Stage identifier.
     * @param time_in_micros Time in microseconds.
     */
    void compute_mean_performance_report (const std::string& stage_identifier,
        const uint64_t& time_in_micros)
    {
        // compute mean IOPS
        double mean_iops = (static_cast<double> (this->m_mean_ops.load ()) / 1000)
            / (static_cast<double> (time_in_micros) / 1000000);
        // compute mean throughput
        double mean_throughput = (static_cast<double> (this->m_mean_bytes.load ()) / 1024 / 1024)
            / (static_cast<double> (time_in_micros) / 1000000);

        // reset mean counters
        this->m_mean_ops.store (0);
        this->m_mean_bytes.store (0);

        // print to stdout mean performance counter
        std::fprintf (this->m_fd,
            "%s \t...\t %.3f KOps; %.3f MiB/s\n",
            stage_identifier.c_str (),
            mean_iops,
            mean_throughput);
    }

    /**
     * compute_performance_report: Compute final IOPS and throughput performance, and print
     * performance report.
     * @param stage_identifier Stage identifier.
     * @param time_in_micros Time in microseconds.
     */
    void compute_performance_report (const std::string& stage_identifier,
        const double& time_in_micros)
    {
        // compute IOPS
        double iops = (static_cast<double> (this->m_total_ops.load ()) / 1000)
            / (static_cast<double> (time_in_micros));
        // compute throughput
        double throughput = (static_cast<double> (this->m_total_bytes.load ()) / 1024 / 1024)
            / (static_cast<double> (time_in_micros));

        // print to stdout mean performance counter
        std::fprintf (this->m_fd, "--------------------------------------\n");
        std::fprintf (this->m_fd, "PosixLayer Performance Report: %s\n", stage_identifier.c_str ());
        std::fprintf (this->m_fd, "- execution time:\t%fs\n", time_in_micros);
        std::fprintf (this->m_fd, "- total ops:\t\t%" PRIu64 "\n", this->m_total_ops.load ());
        std::fprintf (this->m_fd, "- total bytes:\t\t%" PRIu64 "\n", this->m_total_bytes.load ());
        std::fprintf (this->m_fd, "- IOPS:\t\t\t%.3f KOps\n", iops);
        std::fprintf (this->m_fd, "- Throughput:\t\t%.3f MiB/s\n", throughput);
        std::fprintf (this->m_fd, "--------------------------------------\n");
    }

    /**
     * submit_posix_request: Submit a posix request to the data plane stage. The method create a
     * Context object based on the workflow-id, operation type and context, and submits it in the
     * form of a posix_base request.
     * @param posix_instance Pointer to a PosixLayer object.
     * @param workflow_id Workflow identifier.
     * @param operation_type Type of the I/O request.
     * @param operation_context Context of the I/O request.
     * @param operation_size Size of the I/O request.
     */
    void submit_posix_request (PosixLayer* posix_instance,
        const long& workflow_id,
        const int& operation_type,
        const int& operation_context,
        const uint64_t& operation_size)
    {
        // create Context object
        auto context_obj = posix_instance->build_context_object (workflow_id,
            operation_type,
            operation_context,
            operation_size,
            1);

        // submit request through posix_base
        auto size = posix_instance->posix_base (nullptr, operation_size, context_obj);

        // record operation
        switch (operation_type) {
            case static_cast<int> (POSIX::read):
            case static_cast<int> (POSIX::write):
                this->record_ops (1, size);
                break;
            case static_cast<int> (POSIX::open):
            case static_cast<int> (POSIX::close):
                this->record_ops (1, 0);
                break;
            default:
                throw std::runtime_error ("PosixLayerTest: invalid operation type");
        }
    }

public:
    /**
     * PosixLayerTest default constructor.
     */
    PosixLayerTest () = default;

    /**
     * PosixLayerTest parameterized constructor.
     * @param fd Pointer to a file to where log messages should be written.
     */
    explicit PosixLayerTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * PosixLayerTest default destructor.
     */
    ~PosixLayerTest () = default;

    /**
     * set_env: Set environment variable with name 'env_name' and value 'env_value'.
     * @param env_name Name of the environment variable.
     * @param env_value Value to be set in the environment variable.
     * @return Returns true if the environment variable was successfully set; false otherwise.
     */
    bool set_env (const std::string& env_name, const std::string& env_value)
    {
        auto return_value = ::setenv (env_name.c_str (), env_value.c_str (), 1);
        return (return_value == 0);
    }

    /**
     * unset_env: Remove environment variable with name 'env_name'.
     * @param env_name Name of the environment variable to remove.
     * @return Returns true if the environment variable was successfully unset; false otherwise.
     */
    bool unset_env (const std::string& env_name)
    {
        auto return_value = ::unsetenv (env_name.c_str ());
        return (return_value == 0);
    }

    /**
     * submit_requests: Submit a number of POSIX requests to the data plane stage.
     * @param posix_instance Pointer to a PosixLayer object.
     * @param workflow_id Workflow identifier.
     * @param operation_type Type of the I/O request.
     * @param operation_context Context of the I/O request.
     * @param size Size of the I/O request.
     * @param total_ops Total number of operations to be submitted.
     * @param sleep_period Sleep period between two consecutive operations.
     */
    void submit_requests (PosixLayer* posix_instance,
        const long& workflow_id,
        const int& operation_type,
        const int& operation_context,
        const uint64_t& size,
        const uint64_t& total_ops,
        const uint64_t& sleep_period)
    {
        // submit requests
        for (std::uint64_t i = 0; i < total_ops; i++) {
            this->submit_posix_request (posix_instance,
                workflow_id,
                operation_type,
                operation_context,
                size);
            // sleep in micros
            if (sleep_period > 0) {
                ::usleep (sleep_period);
            }
        }

        this->m_has_finished.store (true);
    }

    /**
     * execute_posix_app: Simulate a POSIX application that submits request in loop back manner.
     * The method spawns a worker thread (as much as threads parameter) that submits requests
     * with a given workflow-id, operation type, and operation context, which are fetched from the
     * respective containers. The methods spawns an additional thread that periodically computes
     * the average throughput and IOPS values.
     * @param posix_instance Pointer to a PosixLayer object.
     * @param workflow_id Container with the workflow identifiers.
     * @param operation_type Container with the I/O requests' operation type.
     * @param operation_context Container with the I/O requests' operation context.
     * @param size Size of the operation to be performed.
     * @param total_ops Total number of operations to be submitted.
     * @param sleep_period Sleep period between two consecutive operations.
     */
    void execute_posix_app (PosixLayer* posix_instance,
        const std::string& stage_id,
        const int& threads,
        const std::vector<long>& workflow_id,
        const std::vector<int>& operation_type,
        const std::vector<int>& operation_context,
        const uint64_t& size,
        const uint64_t& total_ops,
        const uint64_t& sleep_period,
        const uint64_t& report_period)
    {
        // spawn thread for executing submit_requests
        auto submitter_func = [this] (PosixLayer* posix_instance,
                                  const long& workflow_id,
                                  const int& operation_type,
                                  const int& operation_context,
                                  const uint64_t& size,
                                  const uint64_t& total_ops,
                                  const uint64_t& sleep_period) {
            this->submit_requests (posix_instance,
                workflow_id,
                operation_type,
                operation_context,
                size,
                total_ops,
                sleep_period);
        };

        // spawn thread for collecting throughput and printing to stdout
        auto monitor_func = [this] (const std::string& stage_id, const uint64_t& report_period) {
            auto start = std::chrono::high_resolution_clock::now ();

            while (!this->m_has_finished) {
                std::this_thread::sleep_for (std::chrono::microseconds (report_period));
                this->compute_mean_performance_report (stage_id, report_period);
            }

            auto end = std::chrono::high_resolution_clock::now ();
            std::chrono::duration<double> elapsed_time_in_micros = end - start;

            this->compute_performance_report (stage_id, elapsed_time_in_micros.count ());
        };

        std::thread worker_threads[threads];

        // spawn threads worker threads
        for (int i = 0; i < threads; i++) {
            worker_threads[i] = std::thread (submitter_func,
                posix_instance,
                workflow_id[i],
                operation_type[i],
                operation_context[i],
                size,
                total_ops,
                sleep_period);
        }
        // spawn monitor thread
        std::thread monitor_thread { monitor_func, stage_id, report_period };

        // join worker threads
        for (int i = 0; i < threads; i++) {
            worker_threads[i].join ();
        }
        // join monitor thread
        monitor_thread.join ();
    }
};
} // namespace paio

using namespace paio;

/**
 * Notes - options header:
 *  - option_default_context_type = ContextType::POSIX;
 *  - option_create_default_channels = false;
 *  - option_default_channel_differentiation_workflow = true;
 * execution sample:
 *  $ ./posix_layer_test
 *  $ env paio_name="" ./posix_layer_test
 */
int main (int argc, char** argv)
{
    // check argv for the file to be placed the result
    FILE* fd = stdout;

    // open file to write the logging results
    if (argc > 1) {
        fd = ::fopen (argv[1], "w");

        if (fd == nullptr) {
            fd = stdout;
        }
    }

    PosixLayerTest test { fd };

    std::string stage_name = paio::options::option_environment_variable_name ();
    std::string stage_name_value = "Posix-layer-test";
    test.set_env (stage_name, stage_name_value);
    std::string stage_env = paio::options::option_environment_variable_env ();
    std::string stage_env_value = "instance-1";
    test.set_env (stage_env, stage_env_value);

    int channels = 4;
    bool default_object_creation = true;

    std::shared_ptr<PaioStage> stage { std::make_shared<PaioStage> (channels,
        default_object_creation,
        stage_name_value,
        std::string { "../files/tests/posix_layer_test_housekeeping_rules" },
        std::string {},
        std::string {},
        true) };
    PosixLayer posix_instance { stage };

    std::fprintf (fd, "%s\n", stage->stage_info_to_string ().c_str ());
    std::fprintf (fd, "%s\n", posix_instance.to_string ().c_str ());

    // defined fixed testing parameters
    const std::vector<long> workflow_ids { 1000, 2000, 3000, 4000 };
    const std::vector<int> operation_type { static_cast<int> (POSIX::read),
        static_cast<int> (POSIX::write),
        static_cast<int> (POSIX::open),
        static_cast<int> (POSIX::close) };
    const std::vector<int> operation_context { static_cast<int> (POSIX::no_op),
        static_cast<int> (POSIX::no_op),
        static_cast<int> (POSIX::no_op),
        static_cast<int> (POSIX::no_op) };
    uint64_t operation_size = 4096;
    uint64_t total_ops = 10000000;
    uint64_t sleep_time_between_ops = 0; // 5us
    uint64_t report_period = 1000000; // 1s

    std::fprintf (fd, "Sleeping for 3 seconds ...\n");
    ::sleep (3);

    // execute test
    test.execute_posix_app (&posix_instance,
        stage->get_stage_info_name (),
        channels,
        workflow_ids,
        operation_type,
        operation_context,
        operation_size,
        total_ops,
        sleep_time_between_ops,
        report_period);

    // close log file
    if (argc > 1 && fd != stdout) {
        std::fclose (fd);
    }

    return 0;
}
