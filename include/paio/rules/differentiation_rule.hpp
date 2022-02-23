/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_DIFFERENTIATION_RULE_HPP
#define PAIO_DIFFERENTIATION_RULE_HPP

#include <paio/options/options.hpp>
#include <paio/utils/logging.hpp>
#include <string>

using namespace paio::utils;

namespace paio::rules {

/**
 * DifferentiationRuleType enum class.
 * Defines the supported types of differentiation operation, namely, if the rule respects to the
 * I/O differentiation made at the channel or enforcement object.
 *  - channel_differentiation: the rule is destined for I/O differentiation at channel level;
 *  - enforcement_object_differentiation: the rule is destined for I/O differentiation at
 *  enforcement object level;
 *  - none: the rule should be discarded.
 */
enum class DifferentiationRuleType {
    channel_differentiation = 1,
    enforcement_object_differentiation = 2,
    none = 0
};

/**
 * DifferentiationRule class.
 * A DifferentiationRule provides the means to classify and differentiate I/O requests in a PAIO
 * data plane stage. It allows users to define how requests should be differentiated, and which
 * PAIO structures (e.g., channels, enforcement objects) should handle them. Each
 * DifferentiationRule respects to the I/O differentiation of a channel or an enforcement object,
 * and comprises seven parameters.
 *  - m_rule_id: Defines the DifferentiationRule's identifier.
 *  - m_rule_type: Defines the type of I/O differentiation to be performed; it is presented as an
 *  DifferentiationRuleType, and can be of type channel_differentiation,
 *  enforcement_object_differentiation, or none;
 *  - m_channel_id: Defines the Channel's identifier to apply the rule;
 *  - m_enforcement_object_id: Defines the EnforcementObject's identifier to apply the rule;
 *  - m_workflow_id: Defines the workflow identifier classifier to perform the differentiation;
 *  - m_operation_type: Defines the operation type classifier to perform the differentiation;
 *  - m_operation_context: Defines the operation context classifier to perform the differentiation.
 * TODO:
 *  - DifferentiationRules are not being explicitly created in PAIO stages, as they are
 *  entangled with HousekeepingRules on the creation of Channels and EnforcementObjects; decouple
 *  the I/O classification and differentiation part from HousekeepingRule objects.
 */
class DifferentiationRule {

private:
    uint64_t m_rule_id { 0 };
    DifferentiationRuleType m_rule_type { DifferentiationRuleType::none };
    long m_channel_id { -1 };
    long m_enforcement_object_id { -1 };
    uint32_t m_workflow_id { 0 };
    uint32_t m_operation_type { 0 };
    uint32_t m_operation_context { 0 };

    /**
     * operation_to_string: Convert DifferentiationRuleType value to string-based format.
     * @return Returns string value of the DifferentiationRuleType.
     */
    [[nodiscard]] std::string operation_to_string () const;

public:
    /**
     * DifferentiationRule default constructor.
     */
    DifferentiationRule ();

    /**
     * DifferentiationRule parameterized constructor.
     * @param rule_id DifferentiationRule identifier.
     * @param rule_type DifferentiationRule operation type.
     * @param channel_id Channel identifier.
     * @param enforcement_object_id EnforcementObject identifier.
     * @param workflow_id Workflow identifier classifier to perform the differentiation.
     * @param operation_type Operation type classifier to perform the differentiation.
     * @param operation_context Operation context classifier to perform the differentiation.
     */
    DifferentiationRule (const uint64_t& rule_id,
        const DifferentiationRuleType& rule_type,
        const long& channel_id,
        const long& enforcement_object_id,
        const uint32_t& workflow_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context);

    /**
     * DifferentiationRule default destructor.
     */
    ~DifferentiationRule ();

    /**
     * get_rule_id: Get DifferentiationRule identifier.
     * @return Return a copy of the m_rule_id parameter.
     */
    [[nodiscard]] uint64_t get_rule_id () const;

    /**
     * get_differentiation_rule_type: Get the DifferentiationRuleType value.
     * @return Return a copy of the m_rule_type parameter.
     */
    [[nodiscard]] DifferentiationRuleType get_differentiation_rule_type () const;

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
     * get_workflow_id: Get the workflow identifier classifier to perform the I/O differentiation.
     * @return Return a copy of the m_workflow_id parameter.
     */
    [[nodiscard]] uint32_t get_workflow_id () const;

    /**
     * get_operation_type: Get the operation type classifier to perform the I/O differentiation.
     * @return Return a copy of the m_operation_type parameter.
     */
    [[nodiscard]] uint32_t get_operation_type () const;

    /**
     * get_operation_context: Get the operation context classifier to perform the I/O
     * differentiation.
     * @return Return a copy of the m_operation_context parameter.
     */
    [[nodiscard]] uint32_t get_operation_context () const;

    /**
     * to_string: generate a string with the DifferentiationRule values.
     * @return DifferentiationRule values in string-based format.
     */
    [[nodiscard]] std::string to_string () const;
};
} // namespace paio::rules

#endif // PAIO_DIFFERENTIATION_RULE_HPP
