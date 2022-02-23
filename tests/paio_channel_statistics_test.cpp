/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/statistics/channel_statistics.hpp>
#include <thread>

namespace paio::statistics {

/**
 * CollectionType enum.
 * Defines the type of statistic to be collected.
 * In detail:
 *  1: collects general statistics (get overall and windowed throughput value)
 *  2: collects a single entry from the statistic object (e.g., operation type, operation context)
 *  3: collect windowed statistics of all (registered) statistic entries
 * This enum should be only used in the ChannelStatisticsTest class.
 */
enum class CollectionType { general_stats = 1, entry_stats = 2, detailed_stats = 3 };

/**
 * ChannelStatisticsTest class.
 * Functional and performance tests related to the collections of statistics in ChannelStatistics
 * objects, including constructors creation and structure initialization, random statistic
 * generation and storage, collect general, single entry, and detailed entries, and more.
 * TODO:
 *  - add tests for collect_detailed_window_entries;
 */
class ChannelStatisticsTest {

private:
    FILE* m_fd { stdout };

    /**
     * select_operation_size: Selection the operation size based on the context type.
     * @param type Context type of the ChannelStatistics object.
     * @return Returns the corresponding operation size of the passed ContextType object.
     */
    static int select_operation_size (const ContextType& type)
    {
        switch (type) {
            case ContextType::LSM_KVS_SIMPLE:
                return lsm_kvs_simple_size;
            case ContextType::LSM_KVS_DETAILED:
                return lsm_kvs_detailed_size;
            case ContextType::POSIX:
                return posix_size;
            case ContextType::POSIX_META:
                return posix_meta_size;
            case ContextType::KVS:
                return kvs_size;
            default:
                return paio_general_size;
        }
    }

public:
    /**
     * ChannelStatisticsTest parameterized constructor.
     * @param fd Pointer to the file where the results of the test should be written to.
     */
    explicit ChannelStatisticsTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * test_constructors: Test default, copy, and parameterized constructors of ChannelStatistics
     * class.
     * @param constructor_type Simple way to define the type of constructor to use. 1 uses a
     * parameterized constructor; 2 uses a copy constructor; 0 or other number, uses the default
     * constructor.
     * @param identifier String identifier of the statistics object.
     * @param metric Defines the metric of which statistics should be computed.
     * @param classifier Defines the operation classifier to be collected.
     */
    void test_constructors (const int& constructor_type,
        const std::string& identifier,
        const StatisticMetric& metric,
        const ClassifierType& classifier)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Testing ChannelStatistics constructors\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        std::string message;
        switch (constructor_type) {
            case 1: {
                // parameterized constructor
                ChannelStatistics stats { identifier, metric, classifier };
                message.append ("ChannelStatistics parameterized constructor.\n");
                message.append (stats.to_string ()).append ("\n");
                message.append (stats.to_string_meta ()).append ("\n");

                break;
            }

            case 2: {
                // copy constructor
                ChannelStatistics stats_original { identifier, metric, classifier };
                message.append ("ChannelStatistics copy constructor.\n");
                message.append ("Original ").append (stats_original.to_string ()).append ("\n");
                message.append ("Original ")
                    .append (stats_original.to_string_meta ())
                    .append ("\n");

                ChannelStatistics stats_copy { identifier, metric, classifier };
                message.append ("Copy ").append (stats_copy.to_string ()).append ("\n");
                message.append ("Copy ").append (stats_copy.to_string_meta ()).append ("\n");

                break;
            }

            case 0:
            default:
                // default constructor
                ChannelStatistics stats_default {};
                message.append ("ChannelStatistics default constructor.\n");
                message.append (stats_default.to_string ()).append ("\n");
                message.append (stats_default.to_string_meta ()).append ("\n");

                break;
        }

        std::fprintf (this->m_fd, "%s", message.c_str ());
        std::fflush (this->m_fd);

        std::fprintf (this->m_fd, "----------------------------\n");
    }

    /**
     * test_initialize: Initialize ChannelStatistics object with a given set of operations
     * (ContextType).
     * @param stats Pointer to a ChannelStatistics object.
     * @param type Defines the type of operations to be collected. In this case, will initialize
     * the ChannelStatistics containers with the respective operators.
     */
    void test_initialize (ChannelStatistics* stats, const ContextType& type)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Testing ChannelStatistics initialization\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        std::string initialize_result;
        switch (type) {
            case ContextType::LSM_KVS_SIMPLE:
                initialize_result.append ("ChannelStatistics test initialize :: LSM_KVS_SIMPLE\n");
                break;

            case ContextType::LSM_KVS_DETAILED:
                initialize_result.append (
                    "ChannelStatistics test initialize :: LSM_KVS_DETAILED\n");
                break;

            case ContextType::POSIX:
                initialize_result.append ("ChannelStatistics test initialize :: POSIX\n");
                break;

            case ContextType::POSIX_META:
                initialize_result.append ("ChannelStatistics test initialize :: POSIX_META\n");
                break;

            case ContextType::KVS:
                initialize_result.append ("ChannelStatistics test initialize :: KVS\n");
                break;

            case ContextType::PAIO_GENERAL:
                initialize_result.append ("ChannelStatistics test initialize :: PAIO_GENERAL\n");
                break;

            default:
                initialize_result.append ("Unknown context type\n");
                break;
        }

        // initialize ChannelStatistics object
        stats->initialize (type);
        initialize_result.append (stats->to_string ()).append ("\n");

        std::fprintf (this->m_fd, "%s", initialize_result.c_str ());
        std::fflush (this->m_fd);

        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * test_random_statistic_generator: Update random statistic entries. Both operation type and
     * context are generated randomly.
     * @param stats Pointer to a ChannelStatistics object.
     * @param iterations Number of iterations execute the update operation.
     * @param type Context type of the ChannelStatistics object.
     * @param period Time period between update cycles, expressed in microseconds.
     * @param interrupted Shared pointer that marks whether the execution should stop.
     */
    void test_random_statistic_generator (ChannelStatistics* stats,
        const long& iterations,
        const ContextType& type,
        const long& period,
        std::shared_ptr<std::atomic<bool>> interrupted,
        const bool& detailed_log)
    {
        // printer header
        if (detailed_log) {
            std::fprintf (this->m_fd, "----------------------------\n");
            std::fprintf (this->m_fd,
                "Testing ChannelStatistics random generator (%d, %d)\n",
                static_cast<int> (stats->get_metric ()),
                static_cast<int> (stats->get_classifier_type ()));
            std::fprintf (this->m_fd, "----------------------------\n");
        }

        // select the size for the range of operation type and context that will be generated
        int operation_generator = ChannelStatisticsTest::select_operation_size (type);

        auto start = std::chrono::system_clock::now ();
        for (int i = 0; i < iterations && !(interrupted->load ()); i++) {
            int operation_type = static_cast<int> (random () % operation_generator);
            int operation_context = static_cast<int> (random () % operation_generator);
            int value = 1;

            // update statistic entry
            stats->update_statistic_entry (operation_type, operation_context, value);

            if (period > 0) {
                // sleep for N microseconds before next iteration
                std::this_thread::sleep_for (microseconds (period));
            }
        }

        std::chrono::duration<double> elapsed_time = std::chrono::system_clock::now () - start;

        if (detailed_log) {
            std::string message { stats->to_string () };
            message.append ("\n---------------------\n");
            message.append ("Register:  ").append (std::to_string (iterations));
            message.append ("\tDuration: ").append (std::to_string (elapsed_time.count ()));
            message.append ("\nIOPS:  ");
            message.append (
                std::to_string (static_cast<double> (iterations) / elapsed_time.count () / 1000));
            message.append (" KEntries/s\n");
            message.append ("----------------------------\n\n");

            // write statistic log to file
            std::fprintf (this->m_fd, "%s", message.c_str ());
            std::fflush (this->m_fd);
        }
    }

    /**
     * test_collect: Collect general statistics from the ChannelStatistics object.
     * @param stats Pointer to a ChannelStatistics object.
     * @param log Boolean that defines whether it prints logging messages to stdout or not.
     */
    void test_collect (ChannelStatistics* stats, const bool& log)
    {
        // create structure to store statistics
        ChannelStatsRaw stats_raw {};
        // collect general statistics
        stats->collect (stats_raw);

        if (log) {
            std::string message;
            message.append ("ChannelStatistics collect: {");
            message.append (std::to_string (stats_raw.m_overall_metric_value)).append (", ");
            message.append (std::to_string (stats_raw.m_windowed_metric_value)).append ("}\n");

            std::fprintf (this->m_fd, "%s", message.c_str ());
        }
    }

    /**
     * test_collect_single_entry: Collect single entry statistic from the ChannelStatistics object.
     * @param stats Pointer to a ChannelStatistics object.
     * @param operation Operation entry to be collected. If -1, the picks an entry randomly.
     * @param type Context type of the ChannelStatistics object.
     * @param log Boolean that defines whether it prints logging messages to stdout or not.
     */
    void test_collect_single_entry (ChannelStatistics* stats,
        const int& operation,
        const ContextType& type,
        const bool& log)
    {
        // create structure to store statistics
        ChannelStatsRaw stats_raw {};

        int operation_entry = operation;
        // generate random operation entry
        if (operation == -1) {
            operation_entry = static_cast<int> (random () % select_operation_size (type));
        }

        // collect statistics of a single entry
        stats->collect_single_entry (stats_raw, operation_entry);

        if (log) {
            std::string message;
            message.append ("ChannelStatistics collect single entry: {");
            message.append (std::to_string (operation_entry)).append (": ");
            message.append (std::to_string (stats_raw.m_overall_metric_value)).append (", ");
            message.append (std::to_string (stats_raw.m_windowed_metric_value)).append ("}\n");
            message.append (stats->to_string ()).append ("\n");

            // write message to log file
            std::fprintf (this->m_fd, "%s", message.c_str ());
        }
    }

    /**
     * test_collect_detailed_entries: Collect statistics for each entry of the ChannelStatistics
     * object.
     * @param stats Pointer to a ChannelStatistics object.
     * @param log Boolean that defines whether it prints logging messages to stdout or not.
     */
    void test_collect_detailed_entries (ChannelStatistics* stats, const bool& log)
    {
        // create entries container
        std::vector<double> entries {};
        // collect detailed statistics
        stats->collect_detailed_windowed_entries (entries);

        if (log) {
            std::stringstream stream;
            stream << "ChannelStatistics collect detailed entries: {";
            for (double entry : entries) {
                stream << entry << ", ";
            }
            stream << "}\n";

            std::fprintf (this->m_fd, "%s", stream.str ().c_str ());
        }
    }

    /**
     * test_register_and_collect: Simultaneously register and collect statistics. Specifically, the
     * method spawns a thread to update statistic entries and another for periodically collecting
     * detailed, general, or single entry statistics. Instead of passing an existing
     * ChannelStatistics object, it creates a new one and operates over it.
     * @param metric Defines the metric of which statistics should be computed.
     * @param classifier Defines the operation classifier to be collected.
     * @param type Defines the type of the operation that will be submitted.
     * @param register_iterations Number of cycles for updating statistics.
     * @param collect_iterations Number of cycles for collecting statistics.
     * @param register_period Time period between update cycles, expressed in microseconds.
     * @param collect_period Time period between collection cycles, expressed in microseconds.
     * @param collection_type Type of statistics to be collected. Specifically, 1 = general
     * statistics (test_collect); 2 = entry statistics (test_collect_single_entry); * = detailed
     * statistics (test_collect_detailed_entries).
     * @param interrupted Shared pointer that marks whether the execution should stop.
     */
    void test_register_and_collect (const StatisticMetric& metric,
        const ClassifierType& classifier,
        const ContextType& type,
        const long& register_iterations,
        const long& collect_iterations,
        const long& register_period,
        const long& collect_period,
        const CollectionType& collection_type,
        std::shared_ptr<std::atomic<bool>> interrupted)
    {
        // create ChannelStatistics object
        ChannelStatistics stats { "channel-test", metric, classifier };
        // initialize ChannelStatistics object
        stats.initialize (type);

        // lambda function to register statistics
        auto register_stats_func = ([this] (ChannelStatistics* stats,
                                        const long& iterations,
                                        const ContextType& type,
                                        const long& period,
                                        std::shared_ptr<std::atomic<bool>> interrupted) {
            // convert Thread-ID to string
            std::stringstream stream;
            stream << "\n----------------------------\n";
            stream << "Thread-" << std::this_thread::get_id ()
                   << " executing test_random_statistic_generator ...\n";
            stream << "----------------------------\n";
            std::fprintf (this->m_fd, "%s", stream.str ().c_str ());

            // execute random statistic generator in separate thread
            this->test_random_statistic_generator (stats,
                iterations,
                type,
                period,
                interrupted,
                false);
        });

        // lambda function to collect statistics
        auto collect_stats_func = ([this] (ChannelStatistics* stats,
                                       const long& iterations,
                                       const CollectionType& collection_type,
                                       const ContextType& type,
                                       const long& period,
                                       std::shared_ptr<std::atomic<bool>> interrupted) {
            // convert Thread-ID to string
            std::stringstream stream;
            stream << "\n----------------------------\n";
            stream << "Thread-" << std::this_thread::get_id () << " collecting statistics ...\n";
            stream << "----------------------------\n";
            std::fprintf (this->m_fd, "%s", stream.str ().c_str ());

            // template log message
            switch (collection_type) {
                case CollectionType::general_stats:
                    std::fprintf (this->m_fd, "iter. : {<overall>, <windowed>}\n");
                    break;

                case CollectionType::entry_stats:
                    std::fprintf (this->m_fd,
                        "iter. : ChannelStatistics collect single entry: {<overall>, "
                        "<windowed>}\n");
                    break;

                case CollectionType::detailed_stats:
                default:
                    std::fprintf (this->m_fd,
                        "iter. : ChannelStatistics collect detailed entries: {<entry1-windowed>, "
                        "..., <entryN-windowed>}\n");
                    break;
            }

            // periodically collect statistics through a separate thread
            for (int i = 0; i < iterations; i++) {
                std::fprintf (this->m_fd, "%d:\t", i);
                switch (collection_type) {
                    case CollectionType::general_stats:
                        // simple collection (get overall and windowed throughput value)
                        this->test_collect (stats, true);
                        break;

                    case CollectionType::entry_stats:
                        // collect single entry from the statistic object (e.g., operation type,
                        // operation context, ...)
                        this->test_collect_single_entry (stats, -1, type, true);
                        break;

                    case CollectionType::detailed_stats:
                    default:
                        // collect windowed statistics of all (registered) statistic entries
                        this->test_collect_detailed_entries (stats, true);
                        break;
                }

                std::this_thread::sleep_for (microseconds (period));
            }

            // mark execution as finished
            interrupted->store (true);
        });

        // spawn thread to register statistics
        std::thread register_thread = std::thread (register_stats_func,
            &stats,
            register_iterations,
            type,
            register_period,
            interrupted);

        // spawn thread to collect statistics
        std::thread collect_thread = std::thread (collect_stats_func,
            &stats,
            collect_iterations,
            collection_type,
            type,
            collect_period,
            interrupted);

        // join register and collection threads
        register_thread.join ();
        collect_thread.join ();
    }

    /**
     * test_get_overall_metric: Get overall statistics metric value.
     * @param stats Pointer to a ChannelStatistics object.
     */
    void test_get_overall_metric (ChannelStatistics* stats)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Testing ChannelStatistics overall metric collection\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        // create raw structure
        ChannelStatsRaw stats_raw {};
        // collect general statistics
        stats->collect (stats_raw);

        std::string message { "ChannelStatistics collect: {" };
        message.append (std::to_string (stats_raw.m_overall_metric_value)).append (", ");
        message.append (std::to_string (stats_raw.m_windowed_metric_value)).append ("}\n");

        // get overall metric
        message.append ("ChannelStatistics overall metric: {");
        message.append (std::to_string (stats->get_overall_metric ())).append ("}\n");

        // get statistics' string
        message.append (stats->to_string ()).append ("\n\n");

        std::fprintf (this->m_fd, "%s", message.c_str ());
    }

    /**
     * test_get_previous_metric_window: Get windowed statistics metric value.
     * @param stats Pointer to a ChannelStatistics object.
     */
    void test_get_previous_metric_window (ChannelStatistics* stats)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Testing ChannelStatistics windowed metric collection\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        // create raw structure
        ChannelStatsRaw stats_raw {};
        // collect general statistics
        stats->collect (stats_raw);

        std::string message { "ChannelStatistics collect: {" };
        message.append (std::to_string (stats_raw.m_overall_metric_value)).append (", ");
        message.append (std::to_string (stats_raw.m_windowed_metric_value)).append ("}\n");

        // get windowed metric
        message.append ("ChannelStatistics windowed metric: {");
        message.append (std::to_string (stats->get_previous_metric_window ())).append ("}\n");

        std::fprintf (this->m_fd, "%s", message.c_str ());
    }
};

} // namespace paio::statistics

using namespace paio::statistics;

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

    ChannelStatisticsTest test { fd };
    StatisticMetric metric = StatisticMetric::throughput;
    ClassifierType classifier = ClassifierType::operation_context;
    paio::core::ContextType type = paio::core::ContextType::LSM_KVS_DETAILED;
    std::shared_ptr<std::atomic<bool>> interrupted { std::make_shared<std::atomic<bool>> (false) };

    // test Statistic constructors
    test.test_constructors (2, "channel-test", metric, classifier);

    ChannelStatistics stats { "channel-test", metric, classifier };

    test.test_initialize (&stats, type);
    test.test_random_statistic_generator (&stats, 1000000, type, 1, interrupted, true);
    // test.test_collect (&stats, true);
    // test.test_collect_single_entry (&stats, -1, type, true);
    // test.test_collect_detailed_entries (&stats, true);

    test.test_get_overall_metric (&stats);
    test.test_get_previous_metric_window (&stats);

    long register_iterations = 10000000;
    long collect_iterations = 20;
    long register_period = 1; // 1 us
    long collection_period = 1000000; // 1 s
    CollectionType collection_type = CollectionType::general_stats;
    // reset interrupted
    interrupted->store (false);

    test.test_register_and_collect (metric,
        classifier,
        type,
        register_iterations,
        collect_iterations,
        register_period,
        collection_period,
        collection_type,
        interrupted);

    // close file descriptor
    if (fd != stdout && fd != nullptr) {
        std::fclose (fd);
    }
}
