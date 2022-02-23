/**
 *  Written by Ricardo Macedo.
 *  Copyright (c) 2020-2022 INESC TEC.
 **/

#include <chrono>
#include <gflags/gflags.h>
#include <iostream>
#include <paio/differentiation/differentiation_token_builder.hpp>
#include <paio/differentiation/murmurhash_token_builder.hpp>
#include <paio/options/options.hpp>
#include <string>

using namespace paio::core;
using namespace paio::differentiation;

namespace paio {

class MurmurHash3Test {

private:
    /**
     * log_results: print performance report to a given file descriptor (including stdout).
     * @param fd Pointer to a FILE object to write the performance report.
     * @param scheme Hashing scheme to select the header of the performance report.
     * @param iterations Number of operations performed.
     * @param elapsed_seconds Total time (in seconds) elapsed.
     */
    void log_results (FILE* fd,
        const HashingScheme& scheme,
        const long& iterations,
        const duration<double>& elapsed_seconds)
    {
        std::string header {};
        switch (scheme) {
            case HashingScheme::MurmurHash_x86_32:
                header.append ("Performance of MurmurHash3 x86-32:");
                break;

            case HashingScheme::MurmurHash_x86_128:
                header.append ("Performance of MurmurHash3 x86-128:");
                break;

            case HashingScheme::MurmurHash_x64_128:
                header.append ("Performance of MurmurHash3 x64-128:");
                break;

            default:
                return;
        }

        std::fprintf (fd, "\n-----------------------------------\n");
        std::fprintf (fd, "%s\n", header.c_str ());
        std::fprintf (fd, "-----------------------------------\n");
        std::fprintf (fd, "Ops: %ld;\t Duration: %.3f\n", iterations, elapsed_seconds.count ());
        std::fprintf (fd,
            "Thr: %.1f KOps/s\n",
            static_cast<double> (iterations) / elapsed_seconds.count () / 1000);
        std::fprintf (fd,
            "Lat: %.3e\n",
            (elapsed_seconds.count () / static_cast<double> (iterations)));
        std::fprintf (fd, "-----------------------------------\n\n");
        std::fflush (fd);
    }

    /**
     * test_murmurhash3_x86_32: Generate a 32-bit Murmur hash.
     * @param fd Pointer to a FILE object to write the performance report.
     * @param msg Message to be hashed.
     * @param iterations Number of operations to be performed.
     * @param detailed Flag that defines if the hashing results of each iteration should be printed
     * to stdout.
     */
    void test_murmurhash3_x86_32 (FILE* fd,
        const std::string& msg,
        const long& iterations,
        bool detailed)
    {
        // create differentiation token builder (w/ MurmurHash x86_32)
        std::unique_ptr<DifferentiationTokenBuilder> builder {
            std::make_unique<MurmurHashTokenBuilder> (HashingScheme::MurmurHash_x86_32)
        };

        auto start = std::chrono::system_clock::now ();

        uint32_t operation_hash;
        for (int i = 0; i < iterations; i++) {
            std::string content = msg + std::to_string (i);

            // apply the differentiation token builder
            builder->generate_differentiation_token (content.c_str (),
                static_cast<int> (content.size ()),
                &operation_hash);

            // print detailed hashing result
            if (detailed) {
                std::fprintf (fd,
                    "murmurhash3_x86_32: %u -- %ld\n",
                    operation_hash,
                    sizeof (operation_hash));
            }
        }

        auto end = std::chrono::system_clock::now ();
        std::chrono::duration<double> elapsed_seconds = end - start;

        // log performance results
        this->log_results (fd, HashingScheme::MurmurHash_x86_32, iterations, elapsed_seconds);
    }

    /**
     * test_murmurhash3_x86_128:
     * @param fd Pointer to a FILE object to write the performance report.
     * @param msg Message to be hashed.
     * @param iterations Number of operations to be performed.
     * @param detailed Flag that defines if the hashing results of each iteration should be printed
     * to stdout.
     */
    void test_murmurhash3_x86_128 (FILE* fd,
        const std::string& msg,
        const long& iterations,
        bool detailed)
    {
        // create differentiation token builder (w/ MurmurHash x86_128)
        std::unique_ptr<DifferentiationTokenBuilder> builder {
            std::make_unique<MurmurHashTokenBuilder> (HashingScheme::MurmurHash_x86_128)
        };

        auto start = std::chrono::system_clock::now ();

        uint32_t operation_hash[4] { 0 };
        for (int i = 0; i < iterations; i++) {
            std::string message = msg + std::to_string (i);

            // apply the differentiation token builder
            builder->generate_differentiation_token (message.c_str (),
                static_cast<int> (message.size ()),
                &operation_hash);

            if (detailed) {
                for (int j = 0; j < 4; j++) {
                    std::fprintf (fd,
                        "hash[%d]: %u -- %ld\n",
                        j,
                        operation_hash[j],
                        sizeof (operation_hash[j]));
                }
                std::fprintf (fd, "\n");
            }
        }

        auto end = std::chrono::system_clock::now ();
        std::chrono::duration<double> elapsed_seconds = end - start;

        // log performance results
        this->log_results (fd, HashingScheme::MurmurHash_x86_128, iterations, elapsed_seconds);
    }

    /**
     * test_murmurhash3_x64_128:
     * @param fd Pointer to a FILE object to write the performance report.
     * @param msg Message to be hashed.
     * @param iterations Number of operations to be performed.
     * @param detailed Flag that defines if the hashing results of each iteration should be printed
     * to stdout.
     */
    void test_murmurhash3_x64_128 (FILE* fd,
        const std::string& msg,
        const long& iterations,
        bool detailed)
    {
        // create differentiation token builder (w/ MurmurHash x64_128)
        std::unique_ptr<DifferentiationTokenBuilder> builder {
            std::make_unique<MurmurHashTokenBuilder> (HashingScheme::MurmurHash_x64_128)
        };

        auto start = std::chrono::system_clock::now ();

        uint32_t operation_hash[4] { 0 };
        for (int i = 0; i < iterations; i++) {
            std::string message = msg + std::to_string (i);

            // apply the differentiation token builder
            builder->generate_differentiation_token (message.c_str (),
                static_cast<int> (message.size ()),
                &operation_hash);

            if (detailed) {
                for (int j = 0; j < 4; j++) {
                    std::fprintf (fd,
                        "hash[%d]: %u -- %ld\n",
                        j,
                        operation_hash[j],
                        sizeof (operation_hash[j]));
                }
                std::fprintf (fd, "\n");
            }
        }

        auto end = std::chrono::system_clock::now ();
        std::chrono::duration<double> elapsed_seconds = end - start;

        // log performance results
        this->log_results (fd, HashingScheme::MurmurHash_x64_128, iterations, elapsed_seconds);
    }

public:
    /**
     * build_message: build message with all classifiers to be hashed.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @return Returns a concatenated string with all classifiers.
     */
    std::string build_message (const uint32_t& workflow_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context)
    {
        std::string message_token { std::to_string (workflow_id) };
        message_token.append ("|").append (std::to_string (operation_type));
        message_token.append ("|").append (std::to_string (operation_context));
        return message_token;
    }

    /**
     * test_murmurhash: Select hashing scheme and conduct the respective performance test.
     * @param scheme Hashing scheme to be used. Only supports murmurhash types.
     * @param fd Pointer to a FILE object to write the performance report.
     * @param msg Message to be hashed.
     * @param iterations Number of operations to be performed.
     * @param detailed Flag that defines if the hashing results of each iteration should be printed
     * to stdout.
     */
    void test_murmurhash (const HashingScheme& scheme,
        FILE* fd,
        const std::string& message,
        const long& iterations,
        bool detailed)
    {
        switch (scheme) {
            case HashingScheme::MurmurHash_x86_32:
                this->test_murmurhash3_x86_32 (fd, message, iterations, detailed);
                break;

            case HashingScheme::MurmurHash_x86_128:
                this->test_murmurhash3_x86_128 (fd, message, iterations, detailed);
                break;

            case HashingScheme::MurmurHash_x64_128:
                this->test_murmurhash3_x64_128 (fd, message, iterations, detailed);
                break;

            default:
                std::cerr << "Murmurhash not supported.\n";
        }
    }
};
} // namespace paio

DEFINE_string (hash,
    "murmurhash_x86_32",
    "Defines the murmur hashing scheme to be used in the performance results.");

DEFINE_bool (detailed_log,
    false,
    "Defines if the result of each hashing round should be printed to stdout.");

DEFINE_int64 (ops, 1000000, "Defines the number of hashing operations to be performed.");

DEFINE_string (log_file_path, "", "Defines the path to the log file.");

int main (int argc, char** argv)
{
    // MurmurHash3 testing class object
    paio::MurmurHash3Test murmurtest {};

    // parse flags from stdin
    gflags::ParseCommandLineFlags (&argc, &argv, true);

    FILE* fd = stdout;
    if (!FLAGS_log_file_path.empty ()) {
        fd = std::fopen (FLAGS_log_file_path.c_str (), "w");

        if (fd == nullptr) {
            fd = stdout;
            std::fprintf (fd,
                "Error while opening log file %s. Writing to stdout\n",
                FLAGS_log_file_path.c_str ());
        }
    }

    // parse hashing scheme
    HashingScheme scheme {};
    if (FLAGS_hash == "murmurhash_x86_32") {
        scheme = HashingScheme::MurmurHash_x86_32;
    } else if (FLAGS_hash == "murmurhash_x86_128") {
        scheme = HashingScheme::MurmurHash_x86_128;
    } else if (FLAGS_hash == "murmurhash_x64_128") {
        scheme = HashingScheme::MurmurHash_x64_128;
    }

    // build default message
    std::string message = murmurtest.build_message (0,
        static_cast<uint32_t> (PAIO_GENERAL::no_op),
        static_cast<uint32_t> (PAIO_GENERAL::no_op));

    // run murmurhash performance test
    murmurtest.test_murmurhash (scheme,
        fd,
        message,
        static_cast<long> (FLAGS_ops),
        FLAGS_detailed_log);

    // close log file
    if (!FLAGS_log_file_path.empty ()) {
        std::fclose (fd);
    }
}
