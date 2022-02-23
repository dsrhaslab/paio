/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <gflags/gflags.h>
#include <paio/enforcement/objects/drl/enforcement_object_drl.hpp>

namespace paio::enforcement {

/**
 * DrlTest class.
 * Class dedicated to test the functionality and performance of the DRL enforcement object.
 * TODO:
 *  - add test for statistic_collection.
 */
class DrlTest {

private:
    FILE* m_fd { stdout };

    /**
     * obj_drl_change_rate: Configure DynamicRateLimiting object with a specific rate.
     * @param drl Pointer to a DynamicRateLimiting object.
     * @param rate New rate/throughput value.
     * @param log Defines if the log messages should be printed.
     */
    void obj_drl_change_rate (const std::shared_ptr<DynamicRateLimiter>& drl,
        const long& rate,
        const bool& log)
    {
        if (log) {
            std::fprintf (this->m_fd,
                "Adjust DRL Rate (%ld -> %ld); ",
                drl->get_token_bucket_rate (),
                rate);
        }

        // create configuration's vector
        std::vector<long> configurations { rate };
        // set new rate in DRL object
        PStatus status
            = drl->obj_configure (static_cast<int> (DRLConfiguration::rate), configurations);

        if (log) {
            std::fprintf (this->m_fd,
                "%s : %ld tokens/s\n",
                status.to_string ().c_str (),
                drl->get_token_bucket_rate ());
        }
    }

public:
    /**
     * DrlTest default constructor.
     */
    DrlTest () = default;

    /**
     * DrlTest parameterized constructor.
     * @param fd Pointer to a FILE that will store the log messages.
     */
    explicit DrlTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * DrlTest default destructor.
     */
    ~DrlTest () = default;

    /**
     * test_obj_drl_configure: Simple testing of DRL object configure. A single thread configures
     * the DRL object.
     * @param configuration Configuration option to be set in the DRL object.
     * @param max_range Maximum range of the random function used to generate initial throughput
     * rates (DRLConfiguration::init and DRLConfiguration::rate) and the refill window period
     * (DRLConfiguration::refill).
     * @param iterations Number of times to set the same configuration.
     */
    void test_obj_drl_configure (const DRLConfiguration& configuration,
        const long& max_range,
        const int& iterations)
    {
        // create drl_object with default configurations
        DynamicRateLimiter drl_object {};

        std::fprintf (this->m_fd, "---------------------------\n");
        std::fprintf (this->m_fd,
            "DRL obj_configure (%d, %d)\n",
            static_cast<int> (configuration),
            iterations);

        for (int i = 0; i < iterations; i++) {
            std::vector<long> configuration_values {};

            std::string message { std::to_string (i) };
            message.append (" >> bef. config: ").append (drl_object.to_string ()).append ("\n");

            switch (configuration) {
                case DRLConfiguration::init: {
                    // set configurations for initializing the DRL object
                    // using a static refill period and a random initial throughput rate
                    configuration_values.push_back (100000); // refill period (microseconds)
                    configuration_values.push_back (
                        static_cast<long> (random () % max_range)); // tokens/s

                    break;
                }

                case DRLConfiguration::rate: {
                    // set configurations for setting a new rate
                    configuration_values.push_back (static_cast<long> (random () % max_range));
                    break;
                }

                case DRLConfiguration::refill: {
                    // set configurations for setting a new refill period
                    configuration_values.push_back (
                        static_cast<long> (random () % (max_range / 10)));
                    break;
                }

                default:
                    Logging::log_error ("DRL Configuration does not exist.");
                    break;
            }

            // configure DRL object with previously set configurations
            PStatus status = drl_object.obj_configure (
                static_cast<int> (configuration), // configuration option
                configuration_values); // configuration parameters

            // validate token-bucket parameters and rate
            message.append (std::to_string (i));
            message.append (" >> aft. config: ").append (status.to_string ());
            message.append (" | ").append (drl_object.to_string ()).append ("\n");

            std::fprintf (this->m_fd, "%s\n", message.c_str ());
        }
    }

    /**
     * obj_drl_change_rate: Configure DynamicRateLimiting object with a specific rate.
     * @param drl Pointer to a DynamicRateLimiting object.
     * @param rate New rate/throughput value.
     * @param log Defines if the log messages should be printed.
     */
    void test_obj_drl_change_rate (const std::shared_ptr<DynamicRateLimiter>& drl,
        const long& rate,
        const bool& log)
    {
        std::fprintf (this->m_fd, "---------------------------\n");
        std::fprintf (this->m_fd, "Configure DynamicRateLimiting object: set new rate\n");

        // set new rate
        this->obj_drl_change_rate (drl, rate, log);
        std::fprintf (this->m_fd, "---------------------------\n\n");
    }

    /**
     * test_obj_configure_bg_worker: configure DynamicRateLimiting object with a dedicated thread.
     * @param drl Pointer to a DynamicRateLimiting object.
     * @param period Defines the period (in seconds) that the object is configured with a new rate.
     * @param iterations Total iterations to configure the rate.
     * @param range Throughput range to be generated.
     */
    void test_obj_configure_bg_worker (const std::shared_ptr<DynamicRateLimiter>& drl,
        const long& period,
        const int& iterations,
        const int& range)
    {
        auto func ([this, drl] (long period, int iterations, int range) {
            std::fprintf (this->m_fd, "---------------------------\n");
            std::fprintf (this->m_fd, "DRL test: adjust rate through background thread.\n");

            for (int i = 0; i < iterations; i++) {
                // sleep for N seconds before adjusting the rate
                std::this_thread::sleep_for (seconds (period));
                // compute new rate value
                long rate = random () % range;
                // change DRL object rate
                this->obj_drl_change_rate (drl, rate, true);
            }
        });

        // spawn background worker
        std::thread background_worker = std::thread (func, period, iterations, range);
        // join background worker
        background_worker.join ();

        std::fprintf (this->m_fd, "---------------------------\n\n");
    }

    /**
     * test_obj_drl_enforce: Enforce I/O mechanism from the DynamicRateLimiting object.
     * The method spawns a dedicated thread that enforces the DRL mechanism. On enforce, the
     * obj_enforce call will consume tokens from the token-bucket.
     * @param drl Pointer to a DynamicRateLimiting object.
     * @param ticket_id Ticket identifier.
     */
    void test_obj_drl_enforce (const std::shared_ptr<DynamicRateLimiter>& drl,
        const long& size,
        const int& iterations)
    {
        std::fprintf (this->m_fd, "---------------------------\n");
        std::fprintf (this->m_fd, "DRL object enforce (%ld)\n", drl->get_token_bucket_rate ());

        auto func = ([this, drl] (long size, int iterations) {
            auto start = std::chrono::system_clock::now ();

            for (int i = 0; i < iterations; i++) {
                // create Ticket object
                Ticket ticket {
                    static_cast<uint64_t> (i), // ticket-id
                    1, // total operations
                    size, // operation size
                    static_cast<int> (PAIO_GENERAL::no_op), // operation type
                    static_cast<int> (PAIO_GENERAL::no_op) // operation context
                };

                Result result {};
                // enforce DRL's I/O mechanism
                drl->obj_enforce (ticket, result);
            }

            std::chrono::duration<double> elapsed_time = std::chrono::system_clock::now () - start;

            std::fprintf (this->m_fd,
                "Ops:         %d\tDuration: %f\n",
                iterations,
                elapsed_time.count ());
            std::fprintf (this->m_fd, "Est. IOPS:   %ld Ops/s\n", drl->get_token_bucket_rate ());
            std::fprintf (this->m_fd,
                "Real IOPS:   %.3f Ops/s\n",
                static_cast<double> (iterations) / elapsed_time.count ());
            std::fprintf (this->m_fd, "---------------------------\n\n");
        });

        // spawn enforce worker
        std::thread worker = std::thread (func, size, iterations);
        worker.join ();
    }

    /**
     * test_drl_enforcement: Enforce I/O mechanisms from the DynamicRateLimiting object. The
     * method spawns a dedicated thread for enforcing the DRL mechanism (w/ test_obj_drl_enforce)
     * and another that changes the token-bucket rate (w/ test_obj_configure_bg_worker).
     * @param drl Pointer to a DynamicRateLimiting object.
     * @param enforce_iterations Number of enforcement iterations.
     * @param enforce_size Payload (number of tokens) to be consumed.
     * @param configure_iterations Number of rate adjustment iterations.
     * @param configure_period Periodicity to adjust the DRL rate.
     * @param configure_range Range of the rate values to be adjusted.
     */
    void test_drl_enforcement (const std::shared_ptr<DynamicRateLimiter>& drl,
        const int& enforce_iterations,
        const int& enforce_size,
        const int& configure_iterations,
        const int& configure_period,
        const int& configure_range)
    {
        // spawn a thread to call obj_enforce (consume tokens)
        auto consume_func = ([this, drl] (long size, int iterations) {
            this->test_obj_drl_enforce (drl, size, iterations);
        });

        // spawn a thread to periodically adjust the rate
        auto configure_func = ([this, drl] (long period, int iterations, int range) {
            this->test_obj_configure_bg_worker (drl, period, iterations, range);
        });

        std::thread enforce = std::thread (consume_func, enforce_size, enforce_iterations);
        std::thread adjust
            = std::thread (configure_func, configure_period, configure_iterations, configure_range);

        // join threads
        enforce.join ();
        adjust.join ();
    }
};
} // namespace paio::enforcement

using namespace paio::enforcement;

enum TestType {
    configs = 1, // test configurations
    simple_enforce = 2, // test simple enforce and bg worker
    advanced_enforce = 3 // simultaneously enforce and adjust rate
};

DEFINE_string (log_file_path, "", "Defines the path to the log file.");

DEFINE_bool (detailed_logging, true, "Enables detailed logging.");

DEFINE_int64 (ops, 100, "Defines the number of enforcement operations to be performed.");

DEFINE_int64 (enforcement_ops,
    1000000,
    "Defines the number of enforcement operations to be performed.");

DEFINE_int64 (throughput_range, 50000, "Defines the throughput range.");

DEFINE_int64 (rate, 100000, "Defines the rate of each rate-based operations");

DEFINE_string (test,
    "simple_enforce",
    "Defines the testing type to be used. configs: tests the DRL configure options; "
    "simple_enforce: perform a simple runs of the involving methods of DRL object, namely "
    "change rate, enforce requests, and configure rate through a background worker;"
    "advanced_enforce: performs a full enforcement environment, where a foreground thread "
    "continuously enforces requests through DRL object, and a background thread adjusts its rate.");

int main (int argc, char** argv)
{
    // check argv for the file to be placed the result
    FILE* fd = stdout;

    // parse flags from stdin
    gflags::ParseCommandLineFlags (&argc, &argv, true);

    // open file to write the logging results
    if (!FLAGS_log_file_path.empty ()) {
        fd = ::fopen (FLAGS_log_file_path.c_str (), "w");

        if (fd == nullptr) {
            fd = stdout;
            std::fprintf (fd,
                "Error while opening log file %s. Writing to stdout\n",
                FLAGS_log_file_path.c_str ());
        }
    }

    DrlTest test { fd };
    std::shared_ptr<DynamicRateLimiter> shared_drl = std::make_shared<DynamicRateLimiter> ();

    TestType type;

    if (FLAGS_test == "config") {
        type = TestType::configs;
    } else if (FLAGS_test == "simple_enforce") {
        type = TestType::simple_enforce;
    } else if (FLAGS_test == "advanced_enforce") {
        type = TestType::advanced_enforce;
    } else {
        std::cerr << "Test type not recognized.\n";
        return 1;
    }

    switch (type) {
        case TestType::configs: {
            // test DRL initialization
            test.test_obj_drl_configure (DRLConfiguration::init,
                FLAGS_throughput_range,
                static_cast<int> (FLAGS_ops));

            // test configure rate
            test.test_obj_drl_configure (DRLConfiguration::rate,
                FLAGS_throughput_range,
                static_cast<int> (FLAGS_ops));

            // test configure refill window
            test.test_obj_drl_configure (DRLConfiguration::refill,
                FLAGS_throughput_range,
                static_cast<int> (FLAGS_ops));

            // test change rate
            test.test_obj_drl_change_rate (shared_drl, FLAGS_rate, FLAGS_detailed_logging);
            break;
        }

        case TestType::simple_enforce: {
            test.test_obj_drl_change_rate (shared_drl, FLAGS_rate, FLAGS_detailed_logging);
            test.test_obj_drl_enforce (shared_drl, 1, static_cast<int> (FLAGS_enforcement_ops));
            test.test_obj_configure_bg_worker (shared_drl,
                5,
                5,
                static_cast<int> (FLAGS_throughput_range));
            break;
        }

        case TestType::advanced_enforce:
        default: {
            test.test_obj_drl_change_rate (shared_drl, FLAGS_rate, FLAGS_detailed_logging);
            test.test_drl_enforcement (shared_drl,
                static_cast<int> (FLAGS_enforcement_ops),
                1,
                10,
                2,
                FLAGS_rate);
        }
    }

    // close log file
    if (!FLAGS_log_file_path.empty ()) {
        std::fclose (fd);
    }
}
