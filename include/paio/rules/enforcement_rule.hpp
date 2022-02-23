/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_ENFORCEMENT_RULE_HPP
#define PAIO_ENFORCEMENT_RULE_HPP

#include <paio/core/interface_definitions.hpp>
#include <paio/utils/logging.hpp>
#include <sstream>
#include <vector>

using namespace paio::core;
using namespace paio::utils;

namespace paio::rules {

/**
 * EnforcementRule class.
 * An EnforcementRule provides the means to change the original state of enforcement objects, at
 * runtime. Each EnforcementRule respects to a specific Channel and EnforcementObjects, and is given
 * by 5 parameters:
 *  - m_rule_id: Defines the EnforcementRule's identifier;
 *  - m_channel_id: Defines the Channel's identifier to apply the rule;
 *  - m_enforcement_object_id: Defines the EnforcementObject's identifier to apply the rule;
 *  - m_operation_type: Defines the type of operation to be applied over the EnforcementObject.
 *  Each object contains its own set of operations. For example, DRL provides the init, rate, and
 *  refill operations;
 *  - m_configurations: Defines the parameters to be set in the respective operation. Depending on
 *  the operation type, it can comprise none, one, or several configuration values.
 *  Currently, EnforcementRules are enforced immediately, either through an EnforcementRules
 *  configuration file, or through the SDS control plane.
 *  TODO:
 *   - create dedicated testing class for EnforcementRules (paio_enforcement_rule_test.cpp);
 *   - m_properties: configurations should be of generic type (to handle different properties);
 *   - m_properties: update (raw) copy constructor (must be more generic).
 */
class EnforcementRule {

private:
    uint64_t m_rule_id { 0 };
    long m_channel_id { -1 };
    long m_enforcement_object_id { -1 };
    int m_operation_type { 0 };
    std::vector<long> m_configurations {};

public:
    /**
     * EnforcementRule default constructor.
     */
    EnforcementRule ();

    /**
     * EnforcementRule parameterized constructor.
     * @param rule_id EnforcementRule identifier.
     * @param channel_id Channel identifier.
     * @param enforcement_object_id EnforcementObject identifier.
     * @param operation_type Enforcement operation type.
     * @param configurations Configurations of the EnforcementRule.
     */
    EnforcementRule (const uint64_t& rule_id,
        const long& channel_id,
        const long& enforcement_object_id,
        const int& operation_type,
        std::vector<long> configurations);

    /**
     * EnforcementRule copy constructor.
     * @param rule EnforcementRule object to be copied.
     */
    EnforcementRule (const EnforcementRule& rule);

    /**
     * EnforcementRule raw struct copy constructor.
     * @param raw_enforcement_rule Raw struct with enforcement elements.
     */
    explicit EnforcementRule (const EnforcementRuleRaw& raw_enforcement_rule);

    /**
     * EnforcementRule default destructor.
     */
    ~EnforcementRule ();

    /**
     * get_rule_id: Get EnforcementRule identifier.
     * @return Return a copy of the m_rule_id parameter.
     */
    [[nodiscard]] uint64_t get_rule_id () const;

    /**
     * get_channel_id: Get the Channel's identifier which the rule respects to.
     * @return Return a copy of the m_channel_id parameter.
     */
    [[nodiscard]] long get_channel_id () const;

    /**
     * get_enforcement_object_id: Get the EnforcementObject's identifier which the rule respects to.
     * @return Return a copy of the m_enforcement_object_id parameter.
     */
    [[nodiscard]] long get_enforcement_object_id () const;

    /**
     * get_operation_type: Get the type of the operation to be enforced.
     * @return Return an integer value of the operation to be enforced.
     */
    [[nodiscard]] int get_operation_type () const;

    /**
     * get_configurations: Return a all configurations to be set in the enforcement mechanism.
     * @return Return a copy of m_configurations container.
     */
    [[nodiscard]] std::vector<long> get_configurations () const;

    /**
     * get_configurations_size: Get the number of elements in the m_configurations vector.
     * @return Number of elements in the m_configurations container.
     */
    [[nodiscard]] int get_configurations_size () const;

    /**
     * configurations_begin_iterator: Get the begin const_iterator of the EnforcementRule's
     * configurations.
     * @return Returns a const_iterator to the beginning of the m_configurations container.
     */
    [[nodiscard]] std::vector<long>::const_iterator configurations_begin_iterator () const;

    /**
     * configurations_end_iterator: Get the end const_iterator of the EnforcementRule's
     * configurations.
     * @return Returns a const_iterator to the ending of the m_configurations container.
     */
    [[nodiscard]] std::vector<long>::const_iterator configurations_end_iterator () const;

    /**
     * to_string: generate a string with the EnforcementRule values.
     * @return EnforcementRule values in string-based format.
     */
    [[nodiscard]] std::string to_string () const;
};
} // namespace paio::rules

#endif // PAIO_ENFORCEMENT_RULE_HPP
