/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_ENFORCEMENT_OBJECT_STATISTICS_HPP
#define PAIO_ENFORCEMENT_OBJECT_STATISTICS_HPP

#include <string>

/**
 * TODO: create a generic statistics class for EnforcementObject.
 */
namespace paio::statistics {

/**
 * StatisticMetric class.
 * Defines the metric at which statistics should be collected and computed.
 * - StatisticMetric::counter - simple counter; on statistic retrieval just present the value as is;
 * - StatisticMetric::throughput - throughput counter; on statistic retrieval, each entry value is
 *  divided by a time point (and can represent IOPS, bandwidth, ...).
 *  TODO:
 *   - probably merge/use the StatisticMetric enum class from channel-statistics.hpp;
 */
enum class ObjectStatisticMetric { counter = 1, throughput = 2 };

/**
 *  EnforcementObjectStatistics class.
 *  TODO:
 *   - create a generic statistics class for enforcement objects;
 */
class EnforcementObjectStatistics {

private:
    std::string m_stats_identifier {};
    ObjectStatisticMetric m_collection_metric { ObjectStatisticMetric::counter };

public:
    /**
     * EnforcementObjectStatistics default constructor.
     */
    EnforcementObjectStatistics ();

    /**
     * EnforcementObjectStatistics constructor.
     * @param stats_identifier identifier of the statistics.
     * @param collection_metric metric at which statistics should be collected and computed.
     */
    EnforcementObjectStatistics (const std::string& stats_identifier,
        const ObjectStatisticMetric& collection_metric);

    /**
     * EnforcementObjectStatistics destructor.
     */
    ~EnforcementObjectStatistics ();

    /**
     * get_stats_identifier: get the EnforcementObjectStatistics' identifier.
     * @return Returns a copy of m_stats_identifier parameter.
     */
    [[nodiscard, maybe_unused]] std::string get_stats_identifier () const;

    /**
     * get_collection_metric: get the EnforcementObjectStatistics' collection metric.
     * @return Returns a copy of m_collection_metric parameter.
     */
    [[nodiscard, maybe_unused]] ObjectStatisticMetric get_collection_metric () const;
};

} // namespace paio::statistics

#endif // PAIO_ENFORCEMENT_OBJECT_STATISTICS_HPP
