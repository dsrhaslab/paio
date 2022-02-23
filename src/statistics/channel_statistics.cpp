/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/statistics/channel_statistics.hpp>

namespace paio::statistics {

// ChannelStatistics default constructor.
ChannelStatistics::ChannelStatistics () = default;

// ChannelStatistics parameterized constructor.
ChannelStatistics::ChannelStatistics (const std::string& identifier,
    const StatisticMetric& metric,
    const ClassifierType& type) :
    m_stats_identifier { std::string (identifier) },
    m_collection_metric { metric },
    m_classifier_type { type }
{ }

// ChannelStatistics copy constructor.
ChannelStatistics::ChannelStatistics (const ChannelStatistics& stats) :
    m_stats_identifier { stats.m_stats_identifier },
    m_collection_metric { stats.m_collection_metric },
    m_classifier_type { stats.m_classifier_type },
    m_stats_size { stats.m_stats_size },
    m_total_counter {},
    m_windowed_counter {},
    m_overall_metric { -1 },
    m_last_window_metric { -1 },
    m_start_collection_time { 0 },
    m_end_collection_time { 0 },
    m_last_collection_time { 0 }
{ }

// ChannelStatistics default destructor.
ChannelStatistics::~ChannelStatistics () = default;

// initialize call. Initialize statistic counters based on the type of operations to be collected.
void ChannelStatistics::initialize (const ContextType& context_type)
{
    std::lock_guard<std::mutex> guard (this->m_stats_mutex);

    // verify the type of context that the data plane stage will receive
    switch (context_type) {
        case ContextType::LSM_KVS_SIMPLE:
            this->m_stats_size = lsm_kvs_simple_size;
            break;

        case ContextType::LSM_KVS_DETAILED:
            this->m_stats_size = lsm_kvs_detailed_size;
            break;

        case ContextType::POSIX:
            this->m_stats_size = posix_size;
            break;

        case ContextType::POSIX_META:
            this->m_stats_size = posix_meta_size;
            break;

        case ContextType::KVS:
            this->m_stats_size = kvs_size;
            break;

        case ContextType::PAIO_GENERAL:
        default:
            this->m_stats_size = paio_general_size;
            break;
    }

    // initialize m_total_counter and m_windowed_counter vectors
    for (int i = 0; i < this->m_stats_size; i++) {
        this->m_total_counter.push_back (0);
        this->m_windowed_counter.push_back (0);
    }

    uint64_t now = static_cast<uint64_t> (
        time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ());

    // initialize start_collection_time
    this->record_start_collection_time (now);
    // initialize last_collection_time
    this->record_last_collection_time (now);
}

// update_statistic_entry call. Update statistic entry.
void ChannelStatistics::update_statistic_entry (const int& operation_type,
    const int& operation_context,
    const uint64_t& value)
{
    switch (this->m_classifier_type) {
        case ClassifierType::operation_context:
            this->update_entry (operation_context, value);
            break;

        case ClassifierType::operation_type:
        default:
            this->update_entry (operation_type, value);
            break;
    }
}

// update_entry call. Increment the operation entry by value.
void ChannelStatistics::update_entry (const int& operation, const uint64_t& value)
{
    std::lock_guard<std::mutex> guard (this->m_stats_mutex);

    // calculate the operation's position in the statistics containers
    int position = operation % this->m_stats_size;

    // update total and windowed counter
    this->m_total_counter[position] += value;
    this->m_windowed_counter[position] += value;
}

// collect call. Collect instance statistics and store them in the ChannelStatsRaw object.
void ChannelStatistics::collect (ChannelStatsRaw& channel_stats_raw)
{
    // calculate elapsed time since last collection
    auto now = static_cast<uint64_t> (
        time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ());

    { // entering critical section
        std::lock_guard<std::mutex> guard (this->m_stats_mutex);

        // calculate overall metric values and record total counter stats in raw object
        channel_stats_raw.m_overall_metric_value
            = this->aggregate_total_counter (get_elapsed_time_in_seconds ());

        // calculate window metric values and record windowed counter stats in raw object
        channel_stats_raw.m_windowed_metric_value = this->aggregate_windowed_counter (
            elapsed_time_since_last_collection_in_seconds (now));

        // update collection values
        this->record_last_collection_time (now);
        this->record_end_collection_time (now);

        // reset windowed operation counters
        this->reset_windowed_counters ();
    }
}

// collect_single_entry call. Collect statistics of a single operation entry.
void ChannelStatistics::collect_single_entry (ChannelStatsRaw& channel_stats_raw,
    const int& operation)
{
    // calculate current time in microseconds
    auto now = static_cast<uint64_t> (
        time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ());

    // calculate elapsed time since last collection in seconds
    double elapsed_time = this->elapsed_time_since_last_collection_in_seconds (now);

    { // entering critical section
        std::lock_guard<std::mutex> guard (this->m_stats_mutex);

        // calculate the operation's position in the statistics arrays
        int position = operation % this->m_stats_size;

        // record stats on raw object
        channel_stats_raw.m_overall_metric_value
            = this->calculate_entry_total_counter (position, elapsed_time);
        channel_stats_raw.m_windowed_metric_value
            = this->calculate_entry_windowed_counter (position, elapsed_time);
    }
}

// collect_detailed_windowed_entries call. Collect detailed windowed statistics for each entry.
void ChannelStatistics::collect_detailed_windowed_entries (
    std::vector<double>& detailed_stat_entries)
{
    // calculate current time in microseconds
    auto now = static_cast<uint64_t> (
        time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ());

    // calculate elapsed time since last collection in seconds
    double elapsed_time = this->elapsed_time_since_last_collection_in_seconds (now);

    { // entering critical section
        std::lock_guard<std::mutex> guard (this->m_stats_mutex);
        // calculate the windowed metric counter of each entry of the container
        for (int i = 0; i < this->m_stats_size; i++) {
            // calculate windowed throughput and store in the detailed_stat_entries
            detailed_stat_entries.emplace_back (
                this->calculate_entry_windowed_counter (i, elapsed_time));
        }

        // update collection values
        this->record_last_collection_time (now);
        // reset windowed operation counters
        this->reset_windowed_counters ();
    }
}

// terminate call. Finish ChannelStatistics collection and compute ending operands.
std::string ChannelStatistics::terminate ()
{
    auto now = static_cast<uint64_t> (
        time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ());

    { // entering critical section
        std::lock_guard<std::mutex> guard (this->m_stats_mutex);

        // record the ending time
        this->record_end_collection_time (now);
        // calculate overall metric values (absolute counter or throughput)
        this->aggregate_total_counter (get_execution_time_in_seconds ());

        // return string-formatted statistics results
        return this->to_string ();
    }
}

// create_operation_entry call. Create a new statistic entry.
void ChannelStatistics::create_operation_entry (const int& operation)
{
    // calculate the operation's position in the statistics containers
    int position = operation % this->m_stats_size;
    this->m_total_counter[position] = 0;
    this->m_windowed_counter[position] = 0;
}

// get_stats_identifier call. Get the ChannelStatistics' identifier.
std::string ChannelStatistics::get_stats_identifier () const
{
    return this->m_stats_identifier;
}

// get_overall_metric call. Get overall metric value.
double ChannelStatistics::get_overall_metric ()
{
    std::lock_guard<std::mutex> guard (this->m_stats_mutex);
    return this->m_overall_metric;
}

// get_previous_metric_window call. Get the ChannelStatistics' previous metric value.
double ChannelStatistics::get_previous_metric_window ()
{
    std::lock_guard<std::mutex> guard (this->m_stats_mutex);
    return this->m_last_window_metric;
}

// record_start_collection_time call. Record the statistics collection starting time in
// microseconds.
void ChannelStatistics::record_start_collection_time (const uint64_t& time)
{
    this->m_start_collection_time = time;
}

// record_last_collection_time call. Record the time point, in microseconds, at which statistics
// were collected.
void ChannelStatistics::record_last_collection_time (const uint64_t& time)
{
    this->m_last_collection_time = time;
}

// record_end_collection_time call. Record the statistics collection ending time in microseconds.
void ChannelStatistics::record_end_collection_time (const uint64_t& time)
{
    this->m_end_collection_time = time;
}

// get_start_collection_time call. Get the Statistic's starting time.
uint64_t ChannelStatistics::get_start_collection_time () const
{
    return this->m_start_collection_time;
}

// get_end_collection_time call. Get the Statistic's ending time.
uint64_t ChannelStatistics::get_end_collection_time () const
{
    return this->m_end_collection_time;
}

// elapsed_time_since_last_collection call. Calculate the difference between the current and last
// collection in microseconds.
uint64_t ChannelStatistics::elapsed_time_since_last_collection (const uint64_t& now) const
{
    assert (now > this->m_last_collection_time);
    return now - this->m_last_collection_time;
}

// elapsed_time_since_last_collection_in_seconds call. Calculate the difference between the current
// and last collection in seconds.
double ChannelStatistics::elapsed_time_since_last_collection_in_seconds (uint64_t now) const
{
    // convert elapsed_time_since_last_collection from microseconds to seconds
    return static_cast<double> (this->elapsed_time_since_last_collection (now)) / 1000 / 1000;
}

// aggregate_total_counter call. Calculate the overall metric values.
double ChannelStatistics::aggregate_total_counter (const double& elapsed_time_in_seconds)
{
    double sum = 0;
    // sum all elements of m_total_counter values
    sum = std::accumulate (this->m_total_counter.begin (), this->m_total_counter.end (), sum);

    switch (this->m_collection_metric) {
        case StatisticMetric::throughput:
            // update the overall throughput value
            this->m_overall_metric = sum / elapsed_time_in_seconds;
            break;

        case StatisticMetric::counter:
        default:
            // update the overall counter value
            this->m_overall_metric = sum;
            break;
    }

    return this->m_overall_metric;
}

// aggregate_windowed_counter call. Calculate windowed metric values of the last window interval.
double ChannelStatistics::aggregate_windowed_counter (const double& elapsed_time_in_seconds)
{
    double sum = 0;
    // sum all elements of m_windowed_counter values
    sum = std::accumulate (this->m_windowed_counter.begin (), this->m_windowed_counter.end (), sum);

    switch (this->m_collection_metric) {
        case StatisticMetric::throughput:
            // update the windowed throughput value
            this->m_last_window_metric = sum / elapsed_time_in_seconds;
            break;

        case StatisticMetric::counter:
        default:
            // update the windowed counter value
            this->m_last_window_metric = sum;
            break;
    }

    return this->m_last_window_metric;
}

// calculate_statistic_metric call. Calculates the metric value (throughput or counter) of a given
// time period.
double ChannelStatistics::calculate_statistic_metric (const uint64_t& entry_bytes,
    const double& elapsed_time_in_seconds) const
{
    switch (this->m_collection_metric) {
        case StatisticMetric::throughput:
            // calculate the windowed throughput of the last window interval of a given position
            return (entry_bytes == 0)
                ? static_cast<double> (entry_bytes)
                : (static_cast<double> (entry_bytes) / elapsed_time_in_seconds);

        case StatisticMetric::counter:
        default:
            // return the windowed counter value at a given position
            return static_cast<double> (entry_bytes);
    }
}

// calculate_entry_total_counter call. Calculate the throughput/counter of the last window_interval
// of a specific entry of the total counter container.
double ChannelStatistics::calculate_entry_total_counter (const int& position,
    const double& elapsed_time_in_seconds) const
{
    // this can throw an exception, if the position does not exist
    uint64_t entry_value = this->m_total_counter[position];

    // calculate throughput of the last window_interval
    return this->calculate_statistic_metric (entry_value, elapsed_time_in_seconds);
}

// calculate_entry_windowed_counter call. Calculate the throughput of the last window_interval of
// a specific entry  of the windowed counter container.
double ChannelStatistics::calculate_entry_windowed_counter (const int& position,
    const double& elapsed_time_in_seconds) const
{
    // this can throw an exception, if the position does not exist
    uint64_t entry_value = this->m_windowed_counter[position];

    // calculate throughput of the last window_interval
    return this->calculate_statistic_metric (entry_value, elapsed_time_in_seconds);
}

// get_elapsed_time call. Get time elapsed until now (in microseconds).
uint64_t ChannelStatistics::get_elapsed_time () const
{
    return static_cast<uint64_t> (
               time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ())
        - this->m_start_collection_time;
}

// get_elapsed_time_in_seconds call. Get time elapsed until now (in seconds).
double ChannelStatistics::get_elapsed_time_in_seconds () const
{
    // convert get_elapsed_time (displayed in microseconds) to seconds
    return static_cast<double> (this->get_elapsed_time ()) / 1000 / 1000;
}

// get_execution_time call. Get full execution time.
uint64_t ChannelStatistics::get_execution_time () const
{
    return this->m_end_collection_time - this->m_start_collection_time;
}

// get_execution_time_in_seconds call. Get full execution time in seconds.
double ChannelStatistics::get_execution_time_in_seconds () const
{
    // convert get_execution_time (displayed in microseconds) to seconds
    return static_cast<double> (this->get_execution_time ()) / 1000 / 1000;
}

// reset_windowed_counters call. Reset all statistics entries.
void ChannelStatistics::reset_windowed_counters ()
{
    // erase statistic entries of window_operations_
    for (auto& window_operation : this->m_windowed_counter) {
        window_operation = 0;
    }
}

// to_string call. Generate a string with the ChannelStatistics' m_total_operations counters.
std::string ChannelStatistics::to_string () const
{
    std::stringstream stream;

    stream << "Stats@";
    stream << this->m_stats_identifier << " (";
    stream << this->m_overall_metric << ", ";
    stream << get_execution_time_in_seconds () << "), ";

    for (int i = 0; i < this->m_stats_size; i++) {
        stream << "{" << i << ", ";
        stream << this->m_total_counter.at (i) << "},";
    }

    return stream.str ();
}

// to_string_meta call. Generate a string with the Statistic's meta information.
std::string ChannelStatistics::to_string_meta () const
{
    std::stringstream stream;

    stream << "StatsMeta@";
    stream << this->m_stats_identifier << ", ";
    stream << static_cast<int> (this->m_collection_metric) << ", ";
    stream << static_cast<int> (this->m_classifier_type) << ", ";
    stream << this->m_stats_size;

    return stream.str ();
}

// get_metric call. Get the ChannelStatistics' metric collection value.
StatisticMetric ChannelStatistics::get_metric () const
{
    return this->m_collection_metric;
}

// get_metric call. Get the ChannelStatistics' operation classifier value.
ClassifierType ChannelStatistics::get_classifier_type () const
{
    return this->m_classifier_type;
}

} // namespace paio::statistics
