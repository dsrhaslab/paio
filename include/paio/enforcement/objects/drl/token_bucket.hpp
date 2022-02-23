/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_TOKEN_BUCKET_HPP
#define PAIO_TOKEN_BUCKET_HPP

#include <chrono>
#include <cstdio>
#include <paio/enforcement/objects/drl/enforcement_object_drl_options.hpp>
#include <paio/enforcement/objects/drl/token_bucket_statistics.hpp>
#include <thread>

using namespace std::chrono;

typedef double token;

namespace paio::enforcement {

/**
 * TokenBucket class.
 * A token-bucket is a rate limiting algorithm used to define limits on the bandwidth and burstiness
 * of I/O requests. Specifically, it can be used to regulate the performance at which requests are
 * serviced and convert bursty workloads into controlled I/O rates.
 * The bucket is configured with a bucket size (m_capacity), which delimits the maximum token
 * capacity, and a refill period (m_refill_period), that defines the period to replenish the bucket.
 * Upon each consume operation, the bucket computes if the time to replenish the bucket was
 * achieved (m_next_refill_period). By default, the time unit is set to microseconds
 * (1s == 1 000 000 us).
 * The TokenBucket class is made of several variables:
 *  - m_capacity: defines the size of the bucket, and delimits the maximum token capacity and rate;
 *  - m_tokens: defines the number of tokens in the bucket at a given time (in tokens);
 *  - m_refill_period: defines the time interval, in microseconds, to refill the bucket;
 *  - m_next_refill_period: defines the time period, in microseconds, of the next refill period;
 *  - m_last_refill_period: defines the time period, in microseconds, of the last refill period;
 *  - m_collect_statistics: atomic variable that defines if statistic collection should be made at
 *  the TokenBucket class (enabled = true; disabled = false); set false by default.
 *  - m_token_bucket_statistics: defines a TBStats object to store collect statistics;
 *  - m_sliding_window_statistics: defines a time period/sliding window that determines which
 *  statistics are valid.
 *  TODO:
 *   - change structs to specific/dedicated objects in try_collect_statistics;
 *   - try_collect_statistics methods needs further testing and validation;
 *   - possible issue in record_out_of_tokens_stat method.
 */
class TokenBucket {
    // friend classes of TokenBucket
    friend class TokenBucketTest;

private:
    token m_capacity { 50000 };
    token m_tokens { 50000 };
    long m_refill_period { 1000000 };
    uint64_t m_next_refill_period { this->calc_next_refill_period (
        time_point_cast<microseconds> (system_clock::now ())) };
    uint64_t m_last_refill_period { static_cast<uint64_t> (
        time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ()) };

    // statistic collection variables
    std::atomic<bool> m_collect_statistics { drl_option_collect_statistics };
    TBStats m_token_bucket_statistics {};
    uint64_t m_sliding_window_statistics { 5000000 };

    /**
     * consume_operation: Operation to consume tokens from the token-bucket.
     * If there not enough tokens are available in the bucket, it computes the remaining time to
     * refill it and sleeps.
     * Used in try_consume.
     * @param consume_tokens Total of tokens to consume.
     */
    void consume_operation (const token& consume_tokens);

    /**
     * refill_operation: Operation to refill the token-bucket.
     * If the number of existing tokens + refill_size value exceeds the m_capacity, the number of
     * m_tokens equals to m_capacity. Otherwise adds the refill_size_ (in tokens) to the bucket.
     * Used in try_refill.
     */
    void refill_operation ();

    /**
     * is_time_to_refill: Verifies if enough time has elapsed to refill the token-bucket, returning
     * the total of microseconds that either remain or have passed m_refill_period.
     * Used in try_refill.
     * @return Return time left to refill (>0) or the time that has already passed after refill
     * (<0) in microseconds.
     */
    [[nodiscard]] long is_time_to_refill ();

    /**
     * convert_seconds_to_microseconds: Convert a given time value from seconds to microseconds.
     * @return Returns the result of the conversion.
     */
    [[maybe_unused]] long convert_seconds_to_microseconds (const long& value);

    /**
     * convert_microseconds_to_seconds: Convert a given time value from microseconds to seconds.
     * @return Returns the result of the conversion.
     */
    [[maybe_unused]] long convert_microseconds_to_seconds (const long& value);

    /**
     * calc_next_refill_period: Calculate when is the token-bucket's next refill period.
     * It adds m_refill_period to the specified time point.
     * This value will be used to arithmetically perform the refill operation.
     * Used in is_time_to_refill.
     * @param time Time point (in micros) that serves as base to calculate the next refill time.
     * @return Returns a uint64_t value (converted from time_point) that defines the next refill
     * period.
     */
    [[nodiscard]] uint64_t calc_next_refill_period (
        const time_point<system_clock, microseconds>& time) const;

    /**
     * set_last_refill_period: Update the m_last_refill_period.
     * It records the m_last_refill_period as the current time (system_clock::now()), and converts
     * the time point to the respective long value.
     */
    void set_last_refill_period ();

    /**
     * record_out_of_tokens_stat: Record a new statistic entry that marks the token-bucket is out of
     * tokens (or does not have enough tokens to serve the consume() operation).
     * Used in consume_operation.
     * @param out_of_tokens_time Absolute time point that marks the token-bucket is out of tokens.
     */
    void record_out_of_tokens_stat (const uint64_t& out_of_tokens_time);

    /**
     * run_garbage_collector: Run a garbage collection algorithm over token-bucket statistics to
     * remove/discard entries that are out of the sliding window period (i.e., outdated collected
     * entries).
     * Used in try_collect_statistics.
     */
    void run_garbage_collector ();

public:
    /**
     * TokenBucket default constructor.
     */
    TokenBucket ();

    /**
     * TokenBucket statistic-based parameterized constructor.
     * Only initializes the statistic-related token-bucket parameters, leaving the default value
     * for the remainder.
     * @param collect_statistics Enable/disable the collection of token-bucket's statistics.
     * @param sliding_window Time window that marks which values should be collected or discarded.
     */
    TokenBucket (const bool& collect_statistics, const uint64_t& sliding_window);

    /**
     * TokenBucket parameterized constructor.
     * @param capacity Defines the maximum capacity of the token-bucket. It also specifies the
     * (maximum) achievable throughput for systems using the token-bucket.
     * @param tokens Defines the number of tokens in the token-bucket at any given time.
     * @param refill_period Defines the time period to refill the token-bucket.
     * @param collect_statistics Enable/disable the collection of token-bucket's statistics.
     * @param sliding_window Time window that marks which values should be collected or discarded.
     */
    TokenBucket (const token& capacity,
        const token& tokens,
        const long& refill_period,
        const bool& collect_statistics,
        const uint64_t& sliding_window);

    /**
     * TokenBucket copy constructor.
     * @param bucket TokenBucket to be copied.
     */
    TokenBucket (const TokenBucket& bucket);

    /**
     * TokenBucket default destructor.
     */
    ~TokenBucket ();

    /**
     * TokenBucket operator.
     * This is not used; only the Threaded version of the token-bucket needs this.
     */
    void operator() ();

    /**
     * get_capacity: get token-bucket's maximum token capacity.
     * @return: Returns a copy of m_capacity.
     */
    [[nodiscard]] token get_capacity () const;

    /**
     * set_capacity: set token-bucket's maximum capacity with a new value.
     * @param capacity New token capacity value.
     */
    void set_capacity (const token& capacity);

    /**
     * get_token_count: get number of tokens in the token-bucket at any give instant.
     * @return: Returns a copy of m_tokens.
     */
    [[nodiscard]] token get_token_count () const;

    /**
     * set_token_count: set a new number of tokens in the token-bucket.
     * @param tokens Number of tokens to insert in the bucket.
     */
    void set_token_count (const token& tokens);

    /**
     * get_refill_period(): get the token-bucket's refill period.
     * @return: Returns a copy of m_refill_period.
     */
    [[nodiscard]] long get_refill_period () const;

    /**
     * set_refill_period(): set token-bucket's refill period with new value.
     * @param period New time period to refill the token-bucket.
     */
    void set_refill_period (const long& period);

    /**
     * is_statistic_collection_enabled: Verify if token-bucket's statistic collection is enabled or
     * disabled.
     * @return: Returns a copy of m_collect_statistics.
     */
    [[nodiscard]] bool is_statistic_collection_enabled () const;

    /**
     * set_statistic_collection(): set token-bucket's collect statistics with new value.
     * @param collect Enable or disable token-bucket's statistic collection
     */
    void set_statistic_collection (const bool& collect);

    /**
     * try_consume: Consume tokens from the bucket.
     * The operation is atomic. Call try_refill to refill the bucket if the refill period
     * (m_refill_period) has been reached. It also validate if the token-bucket has enough tokens
     * to be consumed. If not enough tokens are available, it waits until its refill.
     * Used in the DRL enforcement object.
     * @param consume_tokens Number of tokens to consume from the token-bucket.
     */
    void try_consume (const token& consume_tokens);

    /**
     * try_refill: Refill the token-bucket with m_refill_size tokens.
     * The operation is atomic. For each operation, it validates if the refill period
     * (m_refill_period) was reached, refilling the bucket if so. Otherwise, it sleeps until the
     * time is reached.
     * Used in the DRL enforcement object.
     * @param time_left Address of time_left to directly update the remainder time for refilling
     * the bucket.
     */
    void try_refill (long& time_left);

    /**
     * try_collect_statistics: Collect token-bucket statistics.
     * If the token-bucket has statistic collection enabled, it will call the TBStats'
     * token_bucket_statistics to collect the current statistic that it might hold.
     * All statistics are then encapsulated into an EnforcementUnitStatisticsRaw object.
     * @param statistics_raw Address of an ObjectStatisticsRaw object to store the statistics.
     * @return Returns the number of collected statistics.
     */
    int try_collect_statistics (ObjectStatisticsRaw& statistics_raw);

    /**
     * normalize_tokens: As the throughput of the token-bucket is defined by
     * (m_capacity / m_refill_window), given a specific throughput value and fixed m_refill_window,
     * this method calculates the number of associated tokens.
     * @param throughput Throughput value in a given unit (IOPS or bytes).
     * @return Returns the total of tokens to match the given throughput and m_refill_window values.
     */
    [[nodiscard]] token normalize_tokens (const token& throughput) const;

    /**
     * to_string: generate a string with the token-bucket's variables.
     * @return: Returns the token-bucket parameter values in string format.
     */
    std::string to_string () const;
};
} // namespace paio::enforcement

#endif // PAIO_TOKEN_BUCKET_HPP
