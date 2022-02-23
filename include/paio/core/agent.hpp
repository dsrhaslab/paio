/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_AGENT_HPP
#define PAIO_AGENT_HPP

#include <map>
#include <paio/core/core.hpp>
#include <paio/core/stage_info.hpp>
#include <paio/rules/housekeeping_rule.hpp>
#include <paio/utils/rules_parser.hpp>
#include <paio/utils/status.hpp>

using namespace paio::rules;

namespace paio::core {

/**
 * Agent class.
 * The Agent class serves as a mediator between the ConnectionManager/SouthboundInterface and the
 * Core class. When the data plane is operating without control plane, this methods reads rules
 * from files and installs them at the data plane stage. When operating with control plane, the
 * SouthboundInterface will receive the rules and submit them to the Agent class to be treated.
 * Currently, the Agent class contains the following variables:
 * - m_core: Core object that contains the main classification, differentiation, and enforcement
 * structures for enforcing policies over requests; this variables is shared between the Agent and
 * the PaioStage classes.
 * - m_ready: atomic boolean value that marks the data plane stage ready to receives requests. This
 * variables is marked as ready either when all rules read from files are applied (when executing
 * without control plane), or when the control plane explicitly marks the stage as ready. This
 * variable is shared between the Agent, PaioStage, and ConnectionManager classes.
 * - m_stage_identifier: StageInfo class that characterizes the data plane stage, including its
 * name, description, and process identifiers. This variables is shared between the Agent and
 * PaioStage classes.
 * - m_housekeeping_rule_file: path to file where the housekeeping rule file should be accessed.
 * - m_differentiation_rule_file: path to file where the differentiation rule file should be
 * accessed.
 * - m_enforcement_rule_file: path to file where the enforcement rule file should be accessed.
 * - m_execute_on_receive: atomic bool value that defines if HousekeepingRules should be executed
 * on receive (from the SDS control plane), or be stored for later enforcement.
 * TODO:
 *  - support dedicated tests for employ_housekeeping_rule, employ_enforcement_rule, get_stage_name,
 *  serialize_stage_info, set_execute_on_receive, collect_channel_statistics,
 *  collect_detailed_channel_statistics, and collect_enforcement_object_statistics methods;
 *  - support the following methods: execute_housekeeping_rule, remove_housekeeping_rule,
 *  remove_differentiation_rule, and employ_differentiation_rule.
 */
class Agent {

    // friend classes of Agent
    friend class AgentTest;

private:
    std::shared_ptr<Core> m_core { nullptr };
    std::shared_ptr<std::atomic<bool>> m_ready { nullptr };
    std::shared_ptr<StageInfo> m_stage_identifier { nullptr };
    fs::path m_housekeeping_rule_file { option_default_housekeeping_rules_file_path () };
    fs::path m_differentiation_rule_file { option_default_differentiation_rules_file_path () };
    fs::path m_enforcement_rule_file { option_default_enforcement_rules_file_path () };
    std::atomic<bool> m_execute_on_receive { option_execute_rule_on_receive };

    /**
     * insert_housekeeping_rules_from_file: Read and create HousekeepingRules specified on the file
     * path. The number of HousekeepingRules of type create_channel will be created based on the
     * total rules, passed as argument.
     * @param path Absolute path to the HousekeepingRules file.
     * @param total_rules Number of HousekeepingRules to be inserted.
     */
    PStatus insert_housekeeping_rules_from_file (const fs::path& path, const int& total_rules);

    /**
     * insert_differentiation_rules_from_file: Read and create DifferentiationRules specified on
     * the file path. The number of rules will be created based on the total rules, passed as
     * argument, and can target both channels and enforcement objects.
     * @param path Absolute path to the DifferentiationRules file.
     * @param total_rules Number of differentiation rules to be created.
     * @return Returns the number of differentiation rules successfully applied.
     */
    int insert_differentiation_rules_from_file (const fs::path& path, const int& total_rules);

    /**
     * insert_enforcement_rules_from_file: Read and create the EnforcementRules specified on the
     * file path. The number of rules to be defines are based on the total rules, passed as
     * parameter, and are targeted for existing EnforcementObjects
     * @param path Absolute path to the EnforcementRules file.
     * @param total_rules Number of EnforcementRules to be employed.
     * @return Returns the number of enforcement rules successfully applied.
     */
    int insert_enforcement_rules_from_file (const fs::path& path, const int& total_rules);

    /**
     * mark_ready: Mark the data plane stage ready to receive I/O requests from the I/O stack.
     * This method is internal to the Agent class, and should only be triggered after executing the
     * default housekeeping, differentiation, and enforcement rules (when the communication type is
     * set to CommunicationType::none).
     */
    void mark_ready ();

    /**
     * print_housekeeping_rules_in_core: Print all HousekeepingRules stored in the Core's
     * HousekeepingTable.
     * @return Returns a string with all HousekeepingRules in the table.
     */
    std::string print_housekeeping_rules_in_core ();

public:
    /**
     * Agent default constructor.
     * This constructor should only be used if the Stage class does not exist.
     * It initializes m_ready shared pointers with default settings. m_core is initialized, but not
     * shared with any other class. m_stage_identifier is initialized with a boilerplate name.
     * The default rule files (namely, housekeeping, differentiation and enforcement) are
     * initialized with the default options (header) properties, and are will be inserted/executed
     * in case the default communication type if CommunicationType::none.
     */
    Agent ();

    /**
     * Agent parameterized constructor.
     * At creation time, the Agent constructor validates if the system is executing with or without
     * control plane. If the data plane is running without control plane (CommunicationType::none),
     * it reads, parses, installs, and enforces default Housekeeping, Differentiation, and
     * Enforcement rules for a given number of instances.
     * @param communication_type Communication type to be established with the control plane.
     * @param core Shared pointer to the enforcement Core object.
     * @param ready Shared pointer to a boolean value that marks that data plane is ready to receive
     * I/O operations.
     * @param instances Total of instances connecting to the data plane stage.
     * @param stage_identifier Shared pointer to a data plane stage identifier. Serves to identify
     * each of the stages when operating in a complex setting with several stages across different
     * nodes and layers.
     */
    Agent (const CommunicationType& communication_type,
        std::shared_ptr<Core> core,
        std::shared_ptr<std::atomic<bool>> ready,
        const int& instances,
        std::shared_ptr<StageInfo> stage_identifier);

    /**
     * Agent parameterized constructor.
     * At creation time, the Agent constructor validates if the system is executing with or without
     * control plane. If the data plane is running without control plane (CommunicationType::none),
     * it reads, parses, installs, and enforces default Housekeeping, Differentiation, and
     * Enforcement rules for a given number of instances.
     * @param communication_type Communication type to be established with the control plane.
     * @param core Shared pointer to the enforcement Core object.
     * @param ready Shared pointer to a boolean value that marks that data plane is ready to receive
     * I/O operations.
     * @param housekeeping_rules_file_path Absolute path to the default HousekeepingRule file.
     * @param differentiation_rules_file_path Absolute path to the default DifferentiationRule file.
     * @param enforcement_rules_file_path Absolute path to the default EnforcementRule file.
     * @param instances Total of instances connecting to the data plane stage.
     * @param stage_identifier Shared pointer to a data plane stage identifier. Serves to identify
     * each of the stages when operating in a complex setting with several stages across different
     * nodes and layers.
     * @param execute_on_receive Boolean that defines if HousekeepingRules should be enforced upon
     * receive. If false, HousekeepingRules will be stored in the HousekeepingRulesTable for being
     * later enforced upon manual input.
     */
    Agent (const CommunicationType& communication_type,
        std::shared_ptr<Core> core,
        std::shared_ptr<std::atomic<bool>> ready,
        const fs::path& housekeeping_rules_file_path,
        const fs::path& differentiation_rules_file_path,
        const fs::path& enforcement_rules_file_path,
        const int& instances,
        std::shared_ptr<StageInfo> stage_identifier,
        const bool& execute_on_receive);

    /**
     * Agent default destructor.
     */
    ~Agent ();

    /**
     * employ_housekeeping_rule: Employ an HousekeepingRule in the data plane stage.
     * It receives an HousekeepingRule and inserts it in the Core's HousekeepingRulesTable.
     * Depending on the m_execute_on_receive, the rule is directly enforced or postponed (until
     * receiving an execute_housekeeping_rules call).
     * This call can be invoked from the file-based rule insertion (i.e.,
     * insert_housekeeping_rules_from_file), or from the southbound-interface (i.e., originated
     * from the SDS control plane).
     * @param rule HousekeepingRule object to be inserted in the table and enforced.
     * @return Returns PStatus::Ok if the rule is successfully inserted (and executed); and
     * PStatus::Error otherwise.
     */
    PStatus employ_housekeeping_rule (const HousekeepingRule& rule);

    /**
     * execute_housekeeping_rules: Execute all HousekeepingRules stored in the Core's housekeeping
     * table that are left to be enforced.
     * @return Returns a PStatus::OK rules were successfully executed; PStatus::Error otherwise.
     */
    PStatus execute_housekeeping_rules ();

    /**
     * employ_differentiation_rule: Employ a DifferentiationRule in the data plane stage.
     * If the usage of the DifferentiationTable is disabled, the rule will not be inserted and an
     * error message will be written to the Logging's error file descriptor.
     * This call can be invoked from the file-based rule insertion (i.e.,
     * insert_differentiation_rules_from_file), or from the southbound-interface (i.e., originated
     * from the SDS control plane).
     * @param raw_diff_rule Differentiation rule in RAW format. The rule is converted into the
     * respective values.
     * @return Returns PStatus::Ok if the rule is successfully inserted (and executed);
     * PStatus::NotSupported if the DifferentiationTable is disabled; and PStatus::Error otherwise.
     */
    // PStatus employ_differentiation_rule (const DifferentiationRuleRaw& raw_diff_rule);

    /**
     * employ_enforcement_rule: Employ an EnforcementRule in the data plane stage.
     * It receives an EnforcementRule object, and enforces it over a specific storage service of
     * the data plane stage (namely, through a combination of channel and enforcement object).
     * Currently, this feature is specific for the usage of DynamicRateLimiting EnforcementObjects.
     * This will later be adjusted to be general purpose (i.e., support different object).
     * This call can be invoked from the file-based rule insertion (i.e.,
     * insert_enforcement_rules_from_file), or from the southbound-interface (i.e., originated
     * from the SDS control plane).
     * @param enforcement_rule Enforcement rule object to be employed.
     * @return Returns PStatus::Ok if the rule is successfully employed over the respective
     * EnforcementObject; PStatus::NotSupported if the service to be enforced is not supported; and
     * PStatus::Error otherwise.
     */
    PStatus employ_enforcement_rule (const EnforcementRule& enforcement_rule);

    /**
     * collect_channel_statistics: Collect general statistics from a single or multiple channels.
     * If the channel-id == -1, the method collects general statistics from all channels in the
     * data plane stage. Otherwise, it collects statistics from a single channel. If the collection
     * from any channel is unsuccessful, the methods stops and returns PStatus::Error.
     * This call is invoked from the southbound-interface (i.e., originated from the SDS control
     * plane).
     * @param channel_id Identifier of the channel to collect statistics. channel-id == -1 will
     * collect statistics from all channels.
     * @param channel_stats Container that emplace the statistics of a single or multiple channels.
     * @return Returns PStatus::OK if statistics of all specified channels were successfully
     * collected, and PStatus::Error otherwise.
     */
    PStatus collect_channel_statistics (const long& channel_id,
        std::vector<ChannelStatsRaw>& channel_stats);

    /**
     * collect_detailed_channel_statistics: Collect detailed statistics from a single or multiple
     * channels. If the channel-id == -1, the method collects detailed statistics from all channels
     * in the data plane stage. Otherwise, it collects statistics from a single channel. If the
     * collection from any channel is unsuccessful, the methods stops and returns PStatus::Error.
     * This call is invoked from the southbound-interface (i.e., originated from the SDS control
     * plane).
     * @param channel_id Identifier of the channel to collect statistics. channel-id == -1 will
     * collect statistics from all channels.
     * @param detailed_channel_stats Reference to a map that will store the channel-id (key) and
     * the respective detailed statistics (value) as a container of doubles.
     * @return Returns PStatus::OK if statistics of all specified channels were successfully
     * collected, and PStatus::Error otherwise.
     */
    PStatus collect_detailed_channel_statistics (const long& channel_id,
        std::map<long, std::vector<double>>& detailed_channel_stats);

    /**
     * collect_enforcement_object_statistics: Collect statistics of a single or multiple
     * EnforcementObjects.
     * The method traverses the map's keys, which are defined as a pair of channel-id and
     * enforcement-object-id, and invokes Core's collect_enforcement_object_statistics. If the
     * collection of any EnforcementObject fails, the method stops its execution.
     * @param object_stats_raw Map that contains as key a pair of channel-id and enforcement object
     * id. Values are ObjectStatisticsRaw objects, that will store the result of the statistic
     * collection operation.
     * This call is invoked from the southbound-interface (i.e., originated from the control plane.
     * @return Returns PStatus::OK if statistics for all specified EnforcementObjects were
     * successfully collected, and PStatus::Error otherwise.
     */
    PStatus collect_enforcement_object_statistics (
        std::map<std::pair<long, long>, ObjectStatisticsRaw>& object_stats_raw);

    /**
     * get_stage_identifier: Get data plane stage name.
     * The stage identifier will assist in the integration between the data plane stage and
     * applications, as well as the integration between the data plane stage and the control plane.
     * Specifically, it will help identify which rules are directed to this stage, as there can be
     * multiple data plane stages operating simultaneously.
     * Currently the identifier is set in string format.
     * @return Returns a new string with the value pointed by m_stage_identifier.
     */
    [[nodiscard, maybe_unused]] std::string get_stage_name () const;

    /**
     * get_stage_info: serialize StageInfo data into a StageInfoRaw object.
     * This method allows to establish the communication between the data plane stage and the
     * control plane. Specifically it allows to identify which rules are directed to this stage, as
     * there can be multiple data plane stages operating simultaneously.
     * @param handshake_obj Empty struct to be filled.
     */
    void get_stage_info (StageInfoRaw& handshake_obj);

    /**
     * set_execute_on_receive: Defines if HousekeepingRules should be executed on receive or
     * postponed, and be later enforced upon an explicit execute_rules () call.
     * @param value Boolean that defines a new value for m_execute_on_receive.
     */
    [[maybe_unused]] void set_execute_on_receive (const bool& value);

    /**
     * mark_data_plane_stage_ready: Mark the data plane stage ready to receive I/O requests from
     * the I/O stack.
     * This method should only be triggered when explicitly ordered by the control plane.
     */
    void mark_data_plane_stage_ready ();

    // PStatus execute_housekeeping_rule (const uint64_t& rule_id);
    // PStatus remove_housekeeping_rule (const uint64_t& rule_id);
    // PStatus remove_differentiation_rule (const uint64_t& rule_id);
};
} // namespace paio::core

#endif // PAIO_AGENT_HPP
