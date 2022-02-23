/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_TOKEN_BUCKET_STATISTICS_HPP
#define PAIO_TOKEN_BUCKET_STATISTICS_HPP

#include <array>
#include <chrono>
#include <mutex>
#include <paio/core/interface_definitions.hpp>
#include <paio/enforcement/objects/drl/token_bucket_statistics_entry.hpp>
#include <paio/utils/logging.hpp>
#include <sstream>

using namespace paio::core;
using namespace paio::utils;
using namespace std::chrono;

namespace paio::enforcement {

/**
 * TBStats class.
 * This class is used to store all statistic entries of the token-bucket.
 * It comprises  parameters:
 *  - m_tb_statistics: vector that stores TBStatsEntry elements; its a circular array;
 *  - m_total_stats: total statistics in the buffer;
 *  - m_position: current index position in the buffer to store;
 *  - m_valid_entry_index: last index position that is valid; invalid elements are marked upon the
 *  garbage collection execution;
 *  - m_max_statistics: max size of the buffer.
 * TODO:
 *  - further testing and validation;
 *  - change structs to specific/dedicated objects in copy_stat_entry and collect_stats methods;
 */
class TBStats {

private:
    std::vector<TBStatsEntry> m_tb_statistics {};
    int m_total_stats { 0 };
    int m_position { 0 };
    int m_valid_entry_index { -1 };
    int m_max_statistics { 100 };
    std::mutex m_lock;

    /**
     * initialize_statistic_entries: initialize statistic entries of m_tb_statistics container.
     */
    void initialize_statistic_entries ();

    /**
     * copy_stat_entry: copy statistic entry from m_tb_statistics container to ObjectStatisticsRaw
     * object. This method is used by a thread-safe method -- collect_stats.
     * @param statistics_raw ObjectStatisticsRaw object to copy the statistics entries.
     * @param stat_index Index of the m_tb_statistics container to copy from.
     * @param raw_index ObjectStatisticsRaw index to copy to.
     */
    void
    copy_stat_entry (ObjectStatisticsRaw& stats_raw, const int& stat_index, const int& raw_index);

    /**
     * reset_stats: reset token-bucket statistic counters to zero.
     * This reset operation does not remove the contents of the m_tb_statistics container. Rather,
     * it resets the total number of statistics (m_total_stats) and the index on the container
     * (m_position).
     * This operation is lock-free, since is only used by the collectTBStats, which already acquires
     * a lock. Use ResetTBStatsLock for the locked version.
     */
    void reset_stats ();

public:
    /**
     * TBStats default constructor.
     */
    TBStats ();

    /**
     * TBStats copy constructor.
     * @param token_bucket_stats TBStats object to be copied.
     */
    TBStats (const TBStats& token_bucket_stats);

    /**
     * TBStats default destructor.
     */
    ~TBStats ();

    /**
     * store_stats_entry: store new statistic entry in the m_tb_statistics container.
     * After registering the entry, m_total_stats is updated.
     * @param empty_bucket_time Normalized token element.
     * @param tokens_left Tokens left in the bucket upon the failed consume operation.
     */
    void store_stats_entry (const float& empty_bucket_time, const double& tokens_left);

    /**
     * collect_stats: convert the statistics held in the m_tb_statistics container to binary.
     * After collecting statistics, the metadata parameters are reset (with reset_stats).
     * @param statistics_raw ObjectStatisticsRaw object to store the statistics.
     */
    int collect_stats (ObjectStatisticsRaw& statistics_raw);

    /**
     * get_total_stats: get the number of collected statistics since the last reset operation.
     * The result cannot be larger than the m_max_statistics.
     * @return Returns a copy of the total of collect statistics.
     */
    [[nodiscard]] int get_total_stats () const;

    /**
     * garbage_collection: Remove outdated statistic entries and update the TBStats object.
     * @param time_point Current time point.
     * @param sliding_window Sliding window to compute which entries are outdated.
     */
    int garbage_collection (const uint64_t& time_point, const uint64_t& sliding_window);

    /**
     * to_string: generate a string with the TBStats's content.
     * @return: Returns all TBStat's statistic entries in string format.
     */
    std::string to_string ();
};

} // namespace paio::enforcement

#endif // PAIO_TOKEN_BUCKET_STATISTICS_HPP
