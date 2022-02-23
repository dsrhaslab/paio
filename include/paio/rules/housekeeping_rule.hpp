/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_HOUSEKEEPING_RULE_HPP
#define PAIO_HOUSEKEEPING_RULE_HPP

#include <cstdint>
#include <iostream>
#include <paio/utils/logging.hpp>
#include <sstream>
#include <vector>

using namespace paio::utils;

namespace paio::rules {

/**
 * HousekeepingOperation enum class.
 * Defines the supported types of housekeeping operations.
 * Currently, it supports two main operations for creating channels and enforcements objects in a
 * PAIO data plane stage. The remainder are expected to be improved as future work.
 *  - create_channel: Create a new Channel to receive I/O requests;
 *  - create_object: Create a new EnforcementObject, that respects to an existing Channel.
 *  - no_op: Default housekeeping rule type; does not correspond to any operation.
 *  Other types:
 *  - update: not implemented
 *  - remove: not implemented
 */
enum class HousekeepingOperation {
    create_channel = 1,
    create_object = 2,
    configure = 3,
    remove = 4,
    no_op = 0
};

/**
 * HousekeepingRule class.
 * An HousekeepingRule provides the means to create, configure, and remove core enforcement
 * primitives of the data plane stage, such as Channels and EnforcementObjects. Each
 * HousekeepingRule respects to a specific Channel or a combination of Channel and
 * EnforcementObject. Each HousekeepingRule comprises six parameters:
 *  - m_rule_id: Defines de HousekeepingRule's identifier;
 *  - m_rule_type: Defines the type of the HousekeepingRule; it is presented as an
 *  HousekeepingOperation, and can be of type create_channel, create_object, configure, remove, or
 *  no_op;
 *  - m_channel_id: Defines the Channel's identifier to apply the rule;
 *  - m_enforcement_object_id: Defines the EnforcementObjects's identifier to apply the rule; a
 *  value of -1 means that the rules is not targeted to any EnforcementObject in particular;
 *  - m_properties: Defines the parameters to be set in the initial configuration;
 *  - m_enforced: Defines if the rule was already enforced.
 * Currently, HousekeepingRules are stored in an HousekeepingRuleTable, and are enforced at
 * creation time (to ease of use). This can be later changed to allow the execution of
 * HousekeepingRules to be scheduled or upon a certain threshold is hit.
 * TODO:
 *  - m_properties: configurations should be of generic type (to handle different properties);
 *  - add tests for get_property_at_index and get_properties_at_range.
 */
class HousekeepingRule {

private:
    uint64_t m_rule_id { 0 };
    HousekeepingOperation m_rule_type { HousekeepingOperation::no_op };
    long m_channel_id { -1 };
    long m_enforcement_object_id { -1 };
    std::vector<long> m_properties {};
    bool m_enforced { false };

public:
    /**
     * HousekeepingRule default constructor.
     */
    HousekeepingRule ();

    /**
     * HousekeepingRule parameterized constructor.
     * @param rule_id HousekeepingRule identifier.
     * @param housekeeping_rule_type HousekeepingRule operation type.
     * @param channel_id Channel identifier.
     * @param enforcement_object_id EnforcementObject identifier.
     * @param properties Complementary properties of the HousekeepingRule.
     */
    HousekeepingRule (const uint64_t& rule_id,
        const HousekeepingOperation& housekeeping_rule_type,
        const long& channel_id,
        const long& enforcement_object_id,
        std::vector<long> properties);

    /**
     * HousekeepingRule default destructor.
     */
    ~HousekeepingRule ();

    /**
     * get_rule_id: Get HousekeepingRule identifier.
     * @return Return a copy of the m_rule_id parameter.
     */
    [[nodiscard]] uint64_t get_rule_id () const;

    /**
     * get_housekeeping_operation_type: Get the HousekeepingOperation type.
     * @return Return a copy of the m_rule_type parameter.
     */
    [[nodiscard]] HousekeepingOperation get_housekeeping_operation_type () const;

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
     * properties_begin_iterator: Get the begin's const_iterator of the HousekeepingRule's
     * complementary properties (m_properties).
     * @return Returns a const_iterator to the beginning of the m_properties container.
     */
    [[nodiscard]] std::vector<long>::const_iterator properties_begin_iterator () const;

    /**
     * properties_end_iterator: Get the end's const_iterator of the HousekeepingRule's complementary
     * properties (m_properties).
     * @return Returns a const_iterator to the ending of the m_properties container.
     */
    [[nodiscard]] std::vector<long>::const_iterator properties_end_iterator () const;

    /**
     * get_property_at_index: Get a copy of an HousekeepingRule's property at a given index.
     * The method validates if the index is within the bounds of the m_properties container.
     * @param index Index to get the property's value.
     * @return Returns a copy of the property's value at the specified index; -1 if index is
     * out-of-bounds.
     */
    [[nodiscard]] long get_property_at_index (const int& index) const;

    /**
     * get_properties_at_range: Get HousekeepingRule's properties at a given index range.
     * The method validates if the indexes, begin_index and end_index, are within the bounds of the
     * m_properties container.
     * @param begin_index Index of the beginning of the range.
     * @param end_index Index of the ending of the range.
     * @param selected_properties Reference to a vector that will hold a copy of the properties
     * within the range.
     * @return Returns the number of copied properties; -1 if indexes are out-of-bounds.
     */
    [[nodiscard]] int get_properties_at_range (const int& begin_index,
        const int& end_index,
        std::vector<long>& selected_properties) const;

    /**
     * get_properties_size: Get the number of elements in the m_properties vector.
     * @return Number of elements in the m_properties container.
     */
    [[nodiscard]] int get_properties_size () const;

    /**
     * get_enforced: Get the value of m_enforced parameter, which validates if the rule was already
     * enforced or not.
     * @return Return a copy of the m_enforced parameter value.
     */
    [[nodiscard]] bool get_enforced () const;

    /**
     * set_enforced: Update the m_enforced value.
     * @param value New value to be considered.
     */
    void set_enforced (bool value);

    /**
     * operation_to_string: Convert HousekeepingOperation value to string-based format.
     * @return Returns string value of the HousekeepingOperation.
     */
    [[nodiscard]] std::string operation_to_string () const;

    /**
     * toString: generate a string with the HousekeepingRule values.
     * @return HousekeepingRule values in string-based format.
     */
    [[nodiscard]] std::string to_string () const;
};
} // namespace paio::rules

#endif // PAIO_HOUSEKEEPING_RULE_HPP
