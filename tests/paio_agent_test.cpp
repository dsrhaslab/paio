/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/stage/paio_stage.hpp>

namespace paio::core {

/**
 * AgentTest class.
 * Functional and performance tests related to the Agent class, including creation and insertion of
 * rules, statistic collection, and setting stage info variables.
 * TODO:
 *  - adjust create_and_insert_enforcement_rule_test method to be more flexible on the generation of
 *  configurations;
 *  - create method for executing pending rules (execute_pending_rules_test);
 *  - complete and adjust the collect_general_channel_statistics_test,
 *  collect_object_statistics_test, and collect_detailed_channel_statistics_test methods.
 */
class AgentTest {

private:
    FILE* m_fd { stdout };
    int m_max_rule_id { 1000 };
    int m_max_channel_id { 20 };
    int m_max_enf_object_id { 10 };

public:
    /**
     * AgentTest default constructor.
     */
    AgentTest () = default;

    /**
     * AgentTest (explicit) parameterized constructor.
     * @param fd Pointer to FILE to where the log messages should be written to.
     */
    explicit AgentTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * AgentTest parameterized constructor.
     * @param fd Pointer to FILE to where the log messages should be written to.
     * @param max_rule_id Maximum value for rule identifiers.
     * @param max_channel_id Maximum value for Channel identifiers.
     * @param max_enf_object_id Maximum value for EnforcementObject identifiers.
     */
    AgentTest (FILE* fd,
        const int& max_rule_id,
        const int& max_channel_id,
        const int& max_enf_object_id) :
        m_fd { fd },
        m_max_rule_id { max_rule_id },
        m_max_channel_id { max_channel_id },
        m_max_enf_object_id { max_enf_object_id }
    { }

    /**
     * AgentTest default destructor.
     */
    ~AgentTest () = default;

    /**
     * create_and_insert_housekeeping_rule_test: generate random HousekeepingRules and insert/apply
     * them in the Agent object.
     * @param agent Pointer to the Agent object.
     * @param total_rules Number of HousekeepingRules to generate.
     * @param context_type Type of the context to be used in the HousekeepingRule.
     * @param operation_bound Maximum value for the operation bound (of the given Context type).
     * @param debug Bool that defines if the detailed logging is enabled.
     */
    void create_and_insert_housekeeping_rule_test (Agent* agent,
        const int& total_rules,
        const ContextType& context_type,
        const int& operation_bound,
        const bool& log)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Test create and insert HousekeepingRules\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        for (int i = 0; i < total_rules; i++) {
            uint64_t rule_id = random () % m_max_rule_id;
            long operation_tag = random ();
            long channel_id = random () % m_max_channel_id;

            switch (operation_tag % 2) {
                case 0: { // 'create-channel' HousekeepingRule
                    auto workflow_id = static_cast<uint32_t> (random () % m_max_channel_id);
                    auto operation_type = static_cast<uint32_t> (random () % operation_bound);
                    auto operation_context = static_cast<uint32_t> (random () % operation_bound);

                    // create channel differentiation properties
                    std::vector<long> channel_properties { static_cast<long> (context_type),
                        workflow_id,
                        operation_type,
                        operation_context };

                    // create HousekeepingRule object
                    HousekeepingRule hsk_rule { rule_id,
                        HousekeepingOperation::create_channel,
                        channel_id,
                        -1,
                        channel_properties };

                    // create log message
                    std::string message { "Housekeeping rule: " };
                    message.append (hsk_rule.to_string ()).append ("\n");

                    // employ HousekeepingRule
                    PStatus status = agent->employ_housekeeping_rule (hsk_rule);
                    message.append ("PStatus { ").append (status.to_string ()).append (" }\n");

                    // print log message
                    if (log) {
                        std::fprintf (this->m_fd, "%s", message.c_str ());
                    }

                    break;
                }
                case 1: { // 'create-object' HousekeepingRule
                    auto enf_object_id = static_cast<long> (random () % m_max_enf_object_id);
                    auto operation_type = static_cast<uint32_t> (random () % operation_bound);
                    auto operation_context = static_cast<uint32_t> (random () % operation_bound);

                    // create DRL object (static)
                    auto enf_object_type = static_cast<long> (EnforcementObjectType::DRL);
                    long property_first = random () % 1000000; // refill period
                    long property_second = random () % 1000000; // rate

                    // create differentiation and object properties
                    std::vector<long> enf_obj_properties { static_cast<long> (context_type),
                        operation_type,
                        operation_context,
                        enf_object_type,
                        property_first,
                        property_second };

                    // create HousekeepingRule object
                    HousekeepingRule hsk_rule { rule_id,
                        HousekeepingOperation::create_object,
                        channel_id,
                        enf_object_id,
                        enf_obj_properties };

                    // create log message
                    std::string message { "Housekeeping rule: " };
                    message.append (hsk_rule.to_string ()).append ("\n");

                    // employ HousekeepingRule
                    PStatus status = agent->employ_housekeeping_rule (hsk_rule);
                    message.append ("PStatus { ").append (status.to_string ()).append (" }\n");

                    // print log message
                    if (log) {
                        std::fprintf (this->m_fd, "%s", message.c_str ());
                    }
                    break;
                }
            }
        }

        // if debug is enabled, print all HousekeepingRules stored in the data plane stage
        if (log) {
            std::fprintf (this->m_fd, "Housekeeping table listing:\n");
            std::fprintf (this->m_fd, "%s\n", agent->print_housekeeping_rules_in_core ().c_str ());
        }
        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * insert_housekeeping_rules_from_file_test: Insert HousekeepingRules from a given file.
     * @param agent Pointer to an Agent object.
     * @param pathname Path to the HousekeepingRules file to be used.
     * @param num_rules Total of rules to be employed.
     * @param log Boolean that defines if logging is enabled.
     */
    void insert_housekeeping_rules_from_file_test (Agent* agent,
        const fs::path& pathname,
        const int& num_rules,
        const bool& log)
    {
        // print header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd,
            "Test insert HousekeepingRules from file (%s)\n",
            (!pathname.empty () ? pathname.string ().c_str () : "<empty>"));
        std::fprintf (this->m_fd, "----------------------------\n");

        // insert housekeeping rules from file
        auto status = agent->insert_housekeeping_rules_from_file (pathname, num_rules);

        // print status log message
        std::fprintf (this->m_fd, "Status: %s\n", status.to_string ().c_str ());

        // if debug is enabled, print all HousekeepingRules stored in the data plane stage
        if (log) {
            std::fprintf (this->m_fd, "%s\n", agent->print_housekeeping_rules_in_core ().c_str ());
        }

        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * create_and_insert_enforcement_rule_test: Generate random enforcement rules and insert/apply
     * them in the Agent object.
     * @param agent Pointer to an Agent object.
     * @param iterations Number of enforcement rules to generate and employ.
     * @param log Boolean that defines if detailed logging is enabled.
     */
    void create_and_insert_enforcement_rule_test (Agent* agent, const int& iterations, bool log)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Test create and insert EnforcementRules\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        for (int i = 0; i < iterations; i++) {
            long channel_id = random () % m_max_channel_id;
            long enf_object_id = random () % m_max_enf_object_id;
            int operation_type = static_cast<int> (random () % 3);
            std::vector<long> configurations {};

            // generate random configurations
            switch (operation_type) {
                case 1:
                    configurations.push_back (random () % 10000000);
                    configurations.push_back (random () % 10000000);
                    break;

                case 2:
                    configurations.push_back (random () % 10000000);
                    break;

                default:
                    break;
            }

            // create EnforcementRule object
            EnforcementRule enf_rule {
                static_cast<uint64_t> (i), // rule-id
                channel_id, // channel-id
                enf_object_id, // enforcement-object-id
                operation_type, // operation-type
                configurations // configurations
            };

            std::string message { "Enforcement rule: " };
            message.append (enf_rule.to_string ()).append ("\n");

            // employ EnforcementRule
            PStatus status = agent->employ_enforcement_rule (enf_rule);
            message.append ("PStatus { ").append (status.to_string ()).append (" }\n");

            // print log message
            if (log) {
                std::fprintf (this->m_fd, "%s", message.c_str ());
            }
            std::fprintf (this->m_fd, "----------------------------\n\n");
        }
    }

    /**
     * insert_enforcement_rules_from_file_test: Insert/apply EnforcementRules from a given file.
     * @param agent Pointer to an Agent object.
     * @param pathname Name of the EnforcementRules file to be inserted.
     * @param total_rules Total of rules to be employed.
     */
    void insert_enforcement_rules_from_file_test (Agent* agent,
        const fs::path& pathname,
        const int& total_rules)
    {
        // print header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd,
            "Test insert EnforcementRules from file (%s)\n",
            (!pathname.empty () ? pathname.string ().c_str () : "<empty>"));
        std::fprintf (this->m_fd, "----------------------------\n");

        // insert enforcement rules from file
        auto return_value = agent->insert_enforcement_rules_from_file (pathname, total_rules);

        std::fprintf (this->m_fd, "Number of rules enforced: %d\n", return_value);
        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * collect_general_channel_statistics_test: Collect general channel statistics from all
     * channels registered in the data plane stage.
     * @param agent Pointer to an Agent object.
     * @param channel_id Identifier of the channel to collect the general statistics. If this value
     * is set to -1, the statistics of all channels will be collected.
     * @param log Boolean that defines if detailed logging is enabled.
     */
    [[maybe_unused]] void
    collect_general_channel_statistics_test (Agent* agent, const long& channel_id, const bool& log)
    {
        // print header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Test collect general channel statistics (%ld)\n", channel_id);
        std::fprintf (this->m_fd, "----------------------------\n");

        // collect general channel statistics
        std::vector<ChannelStatsRaw> channel_stats {};
        PStatus status = agent->collect_channel_statistics (channel_id, channel_stats);

        // log message
        std::fprintf (this->m_fd, "Status: %s\n", status.to_string ().c_str ());

        if (log) {
            std::stringstream stream;
            for (auto& entry : channel_stats) {
                stream << "[" << entry.m_channel_id;
                stream << ":" << entry.m_overall_metric_value;
                stream << "," << entry.m_windowed_metric_value;
                stream << "]\n";
            }
            Logging::log_debug (stream.str ());
        }
        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * collect_object_statistics_test: Collect object statistics from all objects registered in
     * channel_object_pairs.
     * @param agent Pointer to an Agent object.
     * @param channel_object_pairs Vector of channel-object pairs to collect the statistics.
     * @param log Boolean that defines if detailed logging is enabled.
     */
    [[maybe_unused]] void collect_object_statistics_test (Agent* agent,
        const std::vector<std::pair<long, long>>& object_pairs,
        const bool& log)
    {
        // print header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Test collect object statistics (%ld)\n", object_pairs.size ());
        std::fprintf (this->m_fd, "----------------------------\n");

        // initialize object-stats container
        std::map<std::pair<long, long>, ObjectStatisticsRaw> object_stats {};
        for (auto& entry : object_pairs) {
            ObjectStatisticsRaw object {};
            object_stats.emplace (std::pair<long, long> (entry.first, entry.second), object);
        }

        // collect object statistics
        PStatus status = agent->collect_enforcement_object_statistics (object_stats);

        // log message
        if (status.is_ok () && log) {
            std::string message;
            for (auto& entry : object_stats) {
                message.append ("[").append (std::to_string (entry.first.first)).append ("-");
                message.append (std::to_string (entry.first.second)).append (" : ");
                message.append (std::to_string (entry.second.m_channel_id)).append (", ");
                message.append (std::to_string (entry.second.m_enforcement_object_id))
                    .append (", ");
                message.append (std::to_string (entry.second.m_total_stats)).append ("]\n");
                std::fprintf (this->m_fd, "%s", message.c_str ());
            }
        }
    }
};
} // namespace paio::core

// set_env call. Set environment variable with name 'env_name' and value 'env_value'.
bool set_env (const std::string& env_name, const std::string& env_value)
{
    auto return_value = ::setenv (env_name.c_str (), env_value.c_str (), 1);
    return (return_value == 0);
}

// unset_env call. Remove environment variable with name 'env_name'.
bool unset_env (const std::string& env_name)
{
    auto return_value = ::unsetenv (env_name.c_str ());
    return (return_value == 0);
}

using namespace paio::core;

int main (int argc, char** argv)
{
    // check argv for the file to be placed the result
    FILE* fd = stdout;

    // open file to write the logging results
    if (argc > 1) {
        fd = ::fopen (argv[1], "w");

        if (fd == nullptr) {
            fd = stdout;
        }
    }

    // get PAIO environment variable and define its value
    std::string env_name { paio::options::option_environment_variable_env () };
    std::string env_value { "tmp" };

    // set PAIO environment variable
    auto return_value = set_env (env_name, env_value);

    std::shared_ptr<StageInfo> stage_info = std::make_shared<StageInfo> ("testing-stage");
    int instances = -1;
    bool execute_on_receive = true;
    int define_max_rule_id = 100;
    int define_max_channel_id = 10;
    int define_max_enforcement_object_id = 10;

    AgentTest test { fd,
        define_max_rule_id,
        define_max_channel_id,
        define_max_enforcement_object_id };
    Agent agent { CommunicationType::none,
        std::make_shared<Core> (),
        std::make_shared<std::atomic<bool>> (false),
        {},
        {},
        {},
        instances,
        stage_info,
        execute_on_receive };

    bool detailed_log = true;
    bool create_rules_from_file = true;

    // perform the file-based rules tests
    if (create_rules_from_file) {
        // define path to the file with housekeeping rules
        fs::path hsk_path { main_path ().string () + "default_housekeeping_rules_file" };
        // test insert HousekeepingRules from file
        test.insert_housekeeping_rules_from_file_test (&agent, hsk_path, instances, detailed_log);

        // define path to the file with enforcement rules
        fs::path enf_path { main_path ().string () + "default_enforcement_rules_file" };
        // test insert EnforcementRules from file
        test.insert_enforcement_rules_from_file_test (&agent, enf_path, -1);
    } else {
        // test create and insert random HousekeepingRules
        test.create_and_insert_housekeeping_rule_test (&agent,
            10,
            ContextType::PAIO_GENERAL,
            paio_general_size,
            detailed_log);

        // test create and insert random EnforcementRules
        test.create_and_insert_enforcement_rule_test (&agent, 10, detailed_log);
    }

    // unset environment variable
    if (return_value) {
        unset_env (env_name);
    }
}
