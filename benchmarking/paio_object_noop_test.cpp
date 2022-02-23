/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <gflags/gflags.h>
#include <paio/enforcement/objects/noop/enforcement_object_noop.hpp>
#include <paio/enforcement/ticket.hpp>
#include <random>
#include <thread>

namespace paio::enforcement {

/**
 * NoopTest class.
 * Testing class for the Noop EnforcementObject.
 */
class NoopTest {

private:
    FILE* m_fd { stdout };

public:
    /**
     * NoopTest default constructor.
     */
    NoopTest () = default;

    /**
     * NoopTest parameterized constructor.
     * @param fd Pinter to FILE where log messages should be written to.
     */
    explicit NoopTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * NoopTest default destructor.
     */
    ~NoopTest () = default;

    /**
     * test_obj_noop_enforce_without_content: Test the performance of the Noop enforcement object
     * for the obj_enforce method without data content.
     * @param noop Pointer (shared) to the Noop enforcement object.
     * @param iterations Number of rounds to invoke obj_enforce.
     */
    void test_obj_noop_enforce_without_content (const std::shared_ptr<NoopObject>& noop,
        const uint64_t& iterations)
    {
        for (uint64_t i = 1; i <= iterations; i++) {
            // create ticket object
            Ticket ticket {
                static_cast<uint64_t> (i), // ticket-id
                1, // total operations
                1, // payload
                static_cast<int> (PAIO_GENERAL::no_op), // operation type
                static_cast<int> (PAIO_GENERAL::no_op) // operation context
            };

            Result result {};
            // enforce noop obj_enforce call
            noop->obj_enforce (ticket, result);

            // verify ResultStatus after enforcement
            if (result.get_result_status () == ResultStatus::error) {
                std::fprintf (this->m_fd, "Error: obj_enforce returned an error.\n");
            }
        }
    }

    /**
     * test_obj_noop_enforce_with_content: Test the performance of the Noop enforcement object for
     * the obj_enforce method with data content.
     * @param noop Pointer (shared) to the Noop enforcement object.
     * @param iterations Number of rounds to invoke obj_enforce.
     * @param operation_size Size of the buffer to be transferred.
     */
    void test_obj_noop_enforce_with_content (const std::shared_ptr<NoopObject>& noop,
        const uint64_t& iterations,
        const ssize_t& operation_size)
    {
        auto* message = new unsigned char[operation_size];

        for (uint64_t i = 1; i <= iterations; i++) {
            // set message array to zeros
            memset (message, 0x00, sizeof (unsigned char) * operation_size);

            // create ticket object
            Ticket ticket {
                static_cast<uint64_t> (i), // ticket-id
                1, // total operations
                1 * operation_size, // payload
                static_cast<int> (PAIO_GENERAL::no_op), // operation type
                static_cast<int> (PAIO_GENERAL::no_op), // operation context
                static_cast<std::size_t> (operation_size), // operation size
                message // data/content
            };

            Result result {};
            // enforce noop obj_enforce call
            noop->obj_enforce (ticket, result);

            // verify ResultStatus after enforcement
            if (result.get_result_status () == ResultStatus::error) {
                std::fprintf (this->m_fd, "Error: obj_enforce returned an error.\n");
            }
        }

        // delete message buffer
        delete[] message;
    }

    /**
     * test_obj_noop_enforce: Test the performance of the Noop enforcement object.
     * @param noop Pointer (shared) to a Noop enforcement object.
     * @param iterations Number of rounds to invoke obj_enforce.
     * @param buffer_size Size of the buffer to be transferred.
     */
    std::string test_obj_noop_enforce (const std::shared_ptr<NoopObject>& noop,
        const uint64_t& iterations,
        const ssize_t& buffer_size)
    {
        std::chrono::duration<double> elapsed_time {};
        std::stringstream stream;

        if (buffer_size > 0) {
            auto start = std::chrono::system_clock::now ();

            // enforce noop object with content
            this->test_obj_noop_enforce_with_content (noop, iterations, buffer_size);

            // calculate total time elapsed
            elapsed_time = std::chrono::system_clock::now () - start;

            stream << "Test Enforcement Object Noop w/ content [" << buffer_size << "]\n";
        } else {
            auto start = std::chrono::system_clock::now ();

            // enforce noop object without data content
            this->test_obj_noop_enforce_without_content (noop, iterations);

            // calculate total time elapsed
            elapsed_time = std::chrono::system_clock::now () - start;

            stream << "Test Enforcement Object Noop w/o content\n";
        }

        stream << "--------------------------------------------------------\n";
        stream << "Ops:\t" << iterations << "\t\tDuration: " << elapsed_time.count () << "\n";
        stream << "IOPS:\t" << static_cast<double> (iterations) / elapsed_time.count () / 1000
               << " KOps/s\n";
        stream << "------------------------------------------\n";

        stream << "Thr:\t";
        stream << (static_cast<double> (iterations)
                      * (static_cast<double> (buffer_size) / 1024 / 1024))
                / elapsed_time.count ()
               << " MiB/s\t";
        stream << (static_cast<double> (iterations)
                      * (static_cast<double> (buffer_size) / 1024 / 1024 / 1024))
                / elapsed_time.count ()
               << " GiB/s\n";

        stream << "Bw:\t";
        stream << (static_cast<double> (iterations)
            * (static_cast<double> (buffer_size) / 1024 / 1024))
               << " MiB\t";
        stream << (static_cast<double> (iterations)
            * (static_cast<double> (buffer_size) / 1024 / 1024 / 1024))
               << " GiB\n";
        stream << "--------------------------------------------------------\n";

        return stream.str ();
    }

    /**
     * test_obj_noop_enforce_multithreading: Invoke Noop enforcement mechanism over a varied number
     * of workers threads. All threads will be using the same Noop object.
     * @param noop Pointer (shared) to a Noop enforcement object.
     * @param workers Number of workers to invoke obj_enforce.
     * @param iterations Number of rounds to invoke obj_enforce.
     * @param operation_size Size of the buffer to be transferred.
     */
    void test_obj_noop_enforce_multithreading (const std::shared_ptr<NoopObject>& noop,
        const int& workers,
        const uint64_t& iterations,
        const ssize_t& operation_size)
    {
        std::thread total_workers[workers];
        std::string results[workers];

        // lambda function for each worker to execute
        auto func = ([this, &results] (int position,
                         const std::shared_ptr<NoopObject>& obj_noop,
                         const uint64_t& iterations,
                         const ssize_t& operation_size) {
            results[position] = this->test_obj_noop_enforce (obj_noop, iterations, operation_size);
        });

        // spawn workers
        for (int i = 0; i < workers; i++) {
            total_workers[i] = std::thread (func, i, noop, iterations, operation_size);
        }

        // join workers and print to stdout the result
        for (int i = 0; i < workers; i++) {
            total_workers[i].join ();
            std::fprintf (this->m_fd, "--------------------------------------------------------\n");
            std::fprintf (this->m_fd, "Thread-%d: %s\n\n", i, results[i].c_str ());
        }
    }
};
} // namespace paio::enforcement

using namespace paio::enforcement;

DEFINE_string (log_file_path, "", "Defines the path to the log file.");

DEFINE_int64 (ops, 1000000, "Defines the number of enforcement operations to be performed.");

DEFINE_uint32 (threads, 1, "Defines the number of worker threads to use simultaneously.");

DEFINE_int64 (size, 1024, "Defines the I/O size of each operation.");

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

    bool is_shared = (FLAGS_threads > 1);

    // create a Noop enforcement object
    std::shared_ptr<NoopObject> noop_obj = std::make_shared<NoopObject> (1, "noop_test", is_shared);
    // create a NoopTest object
    NoopTest noop_test { fd };

    // enforce Noop mechanism
    noop_test.test_obj_noop_enforce_multithreading (noop_obj,
        static_cast<int> (FLAGS_threads),
        FLAGS_ops,
        FLAGS_size);

    // close log file
    if (!FLAGS_log_file_path.empty ()) {
        std::fclose (fd);
    }
}
