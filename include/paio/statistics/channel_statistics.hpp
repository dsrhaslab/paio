/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CHANNEL_STATISTICS_HPP
#define PAIO_CHANNEL_STATISTICS_HPP

#include <array>
#include <iostream>
#include <mutex>
#include <numeric>
#include <paio/core/context_propagation_definitions.hpp>
#include <paio/core/interface_definitions.hpp>
#include <paio/utils/logging.hpp>
#include <paio/utils/status.hpp>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace std::chrono;
using namespace paio::core;

namespace paio::statistics {

/**
 * StatisticMetric class.
 * Defines the metric at which statistics should be collected and computed.
 * - StatisticMetric::counter - simple counter; on statistic retrieval just present the value as is;
 * - StatisticMetric::throughput - throughput counter; on statistic retrieval, each entry value is
 *  divided by a time point (and can represent IOPS, bandwidth, ...).
 */
enum class StatisticMetric { counter = 1, throughput = 2 };

/**
 * ClassifierType class.
 * Defines the operation classifier to be collected.
 *  - ClassifierType::operation_type - collects statistics using the operation type classifier;
 *  examples of such classifier include read, write, get, put, create, ...
 *  - ClassifierType::operation_context - collects statistics using the operation context
 *  classifier; examples of such classifier include bg_flush, bg_compaction, ...
 */
enum class ClassifierType { operation_type = 1, operation_context = 2 };

/**
 * ChannelStatistics class.
 * The ChannelStatistics object allows to collect statistics using different metrics, and can be
 * placed at different points of the I/O path, namely at the Channel, EnforcementObject,
 * InstanceInterface, and more. Currently, statistics can be collected and computed over two
 * types of metrics, namely StatisticsMetric::counter and StatisticsMetric::throughput. This is
 * composed by the following instance parameters:
 *  - m_stats_identifier: defines the identifier of the ChannelStatistics object; this can be useful
 *  when the having multiple statistic objects at the same data point;
 *  - m_collection_metric: defines the type of metric to be collected (StatisticMetric);
 *  - m_classifier_type: defines the operation context or type oto be collected (ClassifierType);
 *  - m_total_counter: container that holds the metric counter of each statistic entry for the
 *  overall execution time;
 *  - m_windowed_counter: container that holds the metric counter of each statistic entry between
 *  the last collection period and another time point (typically system_time::now());
 *  - m_overall_metric: Overall metric counter; contains the sum (counter metric) or the average
 *  (throughput metric) of the m_total_counter container;
 *  - m_windowed_metric: Windowed metric counter; contains the sum (counter metric) or the average
 *  (throughput metric) of the m_windowed_counter container between the last collection period and
 *  another time point;
 *  - m_start_collection_time: Starting time point, in microseconds, of the statistic collection;
 *  - m_end_collection_time: Ending time point, in microseconds, of the statistic collection;
 *  - m_last_collection_time: Time point, in microseconds, that marks the last statistic collection;
 *  - m_stats_mutex: used for concurrency control.
 * TODO:
 *  - improve granularity of statistic entries and respective locking: create StatisticEntry class
 *  and update the m_total_counter and m_windowed_counter containers to use map/unordered_map; this
 *  way, on update_entry calls, we can simply lock entry and not the entire container;
 *   change structs to specific/dedicated objects in collect, collect_single_entry, and
 *   collect_detailed_windowed_entries;
 */
class ChannelStatistics {
    // friend classes of ChannelStatistics
    friend class ChannelStatisticsTest;

private:
    std::string m_stats_identifier {};
    StatisticMetric m_collection_metric { StatisticMetric::counter };
    ClassifierType m_classifier_type { ClassifierType::operation_context };
    int m_stats_size { 0 };
    std::vector<uint64_t> m_total_counter {};
    std::vector<uint64_t> m_windowed_counter {};
    double m_overall_metric { -1 };
    double m_last_window_metric { -1 };
    uint64_t m_start_collection_time { 0 };
    uint64_t m_end_collection_time { 0 };
    uint64_t m_last_collection_time { 0 };
    std::mutex m_stats_mutex;

    /**
     * update_entry: update the operation entry of the m_total_counter and m_windowed_counter
     * containers by value.
     * This method is thread-safe.
     * @param operation Operation entry to be incremented.
     * @param value Operation's payload. For example, total operations for when using the counter
     * StatisticMetric type, and total of operations * size of operation when using the throughput
     * StatisticMetric type.
     */
    void update_entry (const int& operation, const uint64_t& value);

    /**
     * create_operation_entry: Create a new statistic entry on the m_total_counter and
     * m_windowed_counter containers.
     * It computes the operation's position in the containers (index).
     * @param operation Operation to be registered.
     */
    [[maybe_unused]] void create_operation_entry (const int& operation);

    /**
     * aggregate_total_counter: Calculate the absolute metric value between a certain time window,
     * and update the m_last_window_metric instance value.
     * This value varies with the StatisticMetric option (m_collection_metric). Namely, with
     * StatisticMetric::counter the method performs the sum of all m_total_counter values; with
     * StatisticMetric::throughput, the method performs the average of m_total_counter.
     * @param elapsed_time_in_seconds Observation window, in seconds, to be considered.
     * @return Returns the aggregation result (m_overall_metric).
     */
    double aggregate_total_counter (const double& elapsed_time_in_seconds);

    /**
     * aggregate_windowed_counter: Calculate the windowed metric values between the current time and
     * the last window interval, and update the m_last_window_metric instance value.
     * This value varies with the StatisticMetric option (m_collection_metric). Namely, with
     * StatisticMetric::counter the method performs the sum of all m_windowed_counter values; with
     * StatisticMetric::throughput, the method performs the average of m_windowed_counter.
     * @param elapsed_time_in_seconds Observation window, in seconds, to be considered.
     * @return Returns the aggregation result (m_windowed_metric).
     */
    double aggregate_windowed_counter (const double& elapsed_time_in_seconds);

    /**
     * calculate_statistic_metric: Calculates the metric value (throughput or counter) of a given
     * time period. This method will compute the final value based on the Statistic Metric option
     * (m_collection_metric). Namely, with StatisticMetric::counter the method simply returns the
     * entry_bytes value; with StatisticMetric::throughput, the method computes the
     * average of m_total_counter, through entry_bytes and elapsed_time_in_seconds.
     * @param entry_bytes Number of bytes (throughput) or operations (counter).
     * @param elapsed_time_in_seconds Elapsed time in seconds.
     * @return Returns the result of performing the respective metric computation.
     */
    [[nodiscard]] double calculate_statistic_metric (const uint64_t& entry_bytes,
        const double& elapsed_time_in_seconds) const;

    /**
     * calculate_entry_total_counter: Calculate the metric value of the last window interval of a
     * specific entry. This value varies with the StatisticMetric option (m_collection_metric).
     * Namely, with StatisticMetric::counter the method read m_total_counter[position] value; with
     * StatisticMetric::throughput, the method computes the average of m_total_counter[position]
     * and the time interval.
     * @param position Statistic entry's index to compute.
     * @param elapsed_time_in_seconds Observation window, in seconds, to be considered.
     * @return Returns the metric counter of the specified entry.
     */
    [[nodiscard]] double calculate_entry_total_counter (const int& position,
        const double& elapsed_time_in_seconds) const;

    /**
     * calculate_entry_windowed_counter: Calculate the metric value of the last window interval of
     * a specific entry. This value varies with the StatisticMetric option (m_collection_metric).
     * Namely, with StatisticMetric::counter the method read m_windowed_counter[position] value;
     * with StatisticMetric::throughput, the method computes the average of
     * m_windowed_counter[position] and the time interval.
     * @param position Statistic entry's index to compute.
     * @param elapsed_time_in_seconds Observation window, in seconds, to be considered.
     * @return Returns the windowed metric counter of the specified entry.
     */
    [[nodiscard]] double calculate_entry_windowed_counter (const int& position,
        const double& elapsed_time_in_seconds) const;

    /**
     * record_start_collection_time: Record the statistics collection starting time in microseconds.
     */
    void record_start_collection_time (const uint64_t& time);

    /**
     * record_end_collection_time: Record the statistics collection ending time in microseconds.
     */
    void record_end_collection_time (const uint64_t& time);

    /**
     * record_last_collection_time: Record the time point, in microseconds, at which statistics
     * were collected.
     */
    void record_last_collection_time (const uint64_t& time);

    /**
     * reset_windowed_counters: Reset all statistics entries (fill with zero) of m_windowed_counter
     * container.
     */
    void reset_windowed_counters ();

    /**
     * get_elapsed_time: Get time elapsed until now. The time is given in microseconds.
     * @return Return the time elapsed between m_start_collection_time and present time.
     */
    [[nodiscard]] uint64_t get_elapsed_time () const;

    /**
     * get_elapsed_time_in_seconds: Get time elapsed until now, in seconds.
     * @return Return the time elapsed between m_start_collection_time and present time, in seconds.
     */
    [[nodiscard]] double get_elapsed_time_in_seconds () const;

    /**
     * get_start_collection_time: Get m_start_collection_time value.
     * @return Returns a copy of the m_start_collection_time parameter.
     */
    [[nodiscard, maybe_unused]] uint64_t get_start_collection_time () const;

    /**
     * get_end_collection_time: Get m_end_collection_time value
     * @return Returns a copy of the m_end_collection_time parameter.
     */
    [[nodiscard, maybe_unused]] uint64_t get_end_collection_time () const;

    /**
     * get_execution_time: Get full execution time. The time is given in microseconds.
     * @return Return the time elapsed between m_start_collection_time and m_end_collection_time.
     */
    [[nodiscard]] uint64_t get_execution_time () const;

    /**
     * get_execution_time_in_seconds: Get full execution time in seconds.
     * @return Return the time elapsed between m_start_collection_time and m_end_collection_time,
     * in seconds.
     */
    [[nodiscard]] double get_execution_time_in_seconds () const;

    /**
     * elapsed_time_since_last_collection: Calculate the difference between the current time and
     * the m_last_collection_time
     * @param now Current time, expressed in microseconds
     * @return Return the total window period.
     */
    [[nodiscard]] uint64_t elapsed_time_since_last_collection (const uint64_t& now) const;

    /**
     * elapsed_time_since_last_collection_in_seconds: Calculate the difference between the current
     * time and the m_last_collection_time, in seconds.
     * @param now Current time, expressed in microseconds
     * @return Return the total window period, in seconds.
     */
    [[nodiscard]] double elapsed_time_since_last_collection_in_seconds (uint64_t now) const;

    /**
     * to_string: generate a string with the ChannelStatistics' m_total_operations counters.
     * @return returns the ChannelStatistics' m_total_operations counters in string-based format.
     */
    [[nodiscard]] std::string to_string () const;

    /**
     * to_string_meta: generate a string with the Statistic's meta information, which includes
     * m_stats_identifier, m_collection_metric, m_classifier-type, and m_stats_size.
     * @return Returns in string-based format the Statistic's meta information.
     */
    [[nodiscard]] std::string to_string_meta () const;

public:
    /**
     * ChannelStatistics default constructor.
     */
    ChannelStatistics ();

    /**
     * ChannelStatistics parameterized constructor.
     * @param identifier ChannelStatistics identifier (we can have multiple statistics objects in a
     * single data plane stage).
     * @param metric Defines the metric at which requests are collected and computed. It can be
     * StatisticMetric::counter or StatisticMetric::throughput.
     * @param type Defines the classifier at which requests should be collected. It can be of type
     * ClassifierType::operation_type (e.g., read, write, put, get) or
     * ClassifierType::operation_context (e.g., bg_flush, bg_compaction, ...).
     */
    ChannelStatistics (const std::string& identifier,
        const StatisticMetric& metric,
        const ClassifierType& type);

    /**
     * ChannelStatistics copy constructor.
     */
    ChannelStatistics (const ChannelStatistics& stats);

    /**
     * ChannelStatistics default destructor.
     */
    ~ChannelStatistics ();

    /**
     * initialize: initializes the m_total_counter and m_windowed_counter containers based on the
     * ContextType and records the initial collection time for both m_start_collection_time and
     * m_last_collection_time.
     * This method is thread-safe.
     * @param context_type Context and/or type of statistic collection.
     *  - ContextType::PAIO_GENERAL receives generic PAIO operations, and initializes both
     *  containers with paio_general_size;
     *  - ContextType::POSIX will receive POSIX-based operations, and initializes both containers
     *  with posix_size;
     *  - ContextType::KVS will receive key-value based operations, and initializes both containers
     *  with kvs_size;
     *  - ContextType::LSM_KVS_SIMPLE will receive POSIX operations originated from key-value
     *  stores, and initializes both containers with posix_lsm_kvs_simple_size; rather than
     *  defining all compaction levels, it bins these in high-priority compactions
     *  (LSM_KVS_SIMPLE::bg_compaction_high_priority) or low-priority compactions
     *  (LSM_KVS_SIMPLE::bg_compaction_low_priority);
     *  - ContextType::LSM_KVS_DETAILED will receive POSIX operations originated from
     *  key-value stores, and initializes both containers with lsm_kvs_detailed_size.
     */
    void initialize (const ContextType& context_type);

    /**
     * update_statistic_entry: update the operation entry of the m_total_counter and
     * m_windowed_counter containers by value. It verifies the statistics' classifier type
     * (m_classifier_type) and invoke update_entry with the correct operation classifier.
     * @param operation_type Defines the operation entry to be collected for the operation type.
     * @param operation_context Defines the operation entry to be collected for the operation
     * context.
     * @param value Operation's payload. For example, total operations for when using the counter
     * StatisticMetric type, and total of operations * size of operation when using the throughput
     * StatisticMetric type.
     */
    void update_statistic_entry (const int& operation_type,
        const int& operation_context,
        const uint64_t& value);

    /**
     * collect: Collect statistic from both overall and windowed counters, respectively
     * m_overall_metric and m_last_window_metric. The method aggregates all entries from the
     * overall and windowed counters, and stores them in ChannelStatsRaw's m_overall_metric_value
     * and m_last_window_metric_value, respectively.
     * This method is thread-safe.
     * @param channel_stats_raw ChannelStatsRaw object to be populated with the collected metrics.
     */
    void collect (ChannelStatsRaw& channel_stats_raw);

    /**
     * collect_single_entry: Collect statistics from both overall and windowed counter of a single
     * operation (entry in the statistics counters).
     * The method first computes the elapsed time between the last collection period and the
     * current time. Then it calculates the operation's position in the statistics arrays. Finally,
     * it records the statistics on the raw object through calculate_entry_total_counter and
     * calculate_entry_windowed_counter.
     * This method is thread-safe.
     * @param channel_stats_raw ChannelStatsRaw object to be populated with the collected metrics.
     * @param operation Operation type to be considered.
     */
    void collect_single_entry (ChannelStatsRaw& channel_stats_raw, const int& operation);

    /**
     * collect_detailed_windowed_entries: Collect detailed windowed statistics of each entry of a
     * given channel (workflow). For each window entry in the container, it computes the entry's
     * windowed counter value. It also updates the last collection time and resets windowed
     * operations.
     * This operation is thread-safe.
     * @param detailed_stat_entries Reference to a vector to store the windowed counters of all
     * statistics entries. The method assumes a "fresh" vector (no elements in the container).
     */
    void collect_detailed_windowed_entries (std::vector<double>& detailed_stat_entries);

    /**
     * terminate: Finish ChannelStatistics collection and compute ending operands.
     * It records the end collection time (m_end_collection_time) and computes the overall metric
     * values (through aggregate_total_counter).
     * This method is thread-safe.
     * @return Returns string-formatted Statistic object.
     */
    std::string terminate ();

    /**
     * get_stats_identifier: get the ChannelStatistics' identifier.
     * @return Returns a copy of m_stats_identifier parameter.
     */
    [[nodiscard, maybe_unused]] std::string get_stats_identifier () const;

    /**
     * get_metric: get the ChannelStatistics' collection metric.
     * @return Returns a copy of m_collection_metric parameter.
     */
    [[nodiscard]] StatisticMetric get_metric () const;

    /**
     * get_classifier_type: get the ChannelStatistics' operation classifier type.
     * @return Returns a copy of m_classifier_type parameter.
     */
    [[nodiscard]] ClassifierType get_classifier_type () const;

    /**
     * get_overall_metric: get the overall metric value.
     * This method is thread-safe.
     * @return Returns a copy of m_overall_metric.
     */
    [[nodiscard]] double get_overall_metric ();

    /**
     * get_previous_metric_window: get the ChannelStatistics' previous metric value.
     * This method is thread-safe.
     * @return Returns a copy of m_last_window_metric.
     */
    [[nodiscard]] double get_previous_metric_window ();
};
} // namespace paio::statistics

#endif // PAIO_CHANNEL_STATISTICS_HPP
