/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/objects/drl/token_bucket.hpp>

namespace paio::enforcement {

// TokenBucket default constructor.
TokenBucket::TokenBucket ()
{
    Logging::log_debug ("TokenBucket default constructor.\n" + this->to_string ());
}

// TokenBucket statistic-only parameterized constructor.
TokenBucket::TokenBucket (const bool& collect_statistics, const uint64_t& sliding_window) :
    m_collect_statistics { collect_statistics },
    m_sliding_window_statistics { sliding_window }
{
    Logging::log_debug ("TokenBucket parameterized constructor.\n" + this->to_string ());
}

// TokenBucket parameterized constructor (fully).
TokenBucket::TokenBucket (const token& capacity,
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
    Logging::log_debug ("TokenBucket parameterized constructor.\n" + this->to_string ());
}

// TokenBucket copy constructor.
TokenBucket::TokenBucket (const TokenBucket& bucket) :
    m_capacity { bucket.m_capacity },
    m_tokens { bucket.m_tokens },
    m_refill_period { bucket.m_refill_period },
    m_next_refill_period { bucket.m_next_refill_period },
    m_last_refill_period { bucket.m_last_refill_period },
    m_collect_statistics { bucket.m_collect_statistics.load () },
    m_token_bucket_statistics { bucket.m_token_bucket_statistics },
    m_sliding_window_statistics { bucket.m_sliding_window_statistics }
{
    Logging::log_debug ("TokenBucket copy constructor.\n" + this->to_string ());
}

// TokenBucket default destructor.
TokenBucket::~TokenBucket () = default;

// TokenBucket operator.
void TokenBucket::operator() ()
{ }

// try_refill call. Try to refill the token-bucket.
void TokenBucket::try_refill (long& time_left)
{
    // verify if it is time to refill the token-bucket.
    time_left = this->is_time_to_refill ();

    // execute refill operation if the time to be replenished has elapsed
    if (time_left <= 0) {
        this->refill_operation ();
    }
}

// consume_operation call. Consume N tokens from the token-bucket.
// If the bucket does not have enough tokens, wait for it to be refilled.
void TokenBucket::consume_operation (const token& consume_tokens)
{
    long time_left;
    // validate if the token-bucket has enough tokens to consume
    while (consume_tokens > this->m_tokens) {
        // try to refill the bucket
        this->try_refill (time_left);

        // if it's not yet the time to refill, sleep ...
        if (time_left > 0) {
            // collect statistics (out of tokens, wait for refill)
            if (this->m_collect_statistics.load ()) {
                this->record_out_of_tokens_stat (
                    time_point_cast<microseconds> (system_clock::now ())
                        .time_since_epoch ()
                        .count ());
            }

            // std::this_thread::sleep_for (microseconds (time_left));
            auto time_to_sleep = static_cast<double> (m_refill_period) * 0.01;
            std::this_thread::sleep_for (microseconds (static_cast<long> (time_to_sleep)));
        }
    }

    // consume tokens
    this->m_tokens -= consume_tokens;
}

// try_consume call. Consume N tokens from the token-bucket.
void TokenBucket::try_consume (const token& consume_tokens)
{
    // validate if the total tokens to consume is larger than the bucket's capacity
    if (consume_tokens > this->m_capacity) {
        token tokens_left = consume_tokens;
        token iteration_tokens = this->m_capacity;

        // continuously consume from the bucket until all requested tokens were serviced
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
int TokenBucket::try_collect_statistics (ObjectStatisticsRaw& statistics_raw)
{
    // before collecting statistics, run garbage collector to remove outdated statistic entries
    this->run_garbage_collector ();

    // collect statistics
    return this->m_token_bucket_statistics.collect_stats (statistics_raw);
}

// get_capacity call. Return the token-bucket's capacity.
token TokenBucket::get_capacity () const
{
    return this->m_capacity;
}

// set_capacity call. Update the token-bucket's capacity.
void TokenBucket::set_capacity (const token& capacity)
{
    this->m_capacity = this->normalize_tokens (capacity);
}

// get_token_count call. Return the current number of tokens in the token-bucket.
token TokenBucket::get_token_count () const
{
    return this->m_tokens;
}

// set_token_count call. Update the token-bucket's current tokens.
void TokenBucket::set_token_count (const token& tokens)
{
    this->m_tokens = this->normalize_tokens (tokens);
}

// get_refill_period call. Return token-bucket's refill period.
long TokenBucket::get_refill_period () const
{
    return this->m_refill_period;
}

// set_refill_period call. Update the refill period.
void TokenBucket::set_refill_period (const long& period)
{
    this->m_refill_period = period;
}

// is_statistic_collection_enabled call. Verify if token-bucket's statistic collection is enabled.
bool TokenBucket::is_statistic_collection_enabled () const
{
    return this->m_collect_statistics.load ();
}

// set_statistic_collection call. Enable/disabled token-bucket statistics collection.
void TokenBucket::set_statistic_collection (const bool& collect)
{
    this->m_collect_statistics.store (collect);
}

// to_string call. Return the token-bucket's variables in string format.
std::string TokenBucket::to_string () const
{
    std::string message { "TokenBucket {" };
    message.append (std::to_string (this->m_capacity)).append (" cap, ");
    message.append (std::to_string (this->m_refill_period)).append (" rp, ");
    message.append (std::to_string (this->m_tokens)).append (" tk, ");
    message.append (std::to_string (this->m_collect_statistics.load ())).append ("}");

    return message;
}

// refill_operation call. Refill the token-bucket.
void TokenBucket::refill_operation ()
{
    // by default, fill the bucket with its maximum capacity
    this->m_tokens = this->m_capacity;

    // validate if statistics collection is enabled
    if (this->m_collect_statistics.load ()) {
        // if enabled, update the last refill period to system_clock::now()
        this->set_last_refill_period ();
    }
}

// is_time_to_refill call. Validates if enough time has elapsed to refill the token-bucket.
long TokenBucket::is_time_to_refill ()
{
    // get current time in microseconds
    auto current_time = time_point_cast<microseconds> (system_clock::now ());

    // compute the difference between the current time and the next refill period
    auto time_left = static_cast<long> (this->m_next_refill_period)
        - current_time.time_since_epoch ().count ();

    // validate if it is time to refill the bucket
    if (time_left <= 0) {
        this->m_next_refill_period = this->calc_next_refill_period (current_time);
    }

    // return the time difference in microseconds (next_refill_period is in nanos)
    return time_left;
}

// calc_next_refill_period call. Auxiliary method for calculating the next refill period.
uint64_t TokenBucket::calc_next_refill_period (
    const time_point<system_clock, microseconds>& time) const
{
    // add the refill time period to a specific time point
    return (time.time_since_epoch ().count () + this->m_refill_period);
}

// convert_seconds_to_microseconds call. Auxiliary method for converting seconds to microseconds.
long TokenBucket::convert_seconds_to_microseconds (const long& value)
{
    return (value * 1000 * 1000);
}

// convert_microseconds_to_seconds call. Auxiliary method for converting microseconds to seconds.
long TokenBucket::convert_microseconds_to_seconds (const long& value)
{
    return (value / 1000 / 1000);
}

// set_last_refill_period call. Update last refill period time.
void TokenBucket::set_last_refill_period ()
{
    // set m_last_refill_period to the current time
    this->m_last_refill_period
        = time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ();
}

// record_out_of_tokens_stat call. Record a new statistic entry indicating that the token-bucket is
// out of tokens, or it does not have enough tokens to serve the consume operation.
void TokenBucket::record_out_of_tokens_stat (const uint64_t& out_of_tokens_time)
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
void TokenBucket::run_garbage_collector ()
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
token TokenBucket::normalize_tokens (const token& throughput) const
{
    // m_refill_period should be normalized to seconds, since throughput is given in bytes/s or IOPS
    return throughput * (static_cast<double> (this->m_refill_period) / 1000 / 1000);
}
} // namespace paio::enforcement
