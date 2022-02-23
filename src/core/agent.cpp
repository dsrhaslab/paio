/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/core/agent.hpp>

namespace paio::core {

// Agent default constructor.
Agent::Agent () :
    m_core { std::make_shared<Core> () },
    m_ready { std::make_shared<std::atomic<bool>> (false) },
    m_stage_identifier { std::make_shared<StageInfo> ("stage") }
{
    // create log message
    std::string message { "Agent default constructor (" };
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_identifier.use_count ())).append (", ");
    message.append ((this->m_execute_on_receive ? "true" : "false")).append (").");
    Logging::log_debug (message);

    // validate if the communication is controller-less and insert default rules from files
    if (option_default_communication_type == CommunicationType::none) {
        // insert housekeeping rules
        PStatus status
            = this->insert_housekeeping_rules_from_file (this->m_housekeeping_rule_file, -1);

        // validate if the parsing and insertion of HousekeepingRules was successful
        if (status.is_ok ()) {
            // insert differentiation rules
            auto diff_rules = this->insert_differentiation_rules_from_file (
                this->m_differentiation_rule_file.string (),
                -1);
            Logging::log_debug (
                "Applied " + std::to_string (diff_rules) + " differentiation rules.");

            // insert enforcement rules
            auto enf_rules
                = this->insert_enforcement_rules_from_file (this->m_enforcement_rule_file.string (),
                    -1);
            Logging::log_debug ("Applied " + std::to_string (enf_rules) + " enforcement rules.");

            // mark data plane stage ready to receive requests
            this->mark_ready ();
        } else {
            Logging::log_error (
                "Error while inserting HousekeepingRules from file (" + status.to_string () + ").");
        }
    }
}

// Agent parameterized constructor.
Agent::Agent (const CommunicationType& communication_type,
    std::shared_ptr<Core> core,
    std::shared_ptr<std::atomic<bool>> ready,
    const int& instances,
    std::shared_ptr<StageInfo> stage_identifier) :
    m_core { std::move (core) },
    m_ready { std::move (ready) },
    m_stage_identifier { std::move (stage_identifier) }
{
    // create log message
    std::string message { "Agent parameterized constructor (" };
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_identifier.use_count ())).append (", ");
    message.append ((this->m_execute_on_receive ? "true" : "false")).append (").");
    Logging::log_debug (message);

    // validate if the communication is controller-less and insert HousekeepingRules from file
    if (communication_type == CommunicationType::none) {
        // insert housekeeping rules
        PStatus status
            = this->insert_housekeeping_rules_from_file (this->m_housekeeping_rule_file, instances);

        // validate if the parsing and insertion of HousekeepingRules was successful
        if (status.is_ok ()) {
            // insert differentiation rules
            auto diff_rules = this->insert_differentiation_rules_from_file (
                this->m_differentiation_rule_file.string (),
                -1);
            Logging::log_debug (
                "Applied " + std::to_string (diff_rules) + " differentiation rules.");

            // insert enforcement rules
            auto enf_rules
                = this->insert_enforcement_rules_from_file (this->m_enforcement_rule_file.string (),
                    -1);
            Logging::log_debug ("Applied " + std::to_string (enf_rules) + " enforcement rules.");

            // mark data plane stage ready to receive requests
            this->mark_ready ();
        } else {
            Logging::log_error (
                "Error while inserting HousekeepingRules from file (" + status.to_string () + ").");
        }
    }
}

// Agent (fully) parameterized constructor.
Agent::Agent (const CommunicationType& communication_type,
    std::shared_ptr<Core> core,
    std::shared_ptr<std::atomic<bool>> ready,
    const fs::path& housekeeping_rules_file_path,
    const fs::path& differentiation_rules_file_path,
    const fs::path& enforcement_rules_file_path,
    const int& instances,
    std::shared_ptr<StageInfo> stage_identifier,
    const bool& execute_on_receive) :
    m_core { std::move (core) },
    m_ready { std::move (ready) },
    m_stage_identifier { std::move (stage_identifier) },
    m_housekeeping_rule_file { housekeeping_rules_file_path },
    m_differentiation_rule_file { differentiation_rules_file_path },
    m_enforcement_rule_file { enforcement_rules_file_path },
    m_execute_on_receive { execute_on_receive }
{
    // create log message
    std::string message { "Agent parameterized (full) constructor (" };
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_identifier.use_count ())).append (", ");
    message.append ((this->m_execute_on_receive ? "true" : "false")).append (").");
    Logging::log_debug (message);

    // validate if the communication is controller-less and insert HousekeepingRules from file
    if (communication_type == CommunicationType::none) {
        // insert housekeeping rules
        PStatus status
            = this->insert_housekeeping_rules_from_file (this->m_housekeeping_rule_file, instances);

        // validate if the parsing and insertion of HousekeepingRules was successful
        if (status.is_ok ()) {
            // insert differentiation rules
            auto diff_rules
                = this->insert_differentiation_rules_from_file (this->m_differentiation_rule_file,
                    -1);
            Logging::log_debug (
                "Applied " + std::to_string (diff_rules) + " differentiation rules.");

            // insert enforcement rules
            auto enf_rules
                = this->insert_enforcement_rules_from_file (this->m_enforcement_rule_file, -1);
            Logging::log_debug ("Applied " + std::to_string (enf_rules) + " enforcement rules.");

            // mark data plane stage ready to receive requests
            this->mark_ready ();
        } else {
            Logging::log_error (
                "Error while inserting HousekeepingRules from file (" + status.to_string () + ").");
        }
    }
}

// Agent default destructor.
Agent::~Agent ()
{
    // create log message
    std::string message { "Agent default destructor (" };
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_identifier.use_count ())).append (", ");
    message.append ((this->m_execute_on_receive ? "true" : "false")).append (").");
    Logging::log_debug_explicit (message);
}

// mark_ready call. Mark the data plane stage ready to receive I/O requests from the I/O stack.
void Agent::mark_ready ()
{
    this->m_ready->store (true);
    Logging::log_debug ("Agent: marked data plane stage as ready ...");
}

// mark_data_plane_stage_ready call. Mark the data plane stage ready to receive I/O requests from
// the I/O stack.
void Agent::mark_data_plane_stage_ready ()
{
    this->mark_ready ();
}

// employ_housekeeping_rule call. Employ HousekeepingRule in the data plane stage.
PStatus Agent::employ_housekeeping_rule (const HousekeepingRule& rule)
{
    PStatus status = PStatus::Error ();

    // only consider creation of Channels and EnforcementObjects
    switch (rule.get_housekeeping_operation_type ()) {
        case HousekeepingOperation::create_channel:
        case HousekeepingOperation::create_object:
            Logging::log_debug (rule.to_string ());

            // submit HousekeepingRule to core (to be inserted in the HousekeepingTable)
            status = this->m_core->insert_housekeeping_rule (rule);
            break;

        default:
            throw std::logic_error { "Agent: unexpected HousekeepingRule type" };
    }

    // verify if rule was correctly inserted; if so, execute rule.
    if (status.is_ok ()) {
        // log message with the HousekeepingRule's creation
        std::string insert_message { "Inserted HousekeepingRule {" };
        insert_message.append (std::to_string (rule.get_rule_id ())).append (" - ");
        insert_message.append (rule.operation_to_string ()).append (" : ");
        insert_message.append (status.to_string ()).append ("}");
        Logging::log_debug (insert_message);

        // validate if HousekeepingRule is to be executed on receive
        if (this->m_execute_on_receive.load ()) {
            // execute HousekeepingRule
            status = m_core->execute_housekeeping_rule (rule.get_rule_id ());

            // log message with the execution result
            std::string execute_message { "Execute housekeeping rule (" };
            execute_message.append (std::to_string (rule.get_rule_id ())).append ("): ");
            execute_message.append (status.to_string ());
            Logging::log_debug (execute_message);
        }
    }

    return status;
}

// execute_housekeeping_rules call. Execute all HousekeepingRules that are left to execute.
PStatus Agent::execute_housekeeping_rules ()
{
    // submit execute_housekeeping_rules to Core
    PStatus status = m_core->execute_housekeeping_rules ();

    // return Error if the execution was not made
    if (!status.is_ok ()) {
        Logging::log_error ("Error while executing all housekeeping rules.");
        return PStatus::Error ();
    }

    return status;
}

// employ_differentiation_rule call. Employ DifferentiationRule in the data plane stage.
// PStatus Agent::employ_differentiation_rule (const DifferentiationRuleRaw& raw_diff_rule)
// {
// validate if debug is enabled and log message to the Logging framework
// if (Logging::is_debug_enabled ()) {
//     std::string message { "CreateAndInsert::DifferentiationRule {" };
//     message.append (std::to_string (raw_diff_rule.m_rule_id)).append (", ");
//     message.append (std::to_string (raw_diff_rule.m_channel_id)).append (", ");
//     message.append (std::to_string (raw_diff_rule.m_enforcement_object_id)).append (", ");
//     message.append (std::to_string (raw_diff_rule.m_operation_type)).append (", ");
//     message.append (std::to_string (raw_diff_rule.m_workflow_id)).append (", ");
//     message.append (std::to_string (raw_diff_rule.m_operation_type)).append (", ");
//     message.append (std::to_string (raw_diff_rule.m_operation_context)).append ("}");
//     Logging::log_debug (message);
// }

// submit rule to PAIO Core
// PStatus status = this->m_core.insert_differentiation_rule ();

// if (status.is_not_supported ()) {
//     Logging::log_error (status.to_string () + ": DifferentiationTable of EnforcementUnit "
//         + std::to_string (raw_diff_rule.enforcement_unit_id)
//         + "is disabled. Cannot insert rule.");
// }

// return status;
// }

// employ_enforcement_rule call. Employ EnforcementRule in the data plane stage.
PStatus Agent::employ_enforcement_rule (const EnforcementRule& enforcement_rule)
{
    PStatus status;

    // submit enforcement rule to Core
    status = this->m_core->employ_enforcement_rule (enforcement_rule.get_channel_id (),
        enforcement_rule.get_enforcement_object_id (),
        enforcement_rule.get_operation_type (),
        enforcement_rule.get_configurations ());

    // log message with the EnforcementRule's creation
    std::string message { "Employ enforcement rule {" };
    message.append (status.to_string ()).append (" : ");
    message.append (enforcement_rule.to_string ()).append ("}");
    Logging::log_debug (message);

    if (!status.is_ok ()) {
        Logging::log_error (status.to_string () + ": EnforcementRule not supported.");
    }

    return status;
}

// get_stage_identifier call. Get the identifier of the data plane stage.
std::string Agent::get_stage_name () const
{
    // return a string value pointed by m_stage_identifier.
    return this->m_stage_identifier->get_name ();
}

// insert_housekeeping_rules_from_file call. Insert housekeeping rules from selected file.
PStatus Agent::insert_housekeeping_rules_from_file (const fs::path& path, const int& total_rules)
{
    PStatus status = PStatus::Error ();

    // validate if file path is not empty
    if (path.empty ()) {
        Logging::log_error (
            "Error while inserting HousekeepingRules from file (file path is not valid).");
        return status;
    }

    // initialize rule parser
    RulesParser file_parser { RuleType::housekeeping, path };

    // get HousekeepingRules of type 'create_channel' from RuleParser object
    std::vector<HousekeepingRule> create_channel_rules {};
    int rules_size = file_parser.get_create_channel_rules (create_channel_rules, total_rules);

    // enforce HousekeepingRules of type 'create_channel'
    for (int i = 0; i < rules_size; i++) {
        status = this->employ_housekeeping_rule (create_channel_rules[i]);

        if (status.is_ok ()) {
            Logging::log_debug ("Inserted HousekeepingRule of type 'create-channel' from file ...");
        } else {
            Logging::log_debug (
                "Error while employing HousekeepingRule of type 'create-channel' ...");
        }
    }

    // get HousekeepingRules of type 'create_object' from RuleParser object
    std::vector<HousekeepingRule> create_object_rules {};
    rules_size = file_parser.get_create_object_rules (create_object_rules, total_rules);

    // enforce HousekeepingRules of type 'create_object'
    for (int i = 0; i < rules_size; i++) {
        status = this->employ_housekeeping_rule (create_object_rules[i]);

        if (status.is_ok ()) {
            Logging::log_debug ("Insert HousekeepingRule of type 'create-object' from file ...");
        } else {
            Logging::log_debug (
                "Error while employing HousekeepingRule of type 'create-object' ...");
        }
    }

    return status;
}

// insert_differentiation_rules_from_file call. Insert differentiation rules from selected file.
int Agent::insert_differentiation_rules_from_file ([[maybe_unused]] const fs::path& path,
    [[maybe_unused]] const int& total_rules)
{
    Logging::log_error ("Error while inserting differentiation rules from file (method "
                        "not implemented).");
    return -1;
}

// insert_enforcement_rules_from_file call. Insert enforcement rules from selected file.
int Agent::insert_enforcement_rules_from_file (const fs::path& path, const int& total_rules)
{
    int rules_enforced = 0;

    // validate if file path is not empty
    if (path.empty ()) {
        Logging::log_error ("Error while inserting EnforcementRules from file (file path "
                            "is not valid).");
        return -1;
    }

    // initialize rule parser
    RulesParser file_parser { RuleType::enforcement, path };

    // get EnforcementRules from RuleParser object
    std::vector<EnforcementRule> enforcement_rules {};
    int rules_size = file_parser.get_enforcement_rules (enforcement_rules, total_rules);

    // employ EnforcementRules
    for (int i = 0; i < rules_size; i++) {
        PStatus status = this->employ_enforcement_rule (enforcement_rules[i]);
        if (status.is_ok ()) {
            Logging::log_debug ("Applied EnforcementRule from file ...");
            rules_enforced++;
        } else {
            Logging::log_error ("Error while employing EnforcementRule from file ...");
        }
    }

    return rules_enforced;
}

// print_housekeeping_rules_in_core call. List all HousekeepingRules stored in the Core's
// HousekeepingTable.
std::string Agent::print_housekeeping_rules_in_core ()
{
    return this->m_core->list_housekeeping_table_rules ();
}

// collect_channel_statistics call. Collect statistics from one or more Channels of the data plane.
PStatus Agent::collect_channel_statistics (const long& channel_id,
    std::vector<ChannelStatsRaw>& channel_stats)
{
    PStatus status = PStatus::Error ();

    // log debug message
    Logging::log_debug ("Collecting channel statistics (" + std::to_string (channel_id) + ")");

    // collect statistics from a single Channel (channel_id)
    if (channel_id != -1) {
        // create temporary ChannelStatsRaw object
        // all elements will be filled by the m_core->collect_channel_statistics call
        ChannelStatsRaw channel_stats_obj;
        // collect Channel's statistics
        status = this->m_core->collect_channel_statistics (channel_id, channel_stats_obj);

        // if channel statistic collection was successful, push object to channel_stats container
        if (status.is_ok ()) {
            channel_stats.push_back (channel_stats_obj);
        } else {
            Logging::log_error ("Error while collecting general statistics from channel "
                + std::to_string (channel_id));
        }
    } else { // collect statistics from all Channels in the data plane stage
        // get total amount of channels int the data plane stage
        int total_channels = this->m_core->get_total_channels ();

        // validate if there are channels to collect statistics
        if (total_channels > 0) {
            std::vector<long> channel_ids {};
            // get Channel's identifiers
            this->m_core->get_channels_identifiers (channel_ids);

            // note: is this really necessary?
            // allocate size in the channel_stats container
            channel_stats.reserve (total_channels);

            // collect general statistics of all Channels
            for (int i = 0; i < total_channels; i++) {
                // create temporary ChannelStatsRaw object
                // all elements will be filled by the m_core->collect_channel_statistics call
                ChannelStatsRaw stats_raw {};
                // collect general channel statistics
                status = this->m_core->collect_channel_statistics (channel_ids[i], stats_raw);

                // if statistic collection was successful, push object to channel_stats container
                if (status.is_ok ()) {
                    channel_stats.push_back (stats_raw);
                } else {
                    Logging::log_error ("Error while collecting general statistics from channel "
                        + std::to_string (channel_ids[i]));
                    break;
                }
            }
        } else {
            Logging::log_error (
                "Error while collecting general statistics: no channels in the data plane stage");
        }
    }

    return status;
}

// collect_detailed_channel_statistics call. Collect detailed statistics from one or more channels.
PStatus Agent::collect_detailed_channel_statistics (const long& channel_id,
    std::map<long, std::vector<double>>& detailed_channel_stats)
{
    PStatus status = PStatus::Error ();

    // log debug message
    Logging::log_debug (
        "Collecting detailed channel statistics (" + std::to_string (channel_id) + ")");

    // collect detailed statistics from a single Channel (channel_id)
    if (channel_id != -1) {
        // create temporary container to store the detailed channel statistics
        std::vector<double> detailed_stats {};
        // collect detailed channel statistics
        status = this->m_core->collect_channel_statistics_detailed (channel_id, detailed_stats);

        // if channel statistic collection was successful; push object to channel_stats container
        if (status.is_ok ()) {
            detailed_channel_stats.emplace (channel_id, std::move (detailed_stats));
        } else {
            Logging::log_error ("Error while collecting detailed statistics from channel "
                + std::to_string (channel_id));
        }

    } else { // collect detailed statistics from all channels in the data plane stage
        // get total amount of channels int the data plane stage to collect statistics
        int total_channels = this->m_core->get_total_channels ();

        // validate if there are channels to collect statistics
        if (total_channels > 0) {
            std::vector<long> channel_ids {};
            // get channel identifiers
            this->m_core->get_channels_identifiers (channel_ids);

            for (int i = 0; i < total_channels; i++) {
                // create temporary container to store detailed statistics of a single channel
                std::vector<double> single_channel_statistics {};

                // collect detailed channel statistics detailed
                status = this->m_core->collect_channel_statistics_detailed (channel_ids[i],
                    single_channel_statistics);

                // if statistic collection was successful, emplace individual container in map
                // structure
                if (status.is_ok ()) {
                    detailed_channel_stats.emplace (channel_ids[i],
                        std::move (single_channel_statistics));
                } else {
                    Logging::log_error ("Error while collecting detailed statistics from channel "
                        + std::to_string (channel_ids[i]));
                    break;
                }
            }
        } else {
            Logging::log_error (
                "Error while collecting detailed statistics: no channels in the data plane stage");
        }
    }

    return status;
}

// collect_enforcement_object_statistics call. Collect statistics from a set of EnforcementObjects.
PStatus Agent::collect_enforcement_object_statistics (
    std::map<std::pair<long, long>, ObjectStatisticsRaw>& object_stats_raw)
{
    PStatus status = PStatus::Error ();

    // traverse map and collect
    for (auto& entry : object_stats_raw) {
        // get channel-id and enforcement object-id pair
        const auto channel_object_pair = entry.first;

        // log debug message
        std::string collect_message { "Collecting enforcement object statistics (" };
        collect_message.append (std::to_string (channel_object_pair.first)).append (",");
        collect_message.append (std::to_string (channel_object_pair.second)).append (")");
        Logging::log_debug (collect_message);

        // fixme: validate if ObjectStatisticsRaw needs to be created before this operation
        // collect enforcement object statistics for the current pair
        status = this->m_core->collect_enforcement_object_statistics (channel_object_pair.first,
            channel_object_pair.second,
            entry.second);

        // validate if enforcement object statistic collection was successful
        if (status.is_error ()) {
            // log error message
            std::string error_message {
                "Error while collecting enforcement statistics from channel {"
            };
            error_message.append (std::to_string (channel_object_pair.first))
                .append ("} and enforcement object {");
            error_message.append (std::to_string (channel_object_pair.second)).append ("}");
            Logging::log_error (error_message);
            break;
        }
    }

    return status;
}

// get_stage_info call. Fill StageInfoRaw object with StageInfo values.
void Agent::get_stage_info (StageInfoRaw& handshake_obj)
{
    this->m_stage_identifier->serialize (handshake_obj);
}

// set_execute_on_receive call. Defines if HousekeepingRules should be executed on receive.
void Agent::set_execute_on_receive (const bool& value)
{
    this->m_execute_on_receive.store (value);
}

} // namespace paio::core
