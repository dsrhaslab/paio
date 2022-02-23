/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CHANNEL_HPP
#define PAIO_CHANNEL_HPP

#include <iostream>
#include <paio/core/context.hpp>
#include <paio/core/interface_definitions.hpp>
#include <paio/differentiation/enforcement_object_differentiation_pair.hpp>
#include <paio/enforcement/result.hpp>
#include <paio/enforcement/ticket.hpp>
#include <paio/options/options.hpp>
#include <paio/utils/status.hpp>
#include <vector>

using namespace paio::core;
using namespace paio::differentiation;
using namespace paio::options;
using namespace paio::utils;

namespace paio::enforcement {

/**
 * Channel abstract class.
 * A channel provides a stream-like abstraction through where requests flow. Each channel contains
 * one or more enforcement objects, as well as a differentiation rule that maps a request to the
 * respective enforcement object, so it can be enforced. The combination of channels and enforcement
 * objects is designed to ease the implementation of new storage services, while promoting their
 * re-utilization and applicability.
 * TODO:
 *  - Change structs to specific/dedicated objects in collect_object_statistics,
 *  collect_general_statistics, collect_detailed_statistics, and collect_statistic_entry methods;
 *  - Create a dedicated object for specifying each configuration or set of configurations in
 *  create_enforcement_object and configure_enforcement_object methods;
 */
class Channel {

public:
    /**
     * Channel default constructor.
     */
    Channel () = default;

    /**
     * Channel default destructor.
     */
    virtual ~Channel () = default;

    /**
     * channel_enforce: enforce the EnforcementObject's service over I/O requests.
     * @param ticket Ticket object that contains all I/O classifiers to enforce a request, including
     * the request size, context, and cost.
     */
    virtual void channel_enforce (const Context& context,
        const void* buffer,
        const std::size_t& buffer_size,
        Result& result)
        = 0;

    /**
     * collect_statistics: collect statistics from a given EnforcementObject.
     * @param enforcement_object_id Identifier of the EnforcementObject to collect the statistics.
     * @param statistics_raw Reference to an ObjectStatisticsRaw structure to place all statistic
     * entries.
     * @return Returns PStatus::OK if statistics were effectively collected; PStatus::Error
     * otherwise.
     */
    virtual PStatus collect_object_statistics (const long& enforcement_object_id,
        ObjectStatisticsRaw& statistics_raw)
        = 0;

    /**
     * collect_general_statistics: Collect the global I/O statistics collected in this channel,
     * which are stored in the m_channel_statistics container.
     * @param general_stats Reference to a ChannelStatsRaw structure to store the general I/O
     * statistics collected in this channel.
     * @return Returns PStatus::OK if statistics were effectively collected; and PStatus::Error
     * otherwise.
     */
    virtual PStatus collect_general_statistics (ChannelStatsRaw& general_stats) = 0;

    /**
     * collect_detailed_statistics: Collect the detailed (full) I/O statistics collected in this
     * channel, which are stored in the m_channel_statistics container.
     * @param detailed_stat_entries Container that stores all statistic entries.
     * @return Returns PStatus::OK if statistics were effectively collected; and PStatus::Error
     * otherwise.
     */
    virtual PStatus collect_detailed_statistics (std::vector<double>& detailed_stat_entries) = 0;

    /**
     * collect_statistic_entry: Collect a single statistic entry from the m_channel_statistics
     * container.
     * @param entry_stats Reference to a ChannelStatsRaw structure to store the I/O statistics of
     * a given entry.
     * @param operation Operation entry to be collected.
     * @return Returns PStatus::OK if statistics were effectively collected; and PStatus::Error
     * otherwise.
     */
    virtual PStatus collect_statistic_entry (ChannelStatsRaw& stats, const int& operation) = 0;

    /**
     * create_enforcement_object: create an EnforcementObject of a specific type with specific
     * configurations.
     * @param enforcement_object_id Identifier of the EnforcementObject to be created.
     * @param differentiation_pair ObjectDifferentiationPair object that defines the I/O classifiers
     * and differentiation token of the EnforcementObject.
     * @param object_type Type of the EnforcementObject to create.
     * @param configurations Configuration of the EnforcementObject to create. If empty, the object
     * is created with default settings.
     * @return Returns PStatus::OK if the object is successfully created; PStatus::Error otherwise.
     */
    virtual PStatus create_enforcement_object (const long& enforcement_object_id,
        const ObjectDifferentiationPair& differentiation_pair,
        const EnforcementObjectType& object_type,
        const std::vector<long>& configurations)
        = 0;

    /**
     * configure_enforcement_object: configure the properties of the EnforcementObject's service
     * to comply with application requirements and/or workload variations.
     * @param enforcement_object_id Identifier of the EnforcementObject to be configured.
     * @param config Defines the configuration operation to employ.
     * @param configurations Contains the configurations (values) to be updated in the
     * EnforcementObject.
     * @return Returns PStatus::Enforced if the object was configured with the new settings;
     * PStatus::Error otherwise.
     */
    virtual PStatus configure_enforcement_object (const long& enforcement_object_id,
        const int& config,
        const std::vector<long>& configurations)
        = 0;

    /**
     * to_string: Wrap Channel elements into string format.
     * @return Returns a string with the Channel information.
     */
    virtual std::string to_string () = 0;
};
} // namespace paio::enforcement

#endif // PAIO_CHANNEL_HPP
