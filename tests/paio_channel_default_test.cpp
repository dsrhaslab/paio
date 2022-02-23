/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <cinttypes>
#include <paio/enforcement/channel_default.hpp>

namespace paio::enforcement {

/**
 *  ChannelDefaultTest class.
 *  Functional and performance tests related to the ChannelDefault object, including the create and
 *  configure EnforcementObjects, create tickets, enforce requests, and collect statistics.
 *  TODO:
 *   - add the following tests: test_define_object_differentiation,
 *   test_build_differentiation_token, test_collect_object_statistics;
 *   - add testing branch (in main) for statistics collection.
 */
class ChannelDefaultTest {

private:
    FILE* m_fd { stdout };

    /**
     * create_object: Invoke the channel's create object mechanism.
     * @param channel Pointer to a ChannelDefault object.
     * @param object_id EnforcementObject identifier.
     * @param operation_type Type of operations that the object should handle.
     * @param operation_context Context of operations that the object should handle.
     * @param object_type Type of the EnforcementObject to be created.
     * @param configurations Initialization configurations to be set.
     * @return Returns a PStatus object, resulted from invoking channel's create_enforcement_object.
     */
    PStatus create_object (ChannelDefault* channel,
        const long& object_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context,
        const EnforcementObjectType& object_type,
        const std::vector<long>& configurations)
    {
        // create object differentiation pair
        ObjectDifferentiationPair diff_pair { operation_type, operation_context };
        // create EnforcementObject
        return channel->create_enforcement_object (object_id,
            diff_pair,
            object_type,
            configurations);
    }

    /**
     * channel_enforce: Invoke the channel's enforcement mechanism over the I/O request. It creates
     * a simple Context object with a given operation type and context, and an empty Result object.
     * @param channel Pointer to a ChannelDefault object.
     * @param iterations Number of iterations to enforce the I/O mechanism.
     * @param operation_type Operation type of the request to be enforced.
     * @param operation_context Operation context of the request to be enforced.
     */
    void channel_enforce (ChannelDefault* channel,
        const uint64_t& iterations,
        const uint32_t& operation_type,
        const uint32_t& operation_context)
    {
        for (uint64_t i = 0; i < iterations; i++) {
            // create Context object
            Context context { static_cast<long> (i),
                static_cast<int> (operation_type),
                static_cast<int> (operation_context),
                1,
                1 };
            // create empty Result object
            Result result {};
            // enforce request
            channel->channel_enforce (context, nullptr, 0, result);
        }
    }

public:
    /**
     * ChannelDefaultTest constructor.
     */
    ChannelDefaultTest () = default;

    /**
     * ChannelDefaultTest parameterized constructor.
     * @param fd Pointer to a FILE through where the log messages should be written to.
     */
    explicit ChannelDefaultTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * ChannelDefaultTest default destructor.
     */
    ~ChannelDefaultTest () = default;

    /**
     * test_build_ticket: Test the performance of building Tickets by several workers.
     * @param channel Pointer to a ChannelDefault object.
     * @param workers Number of workers threads to be spawned.
     * @param iterations Number of total tickets to be generated. Each worker generates
     * iterations / workers tickets.
     * @param size_range Generate tickets with buffer with variable size. If size_range is 0, the
     * buffer field is not set.
     */
    void test_build_ticket (ChannelDefault* channel,
        const int& workers,
        const long& iterations,
        const uint64_t& size_range,
        const bool& log)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Test ticket builder (%d, %ld)\n", workers, iterations);
        std::fprintf (this->m_fd, "----------------------------\n");

        std::thread worker_threads[workers];

        // lambda function to create tickets
        auto worker_func = ([this, channel] (int iterations, uint64_t size_range, bool log) {
            std::stringstream stream;
            stream << "Thread-" << std::this_thread::get_id () << ": build ticket ...\n";
            auto start = std::chrono::system_clock::now ();

            for (int i = 0; i < iterations; i++) {
                // create default context object
                Context context {};

                // create message with variable size
                uint64_t operation_size = 0;
                unsigned char* message = nullptr;

                if (size_range > 0) {
                    operation_size = random () % size_range;
                    message = new unsigned char[operation_size];
                    std::memset (message, 0x00, sizeof (unsigned char) * operation_size);
                }

                // build Ticket object
                channel->build_ticket (context, message, operation_size);

                // delete message buffer
                delete[] message;
            }

            std::chrono::duration<double> elapsed_time = std::chrono::system_clock::now () - start;

            stream << "Ops:\t" << iterations;
            stream << "\tDuration: " << elapsed_time.count ();
            stream << "\tTicket counter: " << channel->m_ticket_id.load () << "\n";
            stream << "IOPS:\t" << static_cast<double> (iterations) / elapsed_time.count ()
                   << " tickets/s\n";

            if (log) {
                std::fprintf (this->m_fd, "%s\n", stream.str ().c_str ());
            }
        });

        // spawn worker threads
        for (int i = 0; i < workers; i++) {
            worker_threads[i] = std::thread (worker_func, iterations / workers, size_range, log);
        }

        // join worker threads
        for (int i = 0; i < workers; i++) {
            worker_threads[i].join ();
        }

        std::fprintf (this->m_fd,
            "Final ticket counter: %" PRIu64 "\n",
            channel->m_ticket_id.load ());
        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * test_create_enforcement_object: Test creation of simple EnforcementObject. The method try to
     * create two EnforcementObject: the first one is successful, the second should fail since it
     * has the same object identifier.
     * @param channel Pointer to a ChannelDefault object.
     * @param repeat Boolean that defines if the method tries to create an existing object.
     */
    void test_create_enforcement_object (ChannelDefault* channel, const bool& repeat)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd,
            "Test create enforcement object (noop, 1, posix::no_op, posix::no_op,)\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        // create static configuration of an EnforcementObject
        long object_id = 1;
        auto operation_type = static_cast<uint32_t> (POSIX::no_op);
        auto operation_context = static_cast<uint32_t> (POSIX::no_op);
        auto object_type = EnforcementObjectType::NOOP;
        std::vector<long> configurations {};

        // create enforcement object
        PStatus status = this->create_object (channel,
            object_id,
            operation_type,
            operation_context,
            object_type,
            configurations);

        std::fprintf (this->m_fd,
            "create object (%ld) : %s\n",
            object_id,
            status.to_string ().c_str ());

        if (status.is_ok ()) {
            std::fprintf (this->m_fd, "Enforcement object created successfully.\n");
        }

        // create the same enforcement object (should return error)
        if (repeat) {
            status = this->create_object (channel,
                object_id,
                operation_type,
                operation_context,
                object_type,
                configurations);

            std::fprintf (this->m_fd,
                "create object (%ld) : %s\n",
                object_id,
                status.to_string ().c_str ());
        }

        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * test_configure_enforcement_object: Test configuration of EnforcementObject. The method first
     * creates a new EnforcementObject and then configures it according to the config parameter.
     * @param channel Pointer to an ChannelDefault object.
     * @param config Configuration to be set.
     * @param configurations Configuration parameters to be set.
     */
    void test_configure_enforcement_object (ChannelDefault* channel,
        const EnforcementObjectType& object_type,
        const int& config,
        const std::vector<long>& configurations)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd,
            "Test configure enforcement object (%d, %d)\n",
            static_cast<int> (object_type),
            config);
        std::fprintf (this->m_fd, "----------------------------\n");

        // create new object identifier
        long object_id = random ();

        ObjectDifferentiationPair diff_pair { static_cast<uint32_t> (POSIX::write),
            static_cast<uint32_t> (POSIX::no_op) };

        // create enforcement object
        PStatus status = channel->create_enforcement_object (object_id, diff_pair, object_type, {});

        std::fprintf (this->m_fd,
            "create object (%ld) : %s\n",
            object_id,
            status.to_string ().c_str ());

        // configure enforcement object
        status = channel->configure_enforcement_object (object_id, config, configurations);
        std::fprintf (this->m_fd,
            "configure object (%ld) : %s\n",
            object_id,
            status.to_string ().c_str ());

        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * test_channel_enforce: Spawn several threads to enforce the channel's I/O mechanisms
     * simultaneously. The method create a small set of Noop enforcement objects, each with a
     * different pair of operation type and context. Each worker than enforces I/O operations
     * destined towards a single enforcement object.
     * @param workers Number of worker threads to be spawned.
     * @param iterations Cumulative iterations to be made (I/O operations to be enforced).
     */
    void test_channel_enforce (const int& workers,
        const int& iterations,
        const EnforcementObjectType& default_type,
        const std::vector<long>& default_configurations)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd,
            "Testing channel enforce (single channel with multiple EnforcementObjects)\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        // create channel
        ChannelDefault channel {};
        int total_objects = 4;
        std::thread worker_threads[workers];

        // create a small set of EnforcementObjects of type Noop
        // create_object with operation_type "POSIX::pread" and operation_context "POSIX::no_op"
        this->create_object (&channel,
            1,
            static_cast<uint32_t> (POSIX::pread),
            static_cast<uint32_t> (POSIX::no_op),
            default_type,
            default_configurations);
        // create_object with operation_type "POSIX::pwrite" and operation_context "POSIX::no_op"
        this->create_object (&channel,
            2,
            static_cast<uint32_t> (POSIX::pwrite),
            static_cast<uint32_t> (POSIX::no_op),
            default_type,
            default_configurations);
        // create_object with operation_type "POSIX::write" and operation_context "POSIX::no_op"
        this->create_object (&channel,
            3,
            static_cast<uint32_t> (POSIX::write),
            static_cast<uint32_t> (POSIX::no_op),
            default_type,
            default_configurations);
        // create_object with operation_type "POSIX::read" and operation_context "POSIX::no_op"
        this->create_object (&channel,
            4,
            static_cast<uint32_t> (POSIX::read),
            static_cast<uint32_t> (POSIX::no_op),
            default_type,
            default_configurations);

        // lambda function that enforces I/O requests over the channel
        auto worker_func = ([this] (ChannelDefault* channel,
                                int iterations,
                                uint32_t operation_type,
                                uint32_t operation_context) {
            std::stringstream stream;
            stream << "Channel enforce (" << std::this_thread::get_id () << ", ";
            stream << operation_type << ", ";
            stream << operation_context << ")\n";
            auto start = std::chrono::system_clock::now ();

            // enforce I/O request
            this->channel_enforce (channel, iterations, operation_type, operation_context);

            std::chrono::duration<double> elapsed_time = std::chrono::system_clock::now () - start;

            stream << "Ops:\t" << iterations << "\t\tDuration: " << elapsed_time.count () << "\n";
            stream << "IOPS:\t" << (double)iterations / elapsed_time.count () / 1000 << " KOps/s\n";
            std::fprintf (this->m_fd, "%s\n", stream.str ().c_str ());
        });

        // spawn worker threads based on operation type and context
        for (int i = 0; i < workers; i++) {
            uint32_t operation_type;
            uint32_t operation_context;

            switch (i % total_objects) {
                case 0:
                    operation_type = static_cast<uint32_t> (POSIX::pread);
                    operation_context = static_cast<uint32_t> (POSIX::no_op);
                    break;

                case 1:
                    operation_type = static_cast<uint32_t> (POSIX::pwrite);
                    operation_context = static_cast<uint32_t> (POSIX::no_op);
                    break;

                case 2:
                    operation_type = static_cast<uint32_t> (POSIX::write);
                    operation_context = static_cast<uint32_t> (POSIX::no_op);
                    break;

                case 3:
                    operation_type = static_cast<uint32_t> (POSIX::read);
                    operation_context = static_cast<uint32_t> (POSIX::no_op);
                    break;
            }

            worker_threads[i] = std::thread (worker_func,
                &channel,
                iterations / workers,
                operation_type,
                operation_context);
        }

        // joint worker threads
        for (int i = 0; i < workers; i++) {
            worker_threads[i].join ();
        }

        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * test_collect_object_statistics:
     * @param channel
     * @param log
     */
    [[maybe_unused]] void test_collect_object_statistics ([[maybe_unused]] ChannelDefault* channel,
        [[maybe_unused]] const bool& log)
    { }

    /**
     * test_collect_general_statistics: Collect general statistics from the channel's statistics
     * object.
     * @param channel Pointer to a ChannelDefault object.
     * @param log Boolean that defines if log messages should printed to stdout.
     */
    void test_collect_general_statistics (ChannelDefault* channel, const bool& log)
    {
        // create structure to store statistics
        ChannelStatsRaw stats_raw {};
        // collect general statistics from channel
        auto status = channel->collect_general_statistics (stats_raw);

        if (log) {
            std::string message { status.to_string () };
            message.append (": ChannelStatistics collect: {");
            message.append (std::to_string (stats_raw.m_overall_metric_value)).append (", ");
            message.append (std::to_string (stats_raw.m_windowed_metric_value)).append ("}\n");

            std::fprintf (this->m_fd, "%s", message.c_str ());
        }
    }

    /**
     * test_collect_detailed_statistics: Collect detailed statistics for each entry of the channel's
     * ChannelStatistics object.
     * @param channel Pointer to a ChannelDefault object.
     * @param log Boolean that defines if log messages should printed to stdout.
     */
    void test_collect_detailed_statistics (ChannelDefault* channel, const bool& log)
    {
        // create structure to store statistics
        std::vector<double> stats_raw {};
        // collect general statistics from channel
        auto status = channel->collect_detailed_statistics (stats_raw);

        if (log) {
            std::stringstream stream;
            stream << status.to_string () << ": ";
            stream << "ChannelStatistics collect detailed entries: {";
            for (double entry : stats_raw) {
                stream << entry << ", ";
            }
            stream << "}\n";

            std::fprintf (this->m_fd, "%s", stream.str ().c_str ());
        }
    }

    /**
     * test_collect_statistic_entry: Collect a single entry statistic from the channel's
     * ChannelStatistics object.
     * @param channel Pointer to a ChannelDefault object.
     * @param operation Operation entry to be collected. If -1, the method picks an entry at random.
     * @param log Boolean that defines if log messages should printed to stdout.
     */
    void
    test_collect_statistic_entry (ChannelDefault* channel, const int& operation, const bool& log)
    {
        // create structure to store statistics
        ChannelStatsRaw stats_raw {};

        int operation_entry = operation;
        // generate random operation entry
        if (operation == -1) {
            operation_entry = static_cast<int> (random () % 1000);
        }

        // collect statistics of a single entry
        auto status = channel->collect_statistic_entry (stats_raw, operation_entry);

        if (log) {
            std::string message { status.to_string () };
            message.append (": ChannelStatistics collect single entry: {");
            message.append (std::to_string (operation_entry)).append (": ");
            message.append (std::to_string (stats_raw.m_overall_metric_value)).append (", ");
            message.append (std::to_string (stats_raw.m_windowed_metric_value)).append ("}\n");

            std::fprintf (this->m_fd, "%s", message.c_str ());
        }
    }

    /**
     * test_define_object_differentiation:
     */
    [[maybe_unused]] void test_define_object_differentiation ()
    { }

    /**
     *  test_build_differentiation_token:
     */
    [[maybe_unused]] void test_build_differentiation_token ()
    { }
};
} // namespace paio::enforcement

using namespace paio::enforcement;

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

    ChannelDefaultTest test { fd };
    int num_workers = 4;
    long num_tickets = 10000000;
    long num_enforce = 20000000;
    uint64_t operation_size_range = 1024;

    ChannelDefault channel {};
    test.test_build_ticket (&channel, num_workers, num_tickets, operation_size_range, true);
    std::this_thread::sleep_for (std::chrono::seconds (2));

    test.test_create_enforcement_object (&channel, true);
    std::this_thread::sleep_for (std::chrono::seconds (2));

    test.test_configure_enforcement_object (&channel,
        paio::options::EnforcementObjectType::DRL,
        static_cast<int> (DRLConfiguration::rate),
        { 100000 });

    test.test_channel_enforce (num_workers,
        num_enforce,
        paio::options::EnforcementObjectType::NOOP,
        { 1000000, 500000 }); // only useful for the DRL object
}
