/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CORE_HPP
#define PAIO_CORE_HPP

#include <atomic>
#include <map>
#include <mutex>
#include <paio/differentiation/channel_differentiation_builder.hpp>
#include <paio/differentiation/channel_differentiation_tuple.hpp>
#include <paio/differentiation/channel_hashing_differentiation.hpp>
#include <paio/differentiation/enforcement_object_differentiation_pair.hpp>
#include <paio/enforcement/channel_default.hpp>
#include <paio/rules/enforcement_rule.hpp>
#include <paio/rules/housekeeping_table.hpp>
#include <paio/statistics/channel_statistics.hpp>
#include <thread>
#include <unordered_map>

using namespace paio::differentiation;
using namespace paio::enforcement;
using namespace paio::options;
using namespace paio::rules;
using namespace paio::statistics;
using namespace paio::utils;

namespace paio::core {

/**
 * Core class.
 * The Core class provides the main components for the classification and enforcement of requests.
 * It contains the main enforcement and management modules, namely the channels for enforcing the
 * I/O requests, and an HousekeepingTable to manage the internal data plane organization.
 * Internally, the Core class is organized with several instance variables:
 * - m_channels: container that stores all Channels of the data plane stage; each key-value pair
 * corresponds to a differentiation token and the respective Channel object;
 * - m_channel_id_to_token_linkers: container used for mapping a Channel identifier to the
 * respective differentiation token;
 * - m_channel_diff_builder: defines the I/O classification and differentiation of requests at
 * Channel level. By default, it uses a hashing differentiation model;
 * - m_housekeeping_table: stores all housekeeping rules submitted from the control plane, or
 * directly read from an HousekeepingRule's file;
 * - m_channels_lock: mutex for concurrency control over m_channels object;
 * - m_linkers_locks: mutex for concurrency control over m_channel_id_to_token_linkers object;
 * - m_define_default_object_differentiation: atomic variable that defines if object I/O
 * differentiation should be made by default and at Channel creation time.
 * Future work: support for DifferentiationTable (use_differentiation_table,
 * insert_differentiation_rule, remove_differentiation_rule; support for remove channels and
 * enforcement objects (careful w/ CC).
 * TODO:
 *  - test the performance of m_channels with unordered_map vs. vector;
 *  - create dedicated tests for create_new_channel_linker, get_channel_diff_token,
 *  define_enforcement_object_differentiation_with_channel_token, define_channel_differentiation,
 *  define_enforcement_object_differentiation, collect_enforcement_object_statistics,
 *  collect_channel_statistics, collect_channel_statistics_detailed, and execute_housekeeping_rule;
 *  - change select_channel method's return value from ChannelDefault* to Channel*;
 *  - change structs to specific/dedicated objects in collect_enforcement_object_statistics,
 *  collect_channel_statistics, and collect_channel_statistics_detailed methods;
 */
class Core {

private:
    std::unordered_map<diff_token_t, std::unique_ptr<ChannelDefault>> m_channels;
    std::vector<std::pair<long, diff_token_t>> m_channel_id_to_token_linkers;
    std::unique_ptr<ChannelDifferentiationBuilder> m_channel_diff_builder {
        std::make_unique<ChannelHashingDifferentiation> (option_default_hashing_algorithm)
    };
    HousekeepingTable m_housekeeping_table {};
    std::mutex m_channels_lock;
    std::mutex m_linkers_lock;
    std::atomic<bool> m_define_default_object_differentiation {
        option_define_default_object_differentiation_on_create_channel
    };

    /**
     * create_new_channel_linker: Whenever a new channel is created, this method creates a new pair
     * with the corresponding channel identifier and differentiation token to be placed in the
     * create_new_channel_linker.
     * Concurrency control over m_channel_id_to_token_linkers.
     * @param channel_id Identifier of the Channel.
     * @param channel_token Differentiation token of the Channel.
     */
    void create_new_channel_linker (const long& channel_id, const diff_token_t& channel_token);

    /**
     * get_channel_diff_token: Get the differentiation token of a given channel.
     * Concurrency control over m_channel_id_to_token_linkers handled by upper methods.
     * @param channel_id Identifier of the corresponding channel.
     * @return If the channel exists, returns the corresponding differentiation token; otherwise,
     * returns -1.
     */
    [[nodiscard]] diff_token_t get_channel_diff_token (const long& channel_id) const;

    /**
     * create_channel: Create a new Channel in the data plane stage.
     * Concurrency control to m_channels is handled by upper methods (execute_housekeeping_rule).
     * @param channel_id Identifier of the channel to be created.
     * @param differentiation_pair Defines the workflow_id, operation_type, and operation_context
     * I/O classifiers of the Channel; the differentiation token is computed based on these values.
     * @return Returns PStatus::OK if the Channel was successfully created, and PStatus::Error
     * otherwise.
     */
    PStatus create_channel (const long& channel_id,
        const ChannelDifferentiationTuple& differentiation_pair);

    /**
     * create_enforcement_object: Create a new EnforcementObject in the data plane stage.
     * The method computes the channel's differentiation token that respects to where the new
     * EnforcementObject should be created.
     * Concurrency control to m_channels is handled by upper methods (execute_housekeeping_rule).
     * @param channel_id Defines the channel's identifier where the object should be created.
     * @param enforcement_object_id EnforcementObject identifier.
     * @param differentiation_pair Defines the operation_type and operation_context I/O classifiers
     * of the EnforcementObject; the differentiation token is computed based on these values.
     * @param object_type Type of the EnforcementObject to be created.
     * @param configurations Initial configurations of the EnforcementObject.
     * @return Returns PStatus::OK if the object was successfully created, and PStatus::Error
     * otherwise.
     */
    PStatus create_enforcement_object (const long& channel_id,
        const long& enforcement_object_id,
        const ObjectDifferentiationPair& differentiation_pair,
        const EnforcementObjectType& object_type,
        const std::vector<long>& configurations);

    /**
     * define_enforcement_object_differentiation_with_token: Defines how enforcement object
     * differentiation is made. Namely, it serve to decide by which enforcement object each request
     * should be handled.
     * Concurrency control handled at object_differentiation object; CC not handled at m_channels
     * because its read-only (careful when update/remove Channel is supported).
     * @param channel_token Differentiation token a the channel that contains the respective
     * enforcement object.
     * @param operation_type Boolean that defines if the request's operation type should be
     * considered.
     * @param operation_context Boolean that defines if the request's operation context should be
     * considered.
     * @return Returns PStatus::OK if the enforcement object differentiation was made; and
     * PStatus::Error otherwise.
     */
    PStatus define_enforcement_object_differentiation_with_channel_token (
        const diff_token_t& channel_token,
        const bool& operation_type,
        const bool& operation_context);

    /**
     * select_channel: Verify and select a given channel of the data plane stage, which can be used
     * to update its internal organization (enforcement objects), enforcement of I/O requests, and
     * statistic collection.
     * Read-only method (at m_channels). Concurrency control to m_channels is handle by upper
     * methods.
     * @param channel_token Differentiation token of the corresponding channel.
     * @return Returns a pointer to the corresponding channel. If the channel does not exists, it
     * returns nullptr.
     */
    [[nodiscard]] ChannelDefault* select_channel (const diff_token_t& channel_token) const;

    /**
     * does_channel_token_exist: Verify if a given channel exists in the m_channels container.
     * Read-only method. Concurrency control to m_channels is handle by the upper methods.
     * @param channel_token Differentiation token of the corresponding channel.
     * @return Returns true if the channel exists, and false otherwise.
     */
    bool does_channel_token_exist (const diff_token_t& channel_token) const;

    /**
     * does_channel_id_exist: Verify if a given channel exists in the m_channel_id_to_token_linkers
     * container.
     * Read-only method. Concurrency control over m_channel_id_to_token_linkers.
     * @param channel_id Channel identifier of the corresponding channel.
     * @return Returns true if the channel exists, and false otherwise.
     */
    bool does_channel_id_exist (const long& channel_id);

public:
    /**
     * Core default constructor.
     * The method defines a default Channel-level I/O differentiation.
     */
    Core ();

    /**
     * Core parameterized constructor.
     * This method defines a default Channel-level I/O differentiation.
     * @param channels Defines a default number of channels to be created. The I/O differentiation
     * object is created with default values, with the workflow-id, operation_type, and
     * operation_context I/O classifiers set with the channel identifier and PAIO_GENERAL::no_op
     * values, respectively.
     * @param create_default_channels Defines if channels should be created by default or if should
     * be created through HousekeepingRule specification (local file or control plane). If true, it
     * creates a total of @param channels.
     * @param default_enforcement_objects Boolean that defines of a default EnforcementObject should
     * be created in each Channel. The I/O differentiation object is created with default values,
     * with the operation_type and operation_context I/O classifiers set with PAIO_GENERAL::no_op
     * The EnforcementObject type is set to Noop.
     */
    Core (const int& channels,
        const bool& create_default_channels,
        const bool& default_enforcement_objects);

    /**
     * Core default destructor.
     */
    ~Core ();

    /**
     * define_channel_differentiation: Defines how Channel selection (I/O classification and
     * differentiation) is made. Namely, it will serve to decide by which channel each request
     * should be handled.
     * Concurrency control is handled by the m_channel_diff_builder object.
     * @param workflow Boolean that defines if the workflow-id I/O classifier should be considered.
     * @param operation_type Boolean that defines if the request's operation type I/O classifier
     * should be considered.
     * @param operation_context Boolean that defines if the request's operation context I/O
     * classifier should be considered.
     */
    void define_channel_differentiation (const bool& workflow,
        const bool& operation_type,
        const bool& operation_context);

    /**
     * define_enforcement_object_differentiation: Defines how enforcement object differentiation is
     * made. Namely, it serve to decide by which enforcement object each request should be handled.
     * Concurrency control handled at object_differentiation object; CC not handled at m_channels
     * because its read-only (careful when update/remove Channel is supported).
     * @param channel_id Identifier of the channel that contains the respective enforcement object.
     * @param operation_type Boolean that defines if the request's operation type should be
     * considered.
     * @param operation_context Boolean that defines if the request's operation context should be
     * considered.
     * @return Returns PStatus::OK if the enforcement object differentiation was made; and
     * PStatus::Error otherwise.
     */
    [[maybe_unused]] PStatus define_enforcement_object_differentiation (const long& channel_id,
        const bool& operation_type,
        const bool& operation_context);

    /**
     * enforce_request: Enforce a specific storage mechanisms over the I/O request.
     * The selects the correct channel that will handle the I/O request, which will then select the
     * correct EnforcementObject that will apply the corresponding enforcement mechanisms over the
     * request.
     * Read-only method (over m_channels). Concurrency control is handled at channel-level; CC over
     * m_channels needs to be handled if update/remove channels are supported.
     * @param request_context Context object that contains the necessary metadata to apply the
     * correct enforcement mechanism over the I/O request.
     * @param buffer Content to be enforced.
     * @param buffer_size Size of the passed data content.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     */
    void enforce_request (const Context& request_context,
        const void* buffer,
        const size_t& buffer_size,
        Result& result);

    /**
     * list_channels: List string value of each channel in the data plane stage.
     * For each channel, the method invokes channel.to_string and emplaces the result in 'channels'
     * container.
     * Concurrency control over m_channels.
     * @param channels Reference to a container that will store the string representation of each
     * channel currently installed in the data plane stage.
     */
    [[maybe_unused]] void list_channels (std::vector<std::string>& channels);

    /**
     * insert_housekeeping_rule: Insert an HousekeepingRule in Core's HousekeepingTable
     * (m_housekeeping_table). This method insert the rule using the parameterized constructor.
     * Concurrency control is handled by HousekeepingTable object.
     * @param rule_id Identifier of the housekeeping rule object.
     * @param operation Defines the operations of the housekeeping rule.
     * @param channel_id Defines the identifier of the channel that the rule respects to.
     * @param enforcement_object_id Defines the identifier of the EnforcementObject that the rule
     * respects to.
     * @param properties Defines the additional properties of the housekeeping rule.
     * @return Returns PStatus::OK if the rule was successfully inserted; PStatus::Error otherwise.
     */
    [[maybe_unused]] PStatus insert_housekeeping_rule (const uint64_t& rule_id,
        const HousekeepingOperation& operation,
        const long& channel_id,
        const long& enforcement_object_id,
        const std::vector<long>& properties);

    /**
     * insert_housekeeping_rule: Insert an HousekeepingRule in Core's HousekeepingTable
     * (m_housekeeping_table). This method insert the rule using the copy constructor.
     * Concurrency control is handled by HousekeepingTable object.
     * @param rule HousekeepingRule to be inserted.
     * @return Returns PStatus::OK if the rule was successfully inserted; PStatus::Error otherwise.
     */
    PStatus insert_housekeeping_rule (const HousekeepingRule& rule);

    /**
     * execute_housekeeping_rule: Execute a specific HousekeepingRule.
     * The method receives the identifier of the rule to be employed, and verifies its housekeeping
     * operation to apply the respective method, namely create channels, create enforcement objects,
     * or others.
     * Concurrency control over create_channel and create_object branches.
     * @param rule_id Identifier of the HousekeepingRule to be employed.
     * @return Returns PStatus::OK if the rule is successfully employed, PStatus::Enforced if the
     * rule was already enforced, PStatus::NotSupported if the housekeeping operation is currently
     * not supported, and PStatus::Error for the remainder scenarios.
     */
    PStatus execute_housekeeping_rule (const uint64_t& rule_id);

    /**
     * execute_housekeeping_rules: Execute all staged HousekeepingRule objects.
     * The method traverses the HousekeepingTable and employs all housekeeping rules that are left
     * to be executed (i.e., rule.m_enforced == false). If a rule is not successfully employed, the
     * method stops its execution.
     * @return Returns PStatus::OK if all rules were successfully employed, and PStatus::Error
     * otherwise.
     */
    PStatus execute_housekeeping_rules ();

    /**
     * list_housekeeping_table_rules: List all HousekeepingRules in the HousekeepingTable.
     * Concurrency control is handled by HousekeepingTable object.
     * @return Returns a string with all HousekeepingRules in the table.
     */
    std::string list_housekeeping_table_rules ();

    /**
     * employ_enforcement_rule: Employ an enforcement rule over a given enforcement object (through
     * the combination of channel-id and enforcement-object-id). Currently, upon receiving a rule,
     * it is immediately enforced.
     * Concurrency control over m_channel_id_to_token_linkers; Concurrency control is handled at
     * channel-level; CC over m_channels needs to be handled if update/remove are supported.
     * As future work, it would be interesting to store and postpone the employment of enforcement
     * rules (e.g., upon a certain threshold is hit, or time point).
     * @param channel_id Channel identifier where the enforcement rule should be employed.
     * @param enforcement_object_id EnforcementObject identifier where the enforcement rule should
     * be employed.
     * @param enforcement_rule_type Type of the enforcement rule to be employed.
     * @param configurations Configuration parameters to be set.
     * @return Returns PStatus::OK if enforcement rule is successfully employed, and PStatus::Error
     * otherwise (e.g., channel and enforcement object do not exist, rule type is not valid, ...).
     */
    PStatus employ_enforcement_rule (const long& channel_id,
        const long& enforcement_object_id,
        const int& enforcement_rule_type,
        const std::vector<long>& configurations);

    /**
     * collect_enforcement_object_statistics: collect statistics of a specific EnforcementObject.
     * Concurrency control over m_channel_id_to_token_linkers; Concurrency control is handled at
     * channel-level; CC over m_channels needs to be handled if update/remove are supported.
     * @param channel_id Channel identifier where the enforcement object is placed at.
     * @param enforcement_object_id EnforcementObject identifier.
     * @param object_stats_raw ObjectStatisticsRaw object to store statistic entries.
     * @return Returns PStatus::OK if statistics were successfully collected, and PStatus::Error
     * otherwise (e.g., channel or enforcement object do not exist, ...).
     */
    PStatus collect_enforcement_object_statistics (const long& channel_id,
        const long& enforcement_object_id,
        ObjectStatisticsRaw& object_stats_raw);

    /**
     * collect_channel_statistics: collect general statistics of a specific channel.
     * Concurrency control over m_channel_id_to_token_linkers; Concurrency control is handled at
     * channel-level; CC over m_channels needs to be handled if update/remove are supported.
     * @param channel_id Channel identifier.
     * @param channel_stats ChannelStatsRaw object to store statistics entries.
     * @return Returns PStatus::OK if statistics were successfully collected, and  PStatus::Error
     * otherwise (e.g., channel does not exist, entry does not exist, statistic collection is
     * disabled, ...).
     */
    PStatus collect_channel_statistics (const long& channel_id, ChannelStatsRaw& channel_stats);

    /**
     * collect_channel_statistics_detailed: collect detailed statistics of a specific channel.
     * Concurrency control over m_channel_id_to_token_linkers; Concurrency control is handled at
     * channel-level; CC over m_channels needs to be handled if update/remove are supported.
     * @param channel_id Channel identifier.
     * @param detailed_stat_entries Container to store detailed statistics entries.
     * @return Returns PStatus::OK if statistics were successfully collected, and  PStatus::Error
     * otherwise (e.g., channel does not exist, entry does not exist, statistic collection is
     * disabled, ...).
     */
    PStatus collect_channel_statistics_detailed (const long& channel_id,
        std::vector<double>& detailed_stat_entries);

    /**
     * get_total_channels: get the number of channels operating in the data plane stage.
     * Concurrency control over m_channel_id_to_token_linkers.
     * @return Returns the size of m_channel_id_to_token_linkers container.
     */
    int get_total_channels ();

    /**
     * get_channels_identifiers: copy the channels identifiers in the m_channel_id_to_token_linkers
     * to the channels_ids container.
     * Concurrency control over m_channel_id_to_token_linkers.
     * @param channels_ids Container that will hold all Channel identifiers.
     */
    void get_channels_identifiers (std::vector<long>& channels_ids);

    /**
     * set_default_object_differentiation: set new value for m_default_object_differentiation,
     * atomically. By default, it uses the
     * option_define_default_object_differentiation_on_create_channel option.
     * @param value New value to be set on m_define_default_object_differentiation.
     */
    [[maybe_unused]] void set_default_object_differentiation (const bool& value);

    // bool use_differentiation_table ();
    // PStatus insert_differentiation_rule (const DifferentiationRule& rule);
    // PStatus remove_differentiation_rule (const long& rule_id);
    // PStatus remove_channel (const long& channel_id);
    // PStatus remove_enforcement_object (const long& channel_id, const long& object_id);
};
} // namespace paio::core
#endif // PAIO_CORE_HPP
