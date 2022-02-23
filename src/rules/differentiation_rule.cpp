/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/rules/differentiation_rule.hpp>

namespace paio::rules {

// DifferentiationRule default constructor.
DifferentiationRule::DifferentiationRule () = default;

// DifferentiationRule parameterized constructor.
DifferentiationRule::DifferentiationRule (const uint64_t& rule_id,
    const DifferentiationRuleType& rule_type,
    const long& channel_id,
    const long& enforcement_object_id,
    const uint32_t& workflow_id,
    const uint32_t& operation_type,
    const uint32_t& operation_context) :
    m_rule_id { rule_id },
    m_rule_type { rule_type },
    m_channel_id { channel_id },
    m_enforcement_object_id { enforcement_object_id },
    m_workflow_id { workflow_id },
    m_operation_type { operation_type },
    m_operation_context { operation_context }
{
    Logging::log_debug ("DifferentiationRule parameterized constructor.");
}

// DifferentiationRule default destructor.
DifferentiationRule::~DifferentiationRule () = default;

// get_rule_id call. Return DifferentiationRule's identifier.
uint64_t DifferentiationRule::get_rule_id () const
{
    return this->m_rule_id;
}

// get_differentiation_rule_type call. Return the differentiation rule type.
DifferentiationRuleType DifferentiationRule::get_differentiation_rule_type () const
{
    return this->m_rule_type;
}

// get_channel_id call. Return the Channel's identifier.
long DifferentiationRule::get_channel_id () const
{
    return this->m_channel_id;
}

// get_enforcement_object_id call. Return the EnforcementObject's identifier.
long DifferentiationRule::get_enforcement_object_id () const
{
    return this->m_enforcement_object_id;
}

// get_workflow_id call. Return the workflow identifier classifier to perform the differentiation.
uint32_t DifferentiationRule::get_workflow_id () const
{
    return this->m_workflow_id;
}

// get_operation_type call. Return the operation type classifier to perform the differentiation.
uint32_t DifferentiationRule::get_operation_type () const
{
    return this->m_operation_type;
}

// get_operation_context call. Return the operation context classifier to perform the
// differentiation.
uint32_t DifferentiationRule::get_operation_context () const
{
    return this->m_operation_context;
}

// operation_to_string call. Convert a DifferentiationRuleType value to string-based format.
std::string DifferentiationRule::operation_to_string () const
{
    switch (this->m_rule_type) {
        case DifferentiationRuleType::channel_differentiation:
            return "channel_differentiation";

        case DifferentiationRuleType::enforcement_object_differentiation:
            return "enforcement_object_differentiation";

        case DifferentiationRuleType::none:
        default:
            return "none";
    }
}

// to_string call. Generate a string based on the DifferentiationRule parameters.
std::string DifferentiationRule::to_string () const
{
    std::stringstream stream;

    stream << this->m_rule_id << ", ";
    stream << this->operation_to_string () << ", ";
    stream << this->m_channel_id << ", ";
    stream << this->m_enforcement_object_id << ", {";
    stream << this->m_workflow_id << ", ";
    stream << this->m_operation_type << ", ";
    stream << this->m_operation_context << "}";

    return stream.str ();
}

} // namespace paio::rules
