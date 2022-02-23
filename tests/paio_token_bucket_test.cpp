/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <cinttypes>
#include <iostream>
#include <paio/enforcement/objects/drl/token_bucket.hpp>

namespace paio::enforcement {

/**
 * TokenBucketTest testing class.
 * TODO:
 *  - support more tests to cover most methods from the TokenBucket class.
 */
class TokenBucketTest {

private:
    FILE* m_fd { stdout };
    std::atomic<bool> m_interrupted { false };

public:
    /**
     * TokenBucketTest default constructor.
     */
    TokenBucketTest () = default;

    /**
     * TokenBucketTest parameterized constructor.
     * @param fd Pointer to file through where log messages will be written.
     */
    explicit TokenBucketTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * TokenBucketTest default destructor.
     */
    ~TokenBucketTest () = default;

    /**
     * token_bucket_consume call. Methods that only consumes tokens from the bucket.
     * @param bucket Pointer to the token-bucket object.
     * @param range The range of tokens to consume each iteration.
     * @param iterations The number of iterations to perform.
     * @param detailed_logs If true, detailed logs will be printed.
     */
    void token_bucket_consume (TokenBucket* bucket,
        const int& range,
        const int& iterations,
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
            message.append (std::to_string (bucket->get_token_count ())).append (",\t");
            message.append (std::to_string (bucket->get_capacity ())).append ("}\t");

            auto partial_start = std::chrono::high_resolution_clock::now ();

            // consume tokens
            bucket->try_consume (tokens);
            tokens_consumed += tokens;

            // get elapsed time
            long elapsed = std::chrono::duration_cast<std::chrono::microseconds> (
                std::chrono::high_resolution_clock::now () - partial_start)
                               .count ();

            // update message with elapsed time to consume tokens
            message.append ("-- ").append (std::to_string (elapsed)).append (" us");

            // write log message
            if (detailed_log) {
                std::fprintf (this->m_fd, "%s\n", message.c_str ());
            }
        }

        auto total_finish = std::chrono::high_resolution_clock::now ();
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
     * test_consume_and_collect: method that consumes and collects statistics from token-bucket. It
     * spawns two threads, one for consuming and other for collecting statistics (TBStats) from the
     * token-bucket.
     * @param bucket Pointer to the token-bucket object.
     * @param range The range of tokens to consume each iteration.
     * @param iterations The number of iterations to perform.
     * @param detailed_log If true, detailed logs will be printed.
     */
    void test_consume_and_collect (TokenBucket* bucket,
        const int& range,
        const int& iterations,
        const uint64_t& collection_period,
        const bool& detailed_log)
    {
        auto consume_func
            = ([this] (TokenBucket* bucket, int range, int iterations, bool detailed_log) {
                  // consume tokens from bucket
                  this->token_bucket_consume (bucket, range, iterations, detailed_log);
                  this->m_interrupted.store (true);
              });

        // Collect Statistics
        ObjectStatisticsRaw statistics_raw {};
        statistics_raw.m_channel_id = 1;
        statistics_raw.m_enforcement_object_id = 1;

        auto collect_func = ([this] (TokenBucket* bucket,
                                 ObjectStatisticsRaw& statistics_raw,
                                 int iterations,
                                 uint64_t collection_period,
                                 bool detailed_log) {
            for (int i = 0; i < iterations && !this->m_interrupted.load (); i++) {
                auto begin = std::chrono::high_resolution_clock::now ();
                // collect statistics
                int total_stats = bucket->try_collect_statistics (statistics_raw);
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

        // spawn consume and collect threads
        std::thread consume_thread { consume_func, bucket, range, iterations, detailed_log };
        std::thread collect_thread { collect_func,
            bucket,
            std::ref (statistics_raw),
            iterations / 10,
            collection_period,
            detailed_log };

        // join consume and collect threads
        consume_thread.join ();
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

    // create TokenBucketTest object
    TokenBucketTest token_bucket_test { fd };
    bool detailed_log = true;
    uint64_t collection_period = 750; // (in milliseconds)

    // create TokenBucket object
    TokenBucket bucket { 5000, 0, 1000000, true, 60000000 };

    token_bucket_test.test_consume_and_collect (&bucket,
        500,
        1000,
        collection_period,
        detailed_log);
}
