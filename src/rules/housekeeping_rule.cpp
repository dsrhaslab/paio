/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/rules/housekeeping_rule.hpp>
#include <stdexcept>

namespace paio::rules {

// HousekeepingRule default constructor.
HousekeepingRule::HousekeepingRule () = default;

// HousekeepingRule parameterized constructor.
HousekeepingRule::HousekeepingRule (const uint64_t& id,
    const HousekeepingOperation& operation,
    const long& channel,
    const long& enforcement_object,
    std::vector<long> properties) :
    m_rule_id { id },
    m_rule_type { operation },
    m_channel_id { channel },
    m_enforcement_object_id { enforcement_object },
    m_properties { std::move (properties) }
{ }

// HousekeepingRule default destructor.
HousekeepingRule::~HousekeepingRule () = default;

// get_rule_id call. Return HousekeepingRule identifier.
uint64_t HousekeepingRule::get_rule_id () const
{
    return this->m_rule_id;
}

// get_housekeeping_operation_type call. Return the HousekeepingOperation type.
HousekeepingOperation HousekeepingRule::get_housekeeping_operation_type () const
{
    return this->m_rule_type;
}

// get_channel_id call. Return the Channel identifier.
long HousekeepingRule::get_channel_id () const
{
    return this->m_channel_id;
}

// get_enforcement_object_id call. Return the EnforcementObject's identifier.
long HousekeepingRule::get_enforcement_object_id () const
{
    return this->m_enforcement_object_id;
}

// properties_begin_iterator call. Returns an iterator pointing to the beginning of the
// HousekeepingRule's properties vector.
std::vector<long>::const_iterator HousekeepingRule::properties_begin_iterator () const
{
    return this->m_properties.cbegin ();
}

// properties_end_iterator call. Returns an iterator pointing to the end of the HousekeepingRule's
// properties vector.
std::vector<long>::const_iterator HousekeepingRule::properties_end_iterator () const
{
    return this->m_properties.cend ();
}

// get_property_at_index call. Returns a copy of the property's value at a given index.
long HousekeepingRule::get_property_at_index (const int& index) const
{
    long property_tmp = -1;
    // validate if index is within the bounds of the vector
    if (static_cast<int> (this->m_properties.size ()) >= (index + 1)) {
        // copy the property value
        property_tmp = this->m_properties.at (index);
    }

    return property_tmp;
}

// get_properties_at_range call. Given two indexes, copies the HousekeepingRule's properties within
// the index range.
int HousekeepingRule::get_properties_at_range (const int& begin_index,
    const int& end_index,
    std::vector<long>& selected_properties) const
{
    int properties_selected = 0;

    // validate if indexes are within the bounds of the vector
    if (static_cast<int> (this->m_properties.size ()) >= (begin_index + 1)
        && static_cast<int> (this->m_properties.size ()) >= (end_index + 1)
        && begin_index <= end_index) {
        // copy values in range to the selected_properties vector
        for (int i = begin_index; i <= end_index; i++) {
            // fixme
            // selected_properties[properties_selected] = this->m_properties.at (i);
            selected_properties.push_back (this->m_properties.at (i));
            properties_selected++;
        }
    } else {
        properties_selected = -1;
    }

    return properties_selected;
}

// get_properties_size call. Return number of elements in the HousekeepingRule's properties vector.
int HousekeepingRule::get_properties_size () const
{
    return static_cast<int> (this->m_properties.size ());
}

// get_enforced call. Return a bool that defines if the HousekeepingRule has been already enforced.
bool HousekeepingRule::get_enforced () const
{
    return this->m_enforced;
}

// set_enforced call. Update the HousekeepingRule's m_enforce value.
void HousekeepingRule::set_enforced (bool value)
{
    this->m_enforced = value;
}

// operation_to_string call. Convert an HousekeepingOperation value to string-based format.
std::string HousekeepingRule::operation_to_string () const
{
    switch (this->m_rule_type) {
        case HousekeepingOperation::create_channel:
            return "create_channel";

        case HousekeepingOperation::create_object:
            return "create_object";

        case HousekeepingOperation::configure:
            return "configure";

        case HousekeepingOperation::remove:
            return "remove";

        case HousekeepingOperation::no_op:
            return "no_op";

        default:
            throw std::logic_error { "HousekeepingRule: unexpected rule type" };
    }
}

// to_string call. Generate a string with the HousekeepingRule values.
std::string HousekeepingRule::to_string () const
{
    std::stringstream stream;

    stream << this->m_rule_id << ", ";
    stream << this->operation_to_string () << ", ";
    stream << this->m_channel_id << ", ";
    stream << this->m_enforcement_object_id << ", {";
    for (long property : this->m_properties) {
        stream << property << ",";
    }
    stream << "}, " << (this->m_enforced ? "true" : "false");

    return stream.str ();
}

}; // namespace paio::rules
