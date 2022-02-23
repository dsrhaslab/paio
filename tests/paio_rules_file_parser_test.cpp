/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/utils/rules_parser.hpp>

namespace paio::utils {

class RulesParserTest {

private:
    FILE* m_fd_log { stdout };

public:
    /**
     * RulesParserTest default constructor.
     */
    RulesParserTest () = default;

    /**
     * RulesParserTest parameterized destructor.
     */
    explicit RulesParserTest (FILE* fd) : m_fd_log { fd }
    { }

    /**
     * RulesParserTest default destructor.
     */
    ~RulesParserTest () = default;

    /**
     * test_read_file: Read rules from file and write them to stdout.
     * @param path Path to the rules' file.
     */
    void test_read_file (const std::string& path)
    {
        std::fprintf (this->m_fd_log, "\nTest read rules from file (%s)\n", path.c_str ());
        // create empty RulesParser object
        RulesParser parser {};
        // read rules from file in path
        parser.read_rules_from_file (path);

        // print to stdout all staged rules from file in path
        parser.print_rules (this->m_fd_log);
    }

    /**
     * test_get_create_channel_rules: Get all HousekeepingRules of type create_channel.
     * @param parser Pointer to a RulesParser object.
     * @param total_rules Total rules to get from parser.
     * @param log Boolean that defines whether it prints logging messages to stdout or not.
     */
    void
    test_get_create_channel_rules (RulesParser* parser, const int& total_rules, const bool& log)
    {
        std::fprintf (this->m_fd_log, "\nTest get create_channel rules\n");

        // create empty vector and get all create_channel rules from parser
        std::vector<HousekeepingRule> hsk_rules {};
        int result = parser->get_create_channel_rules (hsk_rules, total_rules);

        std::fprintf (this->m_fd_log, "Total rules: %d\n", result);

        // print to stdout all create_channel rules
        if (log) {
            // header message
            std::fprintf (this->m_fd_log,
                "<rule-id>, <operation_type>, <channel-id>, "
                "<enforcement-object-id>, <properties> { <workflow-id>, <operation-type>, "
                "<operation-context> }, <enforced>\n");

            for (const auto& raw_rule : hsk_rules) {
                std::fprintf (this->m_fd_log, "Rule: %s\n", raw_rule.to_string ().c_str ());
            }
        }
    }

    /**
     * test_get_create_object_rules: Get HousekeepingRules of type create_object.
     * @param parser Pointer to a RulesParser object.
     * @param total_rules Total rules to get from parser.
     * @param log Boolean that defines whether it prints logging messages to stdout or not.
     */
    void test_get_create_object_rules (RulesParser* parser, const int& total_rules, const bool& log)
    {
        std::fprintf (this->m_fd_log, "\nTest get create_object rules\n");

        // create empty vector and get all create_channel rules from parser
        std::vector<HousekeepingRule> hsk_rules {};
        int result = parser->get_create_object_rules (hsk_rules, total_rules);

        std::fprintf (this->m_fd_log, "Total rules: %d\n", result);

        // print to stdout all create_channel rules
        if (log) {
            // header message
            std::fprintf (this->m_fd_log,
                "<rule-id>, <operation_type>, <channel-id>, "
                "<enforcement-object-id>, <properties>, <enforced>\n");

            for (const auto& raw_rule : hsk_rules) {
                std::fprintf (this->m_fd_log, "Rule: %s\n", raw_rule.to_string ().c_str ());
            }
        }
    }

    /**
     * test_get_enforcement_rules: Get EnforcementRules from the RulesParser object.
     * @param parser Pointer to a RulesParser object.
     * @param total_rules Total rules to get from parser.
     * @param log Boolean that defines whether it prints logging messages to stdout or not.
     */
    void test_get_enforcement_rules (RulesParser* parser, const int& total_rules, const bool& log)
    {
        std::fprintf (this->m_fd_log, "\nTest get enforcement rules\n");

        std::vector<EnforcementRule> enf_rules {};
        int result = parser->get_enforcement_rules (enf_rules, total_rules);

        std::fprintf (this->m_fd_log, "Total rules: %d\n", result);

        // print to stdout all create_channel rules
        if (log) {
            // header message
            std::fprintf (this->m_fd_log,
                "<rule-id>, <channel-id>, <enforcement-object-id>, "
                "<operation-type>, <configurations> \n");

            for (const auto& raw_rule : enf_rules) {
                std::fprintf (this->m_fd_log, "Rule: %s\n", raw_rule.to_string ().c_str ());
            }
        }
    }

    /**
     * test_erase_rules: Remove all staged rules from RulesParser object.
     * @param parser Pointer to a RulesParser object.
     * @param log Boolean that defines whether it prints logging messages to stdout or not.
     */
    void test_erase_rules (RulesParser* parser, bool log)
    {
        std::fprintf (this->m_fd_log, "\nTest erase stages rules from parser\n");

        // erase rules from parser
        int result = parser->erase_rules ();
        std::fprintf (this->m_fd_log, "Total rules: %d\n", result);

        if (log) {
            // print to stdout all staged rules in parser
            parser->print_rules (this->m_fd_log);
        }
    }
};
} // namespace paio::utils

using namespace paio::utils;

int main (int argc, char** argv)
{
    std::string hsk_relative_path = "../files/default_housekeeping_rules_file";
    std::string enf_relative_path = "../files/default_enforcement_rules_file";

    // check argv for the file to be placed the result
    FILE* fd = stdout;
    bool log = true;

    // open file to write the logging results
    if (argc > 1) {
        fd = std::fopen (argv[1], "w");

        if (fd == nullptr) {
            fd = stdout;
        }
    }

    paio::utils::RulesParserTest test { fd };

    // HousekeepingRules parser
    RulesParser housekeeping_rules_parser { RuleType::housekeeping, hsk_relative_path };
    test.test_read_file (hsk_relative_path);

    test.test_get_create_channel_rules (&housekeeping_rules_parser, -1, log);
    test.test_get_create_object_rules (&housekeeping_rules_parser, -1, log);
    test.test_erase_rules (&housekeeping_rules_parser, log);

    std::fprintf (fd, "\n-------------------------------------\n");

    // EnforcementRules parser
    RulesParser enforcement_rules_parser { RuleType::enforcement, enf_relative_path };
    test.test_get_enforcement_rules (&enforcement_rules_parser, -1, log);

    // close file
    if (fd != stdout) {
        std::fclose (fd);
    }

    return 0;
}
