/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_RULES_PARSER_HPP
#define PAIO_RULES_PARSER_HPP

#include <fstream>
#include <iostream>
#include <paio/options/options.hpp>
#include <paio/rules/enforcement_rule.hpp>
#include <paio/rules/housekeeping_rule.hpp>
#include <paio/utils/logging.hpp>
#include <vector>

using namespace paio::options;
using namespace paio::rules;

namespace paio::utils {

/**
 * RuleType enum class.
 * Defines the type of rules to be parsed, submitted, received, and handled.
 * Currently, it supports the following type:
 *  - housekeeping: respects to HousekeepingRules, which are used for general data plane management,
 *  including creation and configuration of Channels and EnforcementObjects;
 *  - differentiation: respects to DifferentiationRules, which are used to classify and
 *  differentiate I/O requests;
 *  - enforcement: respects to EnforcementRules, which are used to dynamically adjust the data plane
 *  state elements at execution time.
 */
enum class RuleType { housekeeping = 1, differentiation = 2, enforcement = 3, noop = 0 };

/**
 * RulesParser class.
 * This class parses existing housekeeping, differentiation, and enforcement rules files, and
 * translate them into the respective objects, namely HousekeepingRule, DifferentiationRule, and
 * EnforcementRule objects.
 * TODO:
 *  - use yaml files and parser, rather than a custom format;
 *  - update differentiation and object properties of get_create_channel_rules,
 *  get_create_object_rules, and get_enforcement_rules methods (way to hardcoded).
 */
class RulesParser {
    friend class RulesParserTest;

private:
    RuleType m_rules_type { RuleType::noop };
    std::vector<std::vector<std::string>> m_staged_rules {};
    const int m_create_channel_rules_min_elements { 7 };
    const int m_create_object_rules_min_elements { 8 };

    /**
     * read_rules_from_file: Read rules from a given file and stored them in the m_staged_rules
     * container.
     * @param path Path to the file that contains the rules.
     * @return Returns the number of rules stored.
     */
    int read_rules_from_file (const fs::path& path);

    /**
     * parse_rule: Split line (rule in string) into tokens.
     * @param rule Rule in string format.
     * @param tokens Vector to store the each token of the string-based rule.
     */
    void parse_rule (const std::string& rule, std::vector<std::string>& tokens);

    /**
     * convert_housekeeping_operation: Convert string-based operation into the respective
     * HousekeepingOperation type.
     * @param operation String-based operation.
     * @return Returns the respective HousekeepingOperation; returns no-op for unlisted operations.
     */
    [[nodiscard]] HousekeepingOperation convert_housekeeping_operation (
        const std::string& operation) const;

    /**
     * convert_object_type: Convert string-based object type into the respective
     * EnforcementObjectType classifier.
     * @param object_type String-based enforcement object type.
     * @return Returns the respective EnforcementObjectType; returns ::NOOP for unlisted types.
     */
    [[nodiscard]] EnforcementObjectType convert_object_type (const std::string& object_type) const;

    /**
     * convert_enforcement_operation: Convert string-based enforcement operations into a listed
     * integer, based on the respective EnforcementObjectType.
     * @param object_type EnforcementObjectType of the operation to be executed.
     * @param operation String-based operation type.
     * @return Returns an integer that corresponds to the respective enforcement operation
     * (configuration).
     */
    [[nodiscard]] int convert_enforcement_operation (const EnforcementObjectType& object_type,
        const std::string& operation) const;

    /**
     * convert_context_type_definition: Convert a string-based ContextType object to the
     * corresponding long value. Currently, it supports the following ContextType objects:
     * PAIO_GENERAL, POSIX, LSM_KVS_SIMPLE, LSM_KVS_DETAILED, and KVS.
     * @param context_type String-based ContextType object.
     * @return Returns the corresponding long value of the ContextType; if the object is not
     * recognized, it returns -1.
     */
    [[nodiscard]] long convert_context_type_definition (const std::string& context_type) const;

    /**
     * convert_differentiation_definitions: Convert I/O classification and differentiation
     * definitions from string-based format to the corresponding long value.
     * @param context_type String-based ContextType object, to select the correct conversion
     * method to use.
     * @param definition String-based definition for the I/O differentiation.
     * @return Returns the corresponding long value of the I/O definition.
     */
    [[nodiscard]] long convert_differentiation_definitions (const std::string& context_type,
        const std::string& definition) const;

    /**
     * convert_paio_general_definitions: Convert PAIO_GENERAL differentiation definitions from a
     * string-based format to the corresponding long value.
     * @param general_definitions String-based definition of a PAIO_GENERAL element for the I/O
     * differentiation.
     * @return Returns the corresponding long value of the I/O definition.
     */
    [[nodiscard]] long convert_paio_general_definitions (
        const std::string& general_definitions) const;

    /**
     * convert_posix_lsm_simple_definitions: Convert LSM_KVS_SIMPLE differentiation definitions
     * from a string-based format to the corresponding long value.
     * @param posix_lsm_definitions String-based definition of a LSM_KVS_SIMPLE element for the I/O
     * differentiation.
     * @return Returns the corresponding long value of the I/O definition.
     */
    [[nodiscard]] long convert_posix_lsm_simple_definitions (
        const std::string& posix_lsm_definitions) const;

    /**
     * convert_posix_lsm_detailed_definitions: Convert LSM_KVS_DETAILED differentiation definitions
     * from a string-based format to the corresponding long value.
     * @param posix_lsm_definitions String-based definition of a LSM_KVS_DETAILED element for the
     * I/O differentiation.
     * @return Returns the corresponding long value of the I/O definition.
     */
    [[nodiscard]] long convert_posix_lsm_detailed_definitions (
        const std::string& posix_lsm_definitions) const;

    /**
     * convert_posix_definitions: Convert POSIX differentiation definitions from a string-based
     * format to the corresponding long value.
     * @param posix_definitions String-based definition of a POSIX element for the I/O
     * differentiation.
     * @return Returns the corresponding long value of the I/O definition.
     */
    [[nodiscard]] long convert_posix_definitions (const std::string& posix_definitions) const;

    /**
     * convert_posix_meta_definitions: Convert POSIX_META differentiation definitions from a
     * string-based format to the corresponding long value.
     * @param posix_meta_definitions String-based definition of a POSIX_META element for the I/O
     * differentiation.
     * @return Returns the corresponding long value of the I/O definition.
     */
    [[nodiscard]] long convert_posix_meta_definitions (
        const std::string& posix_meta_definitions) const;

    /**
     * convert_kvs_definitions: Convert KVS differentiation definitions from a string-based format
     * to the corresponding long value.
     * @param kvs_definitions String-based definition of a KVS element for the I/O differentiation.
     * @return Returns the corresponding long value of the I/O definition.
     */
    [[nodiscard]] long convert_kvs_definitions (const std::string& kvs_definitions) const;

public:
    /**
     * RulesFileParser default constructor.
     */
    RulesParser ();

    /**
     * RulesFileParser parameterized constructor.
     * @param type type of rules the file comprises. Rules can be of type: HSK (Housekeeping), DIF
     * (Differentiation), and ENF (Enforcement).
     * @param path Path to the file that contains the rules.
     */
    RulesParser (RuleType type, const fs::path& path);

    /**
     * RulesFileParser default destructor.
     */
    ~RulesParser ();

    /**
     * get_rule_type: get the type of the rules in the file.
     */
    [[nodiscard]] RuleType get_rule_type () const;

    /**
     * getHousekeepingRulesCreateChannel: read raw housekeeping rules of type HSK_CREATE_CHANNEL
     * from vector and store them in hsk_rules.
     * @param hsk_rules reference to a container that stores the RAW structure.
     * @param total_rules number of rules to store in the container (-1 indicates to pass all
     * rules).
     * @return returns the number of rules stored in the container.
     */
    int get_create_channel_rules (std::vector<HousekeepingRule>& hsk_rules, int total_rules);

    /**
     * getHousekeepingRulesCreateObject: read raw housekeeping rules of type HSK_CREATE_OBJECT from
     * vector and store them in hsk_rules.
     * @param hsk_rules reference to a container that stores the RAW structure.
     * @param total_rules number of rules to store in the container (-1 indicates to pass all
     * rules).
     * @return returns the number of rules stored in the container.
     */
    int get_create_object_rules (std::vector<HousekeepingRule>& hsk_rules, int total_rules);

    /**
     * getEnforcementRules: read raw enforcement rules from staged_rules_ and store them in
     * enf_rules.
     * @param enf_rules Reference to a container that stores the RAW enforcement rules structure.
     * @param total_rules Number of rules to store in the container (-1 indicate to pass all rules).
     * @return Returns the number of rules stored in the container.
     */
    int get_enforcement_rules (std::vector<EnforcementRule>& enf_rules, int total_rules);

    /**
     * erase_rules: Remove all rules stored in the m_staged_rules container.
     * @return Returns the number of removed rules from the container.
     */
    int erase_rules ();

    /**
     * print_rules: Write to stdout the rules stored in the m_staged_rules container.
     * @param fd: File descriptor to write to.
     */
    void print_rules (FILE* fd) const;
};
} // namespace paio::utils

#endif // PAIO_RULES_PARSER_HPP
