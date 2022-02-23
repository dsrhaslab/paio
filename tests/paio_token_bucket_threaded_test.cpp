/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <cinttypes>
#include <paio/enforcement/objects/drl/token_bucket_threaded.hpp>
#include <paio/options/options.hpp>
#include <thread>

namespace paio::enforcement {

/**
 * TokenBucketThreadedTest testing class.
 * TODO:
 *  - support more tests to cover most methods from the TokenBucketThreaded class.
 */
class TokenBucketThreadedTest {

private:
    FILE* m_fd { stdout };

    /**
     * token_bucket_consume call. Methods that only consumes tokens from the bucket.
     * @param bucket Pointer to the token-bucket object.
     * @param range The range of tokens to consume each iteration.
     * @param iterations The number of iterations to perform.
     * @param consume_period The period of time, in microseconds, to consume tokens.
     * @param detailed_logs If true, detailed logs will be printed.
     */
    void token_bucket_consume (TokenBucketThreaded& bucket,
        const int& range,
        const int& iterations,
        const uint64_t& consume_period,
        const bool& detailed_log)
    {
        double tokens_consumed = 0;
        auto total_start = std::chrono::high_resolution_clock::now ();

        for (int i = 0; i < iterations; i++) {
            // generate random value of tokens to consume
            auto tokens = static_cast<double> (random () % range);

            // create message w/ tokens to consume and token-bucket info
            std::string message { "[" };
            message.append (std::to_string (i)).append ("] try_consume (");
            message.append (std::to_string (tokens)).append (")\t{");
            message.append (std::to_string (bucket.get_token_count ())).append (",\t");
            message.append (std::to_string (bucket.get_capacity ())).append ("}\t");

            auto partial_start = std::chrono::high_resolution_clock::now ();

            // consume tokens
            bucket.try_consume (tokens);
            tokens_consumed += tokens;

            long elapsed = std::chrono::duration_cast<std::chrono::microseconds> (
                std::chrono::high_resolution_clock::now () - partial_start)
                               .count ();

            // update message with elapsed time to consume tokens
            message.append ("-- ").append (std::to_string (elapsed)).append (" us");

            // write log message
            if (detailed_log) {
                std::fprintf (this->m_fd, "%s\n", message.c_str ());
            }

            std::this_thread::sleep_for (std::chrono::microseconds (consume_period));
        }

        auto total_finish = std::chrono::high_resolution_clock::now ();

        // interrupt background refill thread
        bucket.set_interrupted (true);

        long duration_in_seconds
            = std::chrono::duration_cast<std::chrono::microseconds> (total_finish - total_start)
                  .count ();

        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Tokens consumed: %.3f\n", tokens_consumed);
        std::fprintf (this->m_fd, "Elapsed time (us): %ld\n", duration_in_seconds);
        std::fprintf (this->m_fd, "Elapsed time (s): %ld\n", (duration_in_seconds / 1000 / 1000));
        std::fprintf (this->m_fd,
            "Throughput: %.3f tokens/s\n",
            tokens_consumed / (static_cast<double> (duration_in_seconds) / 1000 / 1000));
        std::fprintf (this->m_fd, "----------------------------\n");
    }

    /**
     * token_bucket_change_refill_period call. Methods that only changes the refill period
     * @param bucket Pointer to the token-bucket object.
     * @param refill_period Period in milliseconds for setting a new value.
     * @param range The range of refill time to be set in each iteration.
     * @param finished Pointer to an atomic boolean that marks when to stop the execution.
     * @param detailed_log If true, detailed logs will be printed.
     */
    [[maybe_unused]] void token_bucket_refill (TokenBucketThreaded* bucket,
        const uint64_t& refill_period,
        const int& range,
        std::atomic<bool>* finished,
        const bool& detailed_log)
    {
        while (finished->load ()) {
            std::this_thread::sleep_for (std::chrono::milliseconds (refill_period));
            // Generate a random refill period
            bucket->set_refill_period ((random () % range));

            // write log message
            if (detailed_log) {
                std::string message { std::to_string (bucket->get_token_count ()) };
                message.append (" tokens - ");
                message.append (std::to_string (bucket->get_refill_period ())).append (" us");

                std::fprintf (this->m_fd, "%s\n", message.c_str ());
            }
        }
    }

public:
    /**
     * TokenBucketThreadedTest default constructor.
     */
    TokenBucketThreadedTest () = default;

    /**
     * TokenBucketThreadedTest parameterized constructor.
     * @param fd Pointer to file through where log messages will be written.
     */
    explicit TokenBucketThreadedTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * TokenBucketThreadedTest default destructor.
     */
    ~TokenBucketThreadedTest () = default;

    /**
     * test_consume_and_refill call. Method that simulates the token-bucket regular
     * use. This method spawns two threads: one that consumes tokens and one that changes the refill
     * period.
     * @param bucket Pointer to the token-bucket object.
     * @param consume_range The range of tokens to consume in each iteration.
     * @param iterations The number of iterations to be executed.
     * @param consume_period The period in microseconds for consuming tokens.
     * @param detailed_log If true, detailed logs will be printed.
     */
    void test_consume_and_refill (TokenBucketThreaded& bucket,
        const int& consume_range,
        const int& iterations,
        const uint64_t& consume_period,
        const bool& detailed_log)
    {
        auto consume_func = ([this] (TokenBucketThreaded& bucket,
                                 const int& range,
                                 const int& iterations,
                                 const uint64_t& consume_period,
                                 const bool& detailed_log) {
            // consume tokens from bucket
            this->token_bucket_consume (bucket, range, iterations, consume_period, detailed_log);
        });

        auto start = std::chrono::system_clock::now ();

        // Spawn consume and refill threads
        std::thread consume_thread (consume_func,
            std::ref (bucket),
            consume_range,
            iterations,
            consume_period,
            detailed_log);
        // thread will use the operator function of TokenBucketThreaded class
        std::thread refill_thread = std::thread (std::ref (bucket));

        // Join consume and refill threads
        consume_thread.join ();
        refill_thread.join ();

        auto end = std::chrono::system_clock::now ();

        std::chrono::duration<double> elapsed_seconds_t = end - start;
        std::fprintf (this->m_fd,
            "test_token_bucket_consume_and_refill: elapsed time: %f\n",
            elapsed_seconds_t.count ());
    }

    /**
     * test_consume_refill_and_collect: method that consumes and collects statistics from
     * token-bucket. It spawns two threads, one for consuming and other for collecting statistics
     * (TBStats) from the token-bucket.
     * @param bucket Pointer to the token-bucket object.
     * @param range The range of tokens to consume each iteration.
     * @param iterations The number of iterations to perform.
     * @param detailed_log If true, detailed logs will be printed.
     */
    void test_consume_refill_and_collect (TokenBucketThreaded& bucket,
        const int& consume_range,
        const int& iterations,
        const uint64_t& consume_period,
        const uint64_t& collection_period,
        const bool& detailed_log)
    {
        auto consume_func = ([this] (TokenBucketThreaded& bucket,
                                 const int& range,
                                 const int& iterations,
                                 const uint64_t& consume_period,
                                 const bool& detailed_log) {
            // consume tokens from bucket
            this->token_bucket_consume (bucket, range, iterations, consume_period, detailed_log);
        });

        // Collect Statistics
        ObjectStatisticsRaw statistics_raw {};
        statistics_raw.m_channel_id = 1;
        statistics_raw.m_enforcement_object_id = 1;

        auto collect_func = ([this] (TokenBucketThreaded& bucket,
                                 ObjectStatisticsRaw& statistics_raw,
                                 int iterations,
                                 uint64_t collection_period,
                                 bool detailed_log) {
            for (int i = 0; i < iterations; i++) {
                auto begin = std::chrono::high_resolution_clock::now ();
                // collect statistics
                int total_stats = bucket.try_collect_statistics (statistics_raw);
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds> (
                    std::chrono::high_resolution_clock::now () - begin)
                                   .count ();

                if (detailed_log) {
                    std::fprintf (this->m_fd,
                        "TBStats [%d]: %d stats collected in  %" PRId64 " us\n",
                        i,
                        total_stats,
                        std::int64_t { elapsed });
                }

                // sleep for 1 second before moving to the next iteration
                std::this_thread::sleep_for (std::chrono::milliseconds (collection_period));
            }
        });

        // Spawn consume, refill, and collect threads
        std::thread consume_thread { consume_func,
            std::ref (bucket),
            consume_range,
            iterations,
            consume_period,
            detailed_log };
        // thread will use the operator function of TokenBucketThreaded class
        std::thread refill_thread = std::thread (std::ref (bucket));
        std::thread collect_thread { collect_func,
            std::ref (bucket),
            std::ref (statistics_raw),
            iterations / 10,
            collection_period,
            detailed_log };

        // join consume, refill, and collect threads
        consume_thread.join ();
        refill_thread.join ();
        collect_thread.join ();
    }
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

    // testing values
    int consume_range { 100 };
    int iterations { 100 };
    uint64_t consume_period { 100000 }; // in microseconds
    uint64_t collection_period { 750 }; // in milliseconds
    bool detailed_log { true };

    bool test_with_stats { false };

    TokenBucketThreadedTest test { fd };
    TokenBucketThreaded bucket { 5000, 5000, 1000000, test_with_stats, 60000000 };

    test_with_stats ? test.test_consume_and_refill (bucket,
        consume_range,
        iterations,
        consume_period,
        detailed_log)
                    : test.test_consume_refill_and_collect (bucket,
                        consume_range,
                        iterations,
                        consume_period,
                        collection_period,
                        detailed_log);
}
