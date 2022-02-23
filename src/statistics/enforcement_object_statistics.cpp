/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/statistics/enforcement_object_statistics.hpp>

namespace paio::statistics {

// EnforcementObjectStatistics default constructor.
EnforcementObjectStatistics::EnforcementObjectStatistics () = default;

// EnforcementObjectStatistics parameterized constructor.
EnforcementObjectStatistics::EnforcementObjectStatistics (const std::string& identifier,
    const ObjectStatisticMetric& metric) :
    m_stats_identifier { std::string (identifier) },
    m_collection_metric { metric }
{ }

// EnforcementObjectStatistics default destructor.
EnforcementObjectStatistics::~EnforcementObjectStatistics () = default;

// get_stats_identifier call.
std::string EnforcementObjectStatistics::get_stats_identifier () const
{
    return this->m_stats_identifier;
}

// get_collection_metric call.
ObjectStatisticMetric EnforcementObjectStatistics::get_collection_metric () const
{
    return this->m_collection_metric;
};
} // namespace paio::statistics