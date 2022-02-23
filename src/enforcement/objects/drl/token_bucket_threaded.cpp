/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/objects/drl/token_bucket_threaded.hpp>

namespace paio::enforcement {

// TokenBucketThreaded default constructor.
TokenBucketThreaded::TokenBucketThreaded ()
{
    Logging::log_debug ("TokenBucketThreaded default constructor.\n" + this->to_string ());
}

// TokenBucketThreaded statistic-only parameterized constructor.
TokenBucketThreaded::TokenBucketThreaded (const bool& collect_statistics,
    const uint64_t& sliding_window) :
    m_collect_statistics { collect_statistics },
    m_sliding_window_statistics { sliding_window }
{
    Logging::log_debug ("TokenBucketThreaded parameterized constructor.\n" + this->to_string ());
}

// TokenBucketThreaded parameterized constructor (fully).
TokenBucketThreaded::TokenBucketThreaded (const token& capacity,
    const token& tokens,
    const long& refill_period,
    const bool& collect_statistics,
    const uint64_t& sliding_window) :
    m_capacity { capacity },
    m_tokens { tokens },
    m_refill_period { refill_period },
    m_collect_statistics { collect_statistics },
    m_sliding_window_statistics { sliding_window }
{
    Logging::log_debug ("TokenBucketThreaded parameterized constructor.\n" + this->to_string ());
}

// TokenBucketThreaded copy constructor.
TokenBucketThreaded::TokenBucketThreaded (const TokenBucketThreaded& bucket) :
    m_capacity { bucket.m_capacity },
    m_tokens { bucket.m_tokens },
    m_refill_period { bucket.m_refill_period },
    m_last_refill_period { bucket.m_last_refill_period },
    m_collect_statistics { bucket.m_collect_statistics.load () },
    m_token_bucket_statistics { bucket.m_token_bucket_statistics },
    m_sliding_window_statistics { bucket.m_sliding_window_statistics }
{
    Logging::log_debug ("TokenBucketThreaded copy constructor.\n" + this->to_string ());
}

// TokenBucketThreaded default destructor.
TokenBucketThreaded::~TokenBucketThreaded ()
{
    this->set_interrupted (true);
}

// Operator call. To run on thread mode.
void TokenBucketThreaded::operator() ()
{
    Logging::log_debug ("TokenBucketThreaded:: operating in thread mode.");
    // call try_refill to spawn a background thread to refill the bucket.
    long time_left;
    this->try_refill (time_left);
}

// try_refill call. Try to refill the token-bucket.
void TokenBucketThreaded::try_refill ([[maybe_unused]] long& time_left)
{
    Logging::log_debug ("Entering in refill loop ...");
    // enter in endless loop that sleeps and refills the bucket, until the system exits.
    while (!this->get_interrupted ()) {
        // sleep for refill_period time.
        std::this_thread::sleep_for (microseconds (this->m_refill_period));
        {
            std::unique_lock<std::mutex> lock (this->m_refill_mutex);
            // execute refill operation.
            this->refill_operation ();
            // notify consumer thread
            this->m_refill_condition.notify_all ();
        }
    }
}

// consume_operation call. Consume N tokens from the token-bucket.
// If the bucket does not have enough tokens, wait for it to be refilled.
void TokenBucketThreaded::consume_operation (const token& consume_tokens)
{
    std::unique_lock<std::mutex> lock (this->m_refill_mutex);
    // validate if there are enough tokens to consume, otherwise wait until tokens become available
    while (consume_tokens > this->m_tokens) {
        // collect statistics (out of tokens, wait for refill)
        if (this->m_collect_statistics.load ()) {
            record_out_of_tokens_stat (
                time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ());
        }

        // wait for enough tokens to be available
        std::cv_status condition_status
            = this->m_refill_condition.wait_for (lock, microseconds (this->get_refill_period ()));

        // validate if whether a timeout was triggered or the condition was satisfied, and if the
        // system is still running, to prevent staying blocked
        if (condition_status == std::cv_status::timeout && this->get_interrupted ()) {
            break;
        }
    }

    // consume tokens
    this->m_tokens -= consume_tokens;
}

// try_consume call. Consume N tokens from the token-bucket.
void TokenBucketThreaded::try_consume (const token& consume_tokens)
{
    // validate if the total tokens to consume is larger than the bucket's capacity
    if (consume_tokens > this->m_capacity) {
        token tokens_left = consume_tokens;
        token iteration_tokens = this->m_capacity;

        // continuously consume from the bucket until all requested tokens were served
        while (tokens_left > 0) {
            // consume tokens
            this->consume_operation (iteration_tokens);
            // update remaining tokens to serve
            tokens_left -= iteration_tokens;

            if (tokens_left < this->m_capacity) {
                iteration_tokens = tokens_left;
            }
        }
    } else {
        // if the number of tokens is lower than the bucket's capacity, consume
        this->consume_operation (consume_tokens);
    }
}

// try_collect_statistics call. Collect statistics from the token-bucket's TBStats object.
int TokenBucketThreaded::try_collect_statistics (ObjectStatisticsRaw& statistics_raw)
{
    // before collecting statistics, run garbage collector to remove outdated statistic entries
    this->run_garbage_collector ();

    // collect statistics
    return this->m_token_bucket_statistics.collect_stats (statistics_raw);
}

// get_capacity call. Return the token-bucket's capacity.
token TokenBucketThreaded::get_capacity () const
{
    return this->m_capacity;
}

// set_capacity call. Update the token-bucket's capacity.
void TokenBucketThreaded::set_capacity (const token& capacity)
{
    this->m_capacity = this->normalize_tokens (capacity);
}

// get_token_count call. Return the current number of tokens in the token-bucket.
token TokenBucketThreaded::get_token_count () const
{
    return this->m_tokens;
}

// set_token_count call. Update the token-bucket's current tokens.
void TokenBucketThreaded::set_token_count (const token& tokens)
{
    this->m_tokens = normalize_tokens (tokens);
}

// get_refill_period call. Return token-bucket's refill period.
long TokenBucketThreaded::get_refill_period () const
{
    return this->m_refill_period;
}

// set_refill_period call. Update the refill period.
void TokenBucketThreaded::set_refill_period (const long& period)
{
    this->m_refill_period = period;
}

// is_statistic_collection_enabled call. Verify if token-bucket's statistic collection is enabled.
bool TokenBucketThreaded::is_statistic_collection_enabled () const
{
    return this->m_collect_statistics.load ();
}

// set_statistic_collection call. Enable/disabled token-bucket statistics collection.
void TokenBucketThreaded::set_statistic_collection (const bool& collect)
{
    this->m_collect_statistics.store (collect);
}

// to_string call. Return the token-bucket's variables in string format.
std::string TokenBucketThreaded::to_string () const
{
    std::string message { "TokenBucket {" };
    message.append (std::to_string (this->m_capacity)).append (" capacity, ");
    message.append (std::to_string (this->m_refill_period)).append (" refill period, ");
    message.append (std::to_string (this->m_tokens)).append (" tokens, ");
    message.append (std::to_string (this->m_collect_statistics.load ())).append ("}");

    return message;
}

// refill_operation call. Refill the token-bucket.
void TokenBucketThreaded::refill_operation ()
{
    // by default, fill the bucket with its maximum capacity
    this->m_tokens = this->m_capacity;

    // validate if statistics collection is enabled
    if (this->m_collect_statistics.load ()) {
        // is enabled, update the last refill period to system_clock::now()
        this->set_last_refill_period ();
    }
}

// convert_seconds_to_microseconds call. Auxiliary method for converting a time value of seconds
// into microseconds.
long TokenBucketThreaded::convert_seconds_to_microseconds (const long& value)
{
    return (value * 1000 * 1000);
}

// convert_microseconds_to_seconds call. Auxiliary method for converting a microseconds to seconds.
long TokenBucketThreaded::convert_microseconds_to_seconds (const long& value)
{
    return (value / 1000 / 1000);
}

// set_last_refill_period call. Update last refill period time.
void TokenBucketThreaded::set_last_refill_period ()
{
    this->m_last_refill_period
        = time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ();
}

// record_out_of_tokens_stat call. Record a new statistic entry indicating that the token-bucket is
// out of tokens, or it does not have enough tokens to serve the consume operation.
void TokenBucketThreaded::record_out_of_tokens_stat (const uint64_t& out_of_tokens_time)
{
    // calculate approximate next refill period
    uint64_t approximate_next_refill_period = this->m_last_refill_period + this->m_refill_period;

    // calculate normalized empty bucket time
    float normalized_empty_factor
        = static_cast<float> (out_of_tokens_time - this->m_last_refill_period)
        / static_cast<float> (approximate_next_refill_period - this->m_last_refill_period);

    // validate that normalized factor is less than or equal to 1
    if (normalized_empty_factor <= 1) {
        // register {normalized, tokens} tuple
        this->m_token_bucket_statistics.store_stats_entry (normalized_empty_factor, this->m_tokens);
    } else {
        // register {normalized, tokens} tuple
        this->m_token_bucket_statistics.store_stats_entry (1, this->m_tokens);
        Logging::log_error ("Normalized empty factor > 1. Something is wrong ...");
    }
}

// run_garbage_collector call. Execute the garbage collection algorithm to remove/discard outdated
// statistic entries.
void TokenBucketThreaded::run_garbage_collector ()
{
    // register current time
    uint64_t now = static_cast<uint64_t> (
        time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ());

    // run garbage collection algorithm of the TBStats object
    int discarded_entries = this->m_token_bucket_statistics.garbage_collection (now,
        this->m_sliding_window_statistics);

    // build log message
    std::string discarded_entries_str { "runGarbageCollection: discarded statistic entries -- " };
    discarded_entries_str.append (std::to_string (discarded_entries)).append (" --  ");
    discarded_entries_str.append (std::to_string (this->m_sliding_window_statistics)).append ("us");

    Logging::log_debug (discarded_entries_str);
}

// normalize_tokens call. Calculate the number of tokens to match a given throughput.
token TokenBucketThreaded::normalize_tokens (const token& throughput) const
{
    // m_refill_period should be normalized to seconds, since throughput is given in bytes/s or IOPS
    return throughput * (static_cast<double> (this->m_refill_period) / 1000 / 1000);
}

// get_interrupted call. Get the current value of the m_interrupted atomic instance.
bool TokenBucketThreaded::get_interrupted () const
{
    return this->m_interrupted.load ();
}

// set_interrupted call. Set a new value for the m_interrupted atomic instance.
void TokenBucketThreaded::set_interrupted (bool value)
{
    this->m_interrupted.store (value);
}

} // namespace paio::enforcement
