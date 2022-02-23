/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/rules/enforcement_rule.hpp>

namespace paio::rules {

// EnforcementRule default constructor.
EnforcementRule::EnforcementRule ()
{
    Logging::log_debug ("EnforcementRule default constructor.");
}

// EnforcementRule parameterized constructor.
EnforcementRule::EnforcementRule (const uint64_t& rule_id,
    const long& channel_id,
    const long& enforcement_object_id,
    const int& operation_type,
    std::vector<long> configurations) :
    m_rule_id { rule_id },
    m_channel_id { channel_id },
    m_enforcement_object_id { enforcement_object_id },
    m_operation_type { operation_type },
    m_configurations { std::move (configurations) }
{
    Logging::log_debug ("EnforcementRule parameterized constructor.");
}

// EnforcementRule copy constructor.
EnforcementRule::EnforcementRule (const EnforcementRule& rule) :
    m_rule_id { rule.m_rule_id },
    m_channel_id { rule.m_channel_id },
    m_enforcement_object_id { rule.m_enforcement_object_id },
    m_operation_type { rule.m_operation_type },
    m_configurations { rule.m_configurations }
{
    Logging::log_debug ("EnforcementRule copy constructor.");
}

// EnforcementRule raw struct copy constructor.
EnforcementRule::EnforcementRule (const EnforcementRuleRaw& raw_rule) :
    m_rule_id { static_cast<uint64_t> (raw_rule.m_rule_id) },
    m_channel_id { raw_rule.m_channel_id },
    m_enforcement_object_id { raw_rule.m_enforcement_object_id },
    m_operation_type { raw_rule.m_enforcement_operation }
{
    // fixme
    // add configurations
    if (raw_rule.m_property_first != -1)
        m_configurations.push_back (raw_rule.m_property_first);
    if (raw_rule.m_property_second != -1)
        m_configurations.push_back (raw_rule.m_property_second);
    if (raw_rule.m_property_third != -1)
        m_configurations.push_back (raw_rule.m_property_third);
}

// EnforcementRule default destructor.
EnforcementRule::~EnforcementRule ()
{
    Logging::log_debug ("EnforcementRule default destructor.");
}

// get_rule_id call. Return EnforcementRule identifier.
uint64_t EnforcementRule::get_rule_id () const
{
    return this->m_rule_id;
}

// get_channel_id call. Return the Channel identifier.
long EnforcementRule::get_channel_id () const
{
    return this->m_channel_id;
}

// get_enforcement_object_id call. Return the EnforcementObject's identifier.
long EnforcementRule::get_enforcement_object_id () const
{
    return this->m_enforcement_object_id;
}

// get_operation_type call. Return the integer value of the operation to be enforced.
int EnforcementRule::get_operation_type () const
{
    return this->m_operation_type;
}

// get_configurations call. Return a copy of the configurations' container.
std::vector<long> EnforcementRule::get_configurations () const
{
    return this->m_configurations;
}

// get_configurations_size call. Return the number of elements in the EnforcementRule's
// configurations vector.
int EnforcementRule::get_configurations_size () const
{
    return static_cast<int> (this->m_configurations.size ());
}

// configurations_begin_iterator call. Returns an iterator pointing to the beginning of the
// EnforcementRule's configurations vector.
std::vector<long>::const_iterator EnforcementRule::configurations_begin_iterator () const
{
    return this->m_configurations.cbegin ();
}

// configurations_end_iterator call. Returns the end iterator of the EnforcementRule's
// configurations vector.
std::vector<long>::const_iterator EnforcementRule::configurations_end_iterator () const
{
    return this->m_configurations.cend ();
}

// to_string call. Generate a string with the HousekeepingRule values.
std::string EnforcementRule::to_string () const
{
    std::stringstream stream;
    stream << this->m_rule_id << ", ";
    stream << this->m_channel_id << ", ";
    stream << this->m_enforcement_object_id << ", ";
    stream << this->m_operation_type << ", {";
    for (long configuration : this->m_configurations) {
        stream << configuration << ",";
    }
    stream << "}";

    return stream.str ();
}
} // namespace paio::rules