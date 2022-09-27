/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_INTERFACE_DEFINITIONS_HPP
#define PAIO_INTERFACE_DEFINITIONS_HPP

#include <climits>
#include <paio/core/context_propagation_definitions.hpp>
#include <paio/rules/housekeeping_rule.hpp>

/**
 * InterfaceDefinitions header.
 * This header defines the structures that will be used to exchange messages between the data plane
 * and the control plane.
 * TODO:
 *  - possibly change all structures to dedicated classes;
 *  - revisit the need (and remove if not needed) of the ControlResponse, ACK, Execute, and
 *  CollectStatisticsMetadata structs;
 *  - remove StatsSilkRaw and StatsTensorFlowRaw structs;
 *  - change the ObjectStatisticsRaw.m_token_bucket_statistics to a more generic object, otherwise
 *  the struct can only hold statistics of DRL (token-bucket) enforcement objects;
 *  - change how configurations are defined on the HousekeepingCreateObjectRaw and
 *  EnforcementRuleRaw struct, use a dedicated object or a container of configurations;
 *  - support DifferentiationRule in SouthboundConnectionHandler and remainder classes;
 *  - support ChannelDifferentiationClassifiersRaw and ObjectDifferentiationClassifiersRaw;
 *  - remove collect_stats_rocksdb and collect_stats_tensorflow from ControlPlaneOperationSubtype;
 */
namespace paio::core {

/**
 * ControlPlaneOperationType definitions.
 * Defines the main operations that can be performed by the control plane in the data plane stage.
 * Currently, it includes the following operations:
 *  - stage_handshake: establishes the a separate connection between the control plane and the data
 *  plane stage, while simultaneously listing information about the stage, including the stage's
 *  name, pid, hostname, and more;
 *  - mark_stage_ready: marks the stage as ready to receive requests from the targeted I/O layer;
 *  - collect_stats: collect general (windowed) statistics from the data plane stage;
 *  - collect_detailed_stats: collect each (windowed) statistic entry from the data plane stage;
 *  - create_hsk_rule: create a new HousekeepingRule;
 *  - create_dif_rule: create a new DifferentiationRule;
 *  - create_enf_rule: create a new EnforcementRule;
 *  - exec_hsk_rules: execute all stored HousekeepingRules;
 *  - remove_rule: remove a specific rule;
 */
enum class ControlPlaneOperationType {
    stage_handshake = 0,
    mark_stage_ready = 1,
    collect_stats = 2,
    collect_detailed_stats = 3,
    create_hsk_rule = 4,
    create_dif_rule = 5,
    create_enf_rule = 6,
    exec_hsk_rules = 7,
    remove_rule = 8
};

/**
 * ControlPlaneOperationSubtype definitions.
 * Defines a subclass of operations that are performed by the control plane in the data plane stage.
 * Currently, it includes the following operations:
 *  - hsk_create_channel: create a new channel in the data plane stage;
 *  - hsk_create_object: create a new enforcement object in the data plane stage.
 */
enum class ControlPlaneOperationSubtype {
    hsk_create_channel = 1,
    hsk_create_object = 2,
    collect_stats_rocksdb = 3, // fixme: remove ...
    collect_stats_tensorflow = 4, // fixme: remove ...
    collect_stats_global = 5,
    collect_stats_metadata_data = 6,
    collect_stats_mds = 7,
    no_op = 0
};

/**
 * ControlOperation structure.
 * Defines the metadata of the operation to be received from the control plane.
 *  - m_operation_id: defines the control operation identifier;
 *  - m_operation_type: defines the type of operation to be received (housekeeping rule, enforcement
 *  rule, collect statistics, ...);
 *  - m_operation-subtype: defines the subtype of the operation to be received (create channel,
 *  create object, ...);
 *  - m_size: defines the size of the object to be received.
 */
struct ControlOperation {
    int m_operation_id;
    int m_operation_type;
    int m_operation_subtype;
    int m_size;
};

/**
 * ControlResponse structure.
 * Defines the metadata for submitting messages to the control plane.
 */
struct ControlResponse {
    int m_response;
};

/**
 * Acknowledgement codes.
 * Defines the codes for the acknowledgment messages for the communication between the control plane
 * and data plane stage.
 */
enum class AckCode { ok = 1, error = 0 };

/**
 * ACK structure. Defines if a command was executed or not, returning ACK_OK if successfully
 * executed, and ACK_ERROR otherwise.
 */
struct ACK {
    int m_message;
};

/**
 * stage_name_max_size: defines the maximum size of the StageInfo's name.
 */
const int stage_name_max_size = 200;

/**
 * stage_opt_max_size: defines the maximum size of the StageInfo's opt.
 */
const int stage_opt_max_size = 50;

/**
 * define HOST_NAME_MAX and LOGIN_NAME_MAX limits, for the StageInfo's hostname and login_name.
 */
#ifndef __USE_POSIX
#define HOST_NAME_MAX  64
#define LOGIN_NAME_MAX 64
#endif

/**
 * StageInfoRaw: Raw structure that identifies the Data Plane Stage.
 * - m_stage_name: defines the stage identifier (name).
 * - m_stage_opt: defines the environment variable value registered for the data plane stage;
 * - m_pid: defines the pid of the process where the data plane stage is executing;
 * - m_ppid: defines the parent pid of the process where the data plane stage is executing.
 * - m_stage_hostname: defines the hostname where the data plane stage is executing;
 * - m_stage_login_name: defines the login name of the user that is executing the data plane stage.
 */
struct StageInfoRaw {
    char m_stage_name[stage_name_max_size] {};
    char m_stage_opt[stage_opt_max_size] {};
    int m_pid { -1 };
    int m_ppid { -1 };
    char m_stage_hostname[HOST_NAME_MAX] {};
    char m_stage_login_name[LOGIN_NAME_MAX] {};
};

/**
 * stage_max_handshake_address_size: defines the maximum size of the address to where the data plane
 * should connect after the handshake with the control plane.
 */
const int stage_max_handshake_address_size = 100;

/**
 * StageHandshakeRaw: Raw structure that identifies the address and port where the data plane should
 * connect after the handshake with the control plane.
 *  - m_address: defines the connection address;
 *  - m_port: defines the connection port. In the case of CommunicationType::unix, this value will
 *  not be used.
 */
struct StageHandshakeRaw {
    char m_address[stage_max_handshake_address_size] {};
    int m_port { -1 };
};

/**
 * stage_handshake_raw_string: convert StageHandshakeRaw to string.
 * @param stage_handshake_raw StageHandshakeRaw object.
 * @return Returns a string representation of the StageHandshakeRaw object.
 */
inline std::string stage_handshake_raw_string (const StageHandshakeRaw& stage_handshake_raw)
{
    std::string message { "StageHandshakeRaw:\n" };
    message.append ("\taddress: ").append (stage_handshake_raw.m_address).append (" (");
    message.append (std::to_string (sizeof (stage_handshake_raw.m_address))).append (")\n");
    message.append ("\tport: ").append (std::to_string (stage_handshake_raw.m_port)).append ("\n");
    return message;
}

/**
 * stage_info_raw_string: convert StageInfoRaw to string.
 * @param handshake_object StageInfoRaw object.
 * @return Returns a string representation of the StageInfoRaw object.
 */
inline std::string stage_info_raw_string (const StageInfoRaw& handshake_object)
{
    std::string message { "StageHandshakeRaw deserialization:\n" };
    message.append ("\tname : ").append (handshake_object.m_stage_name).append (" (");
    message.append (std::to_string (sizeof (handshake_object.m_stage_name))).append (")\n");
    message.append ("\topt : ").append (handshake_object.m_stage_opt).append (" (");
    message.append (std::to_string (sizeof (handshake_object.m_stage_opt))).append (")\n");
    message.append ("\tpid : ").append (std::to_string (handshake_object.m_pid)).append ("\n");
    message.append ("\tppid : ").append (std::to_string (handshake_object.m_ppid)).append ("\n");
    message.append ("Size of struct: ")
        .append (std::to_string (sizeof (StageInfoRaw)))
        .append ("\n");

    return message;
}

/**
 * HousekeepingCreateChannelRaw: Raw structure to perform the serialization of HousekeepingRules of
 * type create_channel between the PAIO stage and the control plane.
 * - m_rule_id: defines the rule identifier;
 * - m_rule_type: defines the HousekeepingRule operation type (create_channel);
 * - m_channel_id: defines the Channel identifier;
 * - m_context_definition: defines the context definition used in this rule;
 * - m_workflow_id: defines the workflow identifier to be used for the classification and
 * differentiation of requests (including the creation of the differentiation token);
 * - m_operation_type: defines the operation type to be used for the classification and
 * differentiation of requests (including the creation of the differentiation token);
 * - m_operation_context: defines the operation context to be used for the classification and
 * differentiation of requests (including the creation of the differentiation token).
 */
struct HousekeepingCreateChannelRaw {
    uint64_t m_rule_id { 0 };
    int m_rule_type { static_cast<int> (rules::HousekeepingOperation::create_channel) };
    long m_channel_id { -1 };
    int m_context_definition { static_cast<int> (ContextType::PAIO_GENERAL) };
    uint32_t m_workflow_id { 0 };
    uint32_t m_operation_type { static_cast<uint32_t> (PAIO_GENERAL::no_op) };
    uint32_t m_operation_context { static_cast<uint32_t> (PAIO_GENERAL::no_op) };
};

/**
 * HousekeepingCreateObjectRaw: Raw structure to perform the serialization of HousekeepingRules of
 * type create_object between the PAIO stage and the control plane.
 * - m_rule_id: defines the rule identifier;
 * - m_rule_type: defines the HousekeepingRule identifier (create_object);
 * - m_channel_id: defines the Channel identifier where the object should be created;
 * - m_enforcement_object_id: defines the EnforcementObject identifier;
 * - m_context_definition: defines the context definition used in this rule;
 * - m_operation_type: defines the operation type to be used for the classification and
 * differentiation of requests (including the creation of the differentiation token);
 * - m_operation_context: defines the operation context to be used for the classification and
 * differentiation of requests (including the creation of the differentiation token);
 * - m_enforcement_object_type: defines the type of the EnforcementObject to be created;
 * - m_property_first: defines the initial property (first) of the EnforcementObject;
 * - m_property_second: defines the initial property (second) of the EnforcementObject.
 */
struct HousekeepingCreateObjectRaw {
    long m_rule_id { 0 };
    int m_rule_type { static_cast<int> (rules::HousekeepingOperation::create_object) };
    long m_channel_id { -1 };
    long m_enforcement_object_id { -1 };
    int m_context_definition { static_cast<int> (ContextType::PAIO_GENERAL) };
    uint32_t m_operation_type { static_cast<uint32_t> (PAIO_GENERAL::no_op) };
    uint32_t m_operation_context { static_cast<uint32_t> (PAIO_GENERAL::no_op) };
    long m_enforcement_object_type { 0 };
    long m_property_first { 0 };
    long m_property_second { 0 };
};

/**
 * ChannelDifferentiationClassifiersRaw: Raw structure that defines the which I/O classifiers
 * should be considered in the I/O classification and differentiation at Channels. The rule is
 * targeted for the overall data plane (i.e, all Channels).
 * - m_workflow_id: Boolean that defines if the workflow identifier I/O classifier should be
 * considered in I/O differentiation.
 * - m_operation_type: Boolean that defines if the operation type I/O classifier should be
 * considered in I/O differentiation.
 * - m_operation_context: Boolean that defines if the operation context I/O classifier should be
 * considered in I/O differentiation.
 */
struct ChannelDifferentiationClassifiersRaw {
    bool m_workflow_id { true };
    bool m_operation_type { false };
    bool m_operation_context { false };
};

/**
 * ObjectDifferentiationClassifiersRaw: Raw structure that defines the which I/O classifiers should
 * be considered in the I/O classification and differentiation at EnforcementObjects. The rule is
 * targeted for a specific Channel.
 * - m_channel_id: defines the identifier of the Channel that will assume these rules.
 * - m_operation_type: Boolean that defines if the operation type I/O classifier should be
 * considered in I/O differentiation.
 * - m_operation_context: Boolean that defines if the operation context I/O classifier should be
 * considered in I/O differentiation.
 */
struct ObjectDifferentiationClassifiersRaw {
    long m_channel_id { -1 };
    bool m_operation_type { false };
    bool m_operation_context { false };
};

/**
 * DifferentiationRuleRaw: Raw structure to perform the serialization of DifferentiationRules
 * between the PAIO stage and the control plane.
 * - m_rule_id: defines the rule identifier;
 * - m_rule_type: defines the type of the differentiation operation, namely if the rule respects to
 * the I/O differentiation at the channel or enforcement object;
 * - m_channel_id: defines the Channel identifier;
 * - m_enforcement_object_id: defines the EnforcementObject identifier;
 * - m_workflow_id: defines the workflow identifier classifier to perform the differentiation;
 * - m_operation_type: defines the operation type classifier to perform the differentiation;
 * - m_operation_context: defines the operation context classifier to perform the differentiation.
 */
struct DifferentiationRuleRaw {
    long m_rule_id { 0 };
    int m_rule_type { 0 }; // static_cast<int> (DifferentiationRuleType::none)
    long m_channel_id { -1 };
    long m_enforcement_object_id { -1 };
    uint32_t m_workflow_id { 0 };
    uint32_t m_operation_type { 0 };
    uint32_t m_operation_context { 0 };
};

/**
 * EnforcementRuleRaw: Raw structure to perform the serialization of EnforcementRules between the
 * PAIO stage and control the plane.
 * - m_rule_id: defines the rule identifier;
 * - m_channel_id: defines the Channel identifier that contains the object to be enforced;
 * - m_enforcement_object_id: defines the EnforcementObject identifier to be enforced;
 * - m_enforcement_object_type: defines the type of the EnforcementObject to be created;
 * - m_enforcement_operation: defines the operation that should be enforced over the object (e.g.,
 * init, rate, ...);
 * - m_property_first: defines the property (first) to be set in the EnforcementObject;
 * - m_property_second: defines the property (second) to be set in the EnforcementObject;
 * - m_property_third: defines the property (third) to be set in the EnforcementObject;
 */
struct EnforcementRuleRaw {
    long m_rule_id { 0 };
    long m_channel_id { -1 };
    long m_enforcement_object_id { -1 };
    int m_enforcement_operation { 0 };
    long m_property_first { -1 };
    long m_property_second { -1 };
    long m_property_third { -1 };
};

/**
 * StageReadyRaw: Raw structure that defines if the data plane is ready to receive I/O requests
 * from the targeted layer.
 * - m_mark_stage: boolean that defines if the data plane stage is ready to receive I/O requests
 * from the I/O layer.
 */
struct StageReadyRaw {
    bool m_mark_stage { false };
};

/**
 * ChannelStatsRaw: Raw structure to perform the serialization of general statistics of a given
 * Channel, which will be transmitted between the PAIO stage and the control plane.
 * - m_channel_id: defines the Channel identifier to collect the statistics;
 * - m_overall_metric_value: provides the aggregated statistic counter of all entries across the
 * entire collection period;
 * - m_windowed_metric_value: provides the aggregated (windowed) statistic counter of all entries
 * of the last collection period.
 */
struct ChannelStatsRaw {
    long m_channel_id { -1 };
    double m_overall_metric_value { -1 };
    double m_windowed_metric_value { -1 };
};

/**
 * TBStatsRaw: Raw structure to perform the serialization of TokenBucket statistic entries.
 * - m_normalized_empty_bucket: defines the normalized value that marks when the token-bucket could
 * not consume;
 * - m_tokens_left: defines the number of tokens left when the token-bucket could not consume.
 */
struct TBStatsRaw {
    float m_normalized_empty_bucket { 0 };
    double m_tokens_left { 0 };
};

/**
 * object_statistics_entries_size: defines the maximum size of the ObjectStatisticsRaw's entries.
 */
const int object_statistics_entries_size = 100;

/**
 * ObjectStatisticsRaw: Raw structure to perform the serialization of the EnforcementObject
 * statistics, which will be transmitted between the PAIO stage and the control plane.
 * - m_channel_id: defines the Channel identifier where the EnforcementObject is placed;
 * - m_enforcement_object_id: defines the EnforcementObject identifier through where statistics
 * should be collected;
 * - m_total_stats: defines the number of entries in the m_object_statistic_entries;
 * - m_object_statistic_entries: contains the EnforcementObject statistic entries (change the type
 * for a more general object).
 */
struct ObjectStatisticsRaw {
    long m_channel_id { -1 };
    long m_enforcement_object_id { -1 };
    int m_total_stats { 0 };
    TBStatsRaw m_object_statistic_entries[object_statistics_entries_size] {};
};

/**
 * CollectStatisticsMetadata: Raw structure that defines which Channel to collect statistics.
 * If channel_id = -1, collect the statistics of all Channels; otherwise, returns the statistics of
 * the selected Channel.
 */
struct CollectStatisticsMetadata {
    long m_channel_id { -1 };
    int m_number_of_channels { 0 };
};

/**
 * StatsSilkRaw: Raw structure to perform the serialization of I/O statistics between the PAIO stage
 * and the control plane. These statistics are specific to the "Tail latency control in key-value
 * stores" use case of the PAIO paper, where we demonstrate tail latency control similar to the
 * SILK key-value store.
 * - m_fg_tasks: provides the aggregated throughput value of the windowed statistics, of the
 * LSM_KVS_DETAILED::foreground request type, of all channels;
 * - m_bg_tasks_flush: provides the aggregated throughput value of the windowed statistics, of the
 * LSM_KVS_DETAILED::bg_flush request type, of all channels;
 * - m_bg_tasks_compaction_l0: provides the aggregated throughput value of the windowed statistics,
 * of high priority compactions LSM_KVS_DETAILED::bg_compaction_L0_L0 + ::bg_compaction_L0_L1, of
 * all channels;
 * - m_bg_tasks_compaction_lN: provides the aggregated throughput value of the windowed statistics,
 * of low priority compactions LSM_KVS_DETAILED::bg_compaction_L1_L2 + ::bg_compaction_L2_L3 +
 * ::bg_compaction_LN, of all channels.
 */
struct StatsSilkRaw {
    double m_fg_tasks;
    double m_bg_tasks_flush;
    double m_bg_tasks_compaction_l0;
    double m_bg_tasks_compaction_lN;
};

/**
 * StatsTensorFlowRaw: Raw structure to perform the serialization of I/O statistics between the
 * PAIO stage and the control plane. These statistics are specific to the "Per-application bandwidth
 * control" use case of the PAIO paper, where we demonstrate how to ensure QoS bandwidth across
 * multiple applications sharing the same storage device.
 * - m_read_rate: defines the aggregated throughput value of the windowed reads.
 * - m_write_rate: defines the aggregated throughput value of the windowed writes.
 */
struct StatsTensorFlowRaw {
    double m_read_rate;
    double m_write_rate;
};

/**
 * StatsGlobalRaw: Raw structure to perform the serialization of I/O statistics between the PAIO
 * stage and the control plane. These statistics are general, and provide the aggregated throughput
 * value of the windowed statistics.
 */
struct StatsGlobalRaw {
    double m_total_rate;
};

/**
 * StatsDataMetadataRaw: Raw structure to perform the serialization of I/O statistics between the
 * PAIO stage and the control plane. These are general statistics regarding the data and metadata
 * operations, and provide the aggregated throughput value of the windowed statistics.
 */
struct StatsDataMetadataRaw {
    double m_total_metadata_rate;
    double m_total_data_rate;
};

} // namespace paio::core

#endif // PAIO_INTERFACE_DEFINITIONS_HPP
