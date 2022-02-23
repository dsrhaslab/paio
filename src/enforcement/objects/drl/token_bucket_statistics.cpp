/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/objects/drl/token_bucket_statistics.hpp>

namespace paio::enforcement {

// TBStats default constructor.
TBStats::TBStats ()
{
    Logging::log_debug ("TBStats default constructor.");
    // initialize m_tb_statistics container
    this->initialize_statistic_entries ();
}

// TBStats copy constructor.
TBStats::TBStats (const TBStats& token_bucket_stats) :
    m_tb_statistics { token_bucket_stats.m_tb_statistics },
    m_total_stats { token_bucket_stats.m_total_stats },
    m_position { token_bucket_stats.m_position },
    m_valid_entry_index { token_bucket_stats.m_valid_entry_index },
    m_max_statistics { token_bucket_stats.m_max_statistics }
{
    Logging::log_debug ("TBStats copy constructor.");
}

// TBStats default destructor.
TBStats::~TBStats () = default;

// initialize_statistic_entries call. Initialize statistic entries of m_tb_statistics container.
void TBStats::initialize_statistic_entries ()
{
    for (int i = 0; i < this->m_max_statistics; i++) {
        this->m_tb_statistics.emplace_back ();
    }
}

// copy_stat_entry call. Copy statistic entry from m_tb_statistics container to ObjectStatisticsRaw.
void TBStats::copy_stat_entry (ObjectStatisticsRaw& stats_raw,
    const int& stat_index,
    const int& raw_index)
{
    // copy normalized_empty_bucket element
    stats_raw.m_object_statistic_entries[raw_index].m_normalized_empty_bucket
        = this->m_tb_statistics[stat_index].get_normalized_empty_bucket ();
    // copy tokens_left element
    stats_raw.m_object_statistic_entries[raw_index].m_tokens_left
        = this->m_tb_statistics[stat_index].get_tokens_left ();
}

// reset_stats call. Reset Token-Bucket statistics counters to zero.
void TBStats::reset_stats ()
{
    this->m_total_stats = 0;
    this->m_position = 0;
}

// store_stats_entry call. Store new statistic entry in the m_tb_statistics container.
void TBStats::store_stats_entry (const float& empty_bucket_time, const double& tokens_left)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    // calculate the index to store the entry
    this->m_position = this->m_total_stats % this->m_max_statistics;
    // store entry
    this->m_tb_statistics[this->m_position] = TBStatsEntry (empty_bucket_time,
        tokens_left,
        static_cast<uint64_t> (
            time_point_cast<microseconds> (system_clock::now ()).time_since_epoch ().count ()));

    this->m_total_stats++;
}

// collect_stats call. Convert the statistics held in the container to binary.
int TBStats::collect_stats (ObjectStatisticsRaw& statistics_raw)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    int total_stats_copied = 0;

    // validate if there are statistic entries registered
    if (this->m_total_stats > 0) {
        // gather all statistics
        if (this->m_total_stats <= this->m_max_statistics) {
            // begin iteration in the first valid entry (after garbage collection)
            for (int i = this->m_valid_entry_index, index = 0; i < this->m_total_stats;
                 i++, index++) {
                // copy entry
                this->copy_stat_entry (statistics_raw, i, index);
                total_stats_copied++;
            }
        } else {
            // validate if statistic to collect are comprehended between the first array
            // element and m_position
            if ((this->m_position - this->m_valid_entry_index) >= 0) {
                for (int i = this->m_valid_entry_index, index = 0; i < this->m_position;
                     i++, index++) {
                    // copy stat entries from m_tb_statistics[0] to m_tb_statistics[m_position]
                    this->copy_stat_entry (statistics_raw, i, index);
                    total_stats_copied++;
                }

            } else {
                int index;
                // since the m_tb_statistics is a ring buffer, start from the m_position index
                for (index = 0; index < (this->m_max_statistics - this->m_valid_entry_index);
                     index++) {
                    // copy stat entries from m_tb_statistics[m_valid_entry_index] to
                    // m_tb_statistics[m_max_statistics]
                    this->copy_stat_entry (statistics_raw,
                        this->m_valid_entry_index + index,
                        index);

                    total_stats_copied++;
                }

                // then copy the remainder entries
                for (int i = 0; i < this->m_position; i++, index++) {
                    // copy stat entries from m_tb_statistics[0] to m_tb_statistics[m_position]
                    this->copy_stat_entry (statistics_raw, i, index);
                    total_stats_copied++;
                }
            }
        }
    } else {
        if (Logging::is_debug_enabled ()) {
            Logging::log_debug ("TBStats::CollectTBStats: there are no "
                                "(statistic) entries registered.");
        }
    }

    // reset TBStats elements
    reset_stats ();
    // mark valid entries as none
    this->m_valid_entry_index = -1;
    // update ObjectStatisticsRaw's total number of valid statistic entries
    statistics_raw.m_total_stats = total_stats_copied;

    return total_stats_copied;
}

// get_total_stats call. Get the number of collected statistics since the last reset operation.
int TBStats::get_total_stats () const
{
    return this->m_total_stats;
}

// garbage_collection call. Remove outdated statistic entries and update the TBStats object.
int TBStats::garbage_collection (const uint64_t& time_point, const uint64_t& sliding_window)
{
    Logging::log_debug ("Garbage Collection was called.");

    std::unique_lock<std::mutex> lock (this->m_lock);
    int discarded_entries = 0;

    // validate if there are statistics to collect/clean
    if (this->m_total_stats > 0) {
        // if most recent entry is outdated (in comparison to the sliding_window), then discard
        // all entries
        if ((time_point - this->m_tb_statistics[this->m_position].get_collection_timestamp ())
            > sliding_window) {
            // mark valid_entry_index_ with no entries
            this->m_valid_entry_index = -1;
            // reset statistics metadata
            this->reset_stats ();

            // set total of discarded statistic entries
            discarded_entries = (this->m_total_stats <= this->m_max_statistics)
                ? this->m_total_stats
                : this->m_max_statistics;

        } else {
            // validate if total entries are fewer than m_max_statistics
            if (this->m_total_stats <= this->m_max_statistics) {
                for (int i = 0; i < this->m_total_stats; i++) {
                    // if statistic entry is valid, update index and break cycle
                    if ((time_point - this->m_tb_statistics[i].get_collection_timestamp ())
                        < sliding_window) {
                        // update m_valid_entry_index with current m_tb_statistics position
                        this->m_valid_entry_index = i;
                        // break cycle as remainder entries will have their timestamp within
                        // the sliding window
                        break;
                    }
                }

                // set total of discarded statistic entries
                discarded_entries = this->m_valid_entry_index;

            } else {
                // start from the beginning of the m_tb_statistics array
                for (int i = 0; i < this->m_position; i++) {
                    // if statistic entry is valid, update index and break cycle
                    if ((time_point - this->m_tb_statistics[i].get_collection_timestamp ())
                        < sliding_window) {
                        // update m_valid_entry_index with current m_tb_statistics position
                        this->m_valid_entry_index = i;
                        // break cycle as remainder entries will have their timestamp within
                        // the sliding window
                        break;
                    } else {
                        // increment total of discarded statistic entries
                        discarded_entries++;
                    }
                }

                // if m_valid_entry_index != 0, it means the previous statistics are outdated
                if (this->m_valid_entry_index == 0) {
                    // since the m_tb_statistics is a ring buffer, start from the
                    // m_position index
                    for (int i = 0; i < (this->m_max_statistics - this->m_position); i++) {
                        // if statistic entry is valid, update index and break cycle
                        if ((time_point
                                - this->m_tb_statistics[this->m_position + i]
                                      .get_collection_timestamp ())
                            < sliding_window) {
                            this->m_valid_entry_index = this->m_position + i;
                            // break cycle as the other entries will have their timestamp
                            // within the sliding window
                            break;
                        } else {
                            // increment total of discarded statistic entries
                            discarded_entries++;
                        }
                    }

                } else {
                    // increment total of discarded statistic entries
                    discarded_entries += (this->m_max_statistics - this->m_position);
                }
            }
        }
    } else { // there are no statistics to collect
        // reset TBStats metadata
        this->reset_stats ();
        // mark m_valid_entry_index with no entries
        this->m_valid_entry_index = -1;
    }

    return discarded_entries;
}

// to_string call. Generate a string with the TBStat's content.
std::string TBStats::to_string ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    std::stringstream stream;
    stream << "[";
    int iterations = (this->m_total_stats > this->m_max_statistics) ? this->m_max_statistics
                                                                    : this->m_position;

    for (int i = 0; i < iterations; i++) {
        stream << "{";
        stream << this->m_tb_statistics[i].get_normalized_empty_bucket ();
        stream << ",";
        stream << this->m_tb_statistics[i].get_tokens_left ();
        stream << "}; ";
    }

    stream << "]";

    return stream.str ();
}
} // namespace paio::enforcement
