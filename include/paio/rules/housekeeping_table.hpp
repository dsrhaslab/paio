/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_HOUSEKEEPING_TABLE_HPP
#define PAIO_HOUSEKEEPING_TABLE_HPP

#include <paio/rules/housekeeping_rule.hpp>
#include <paio/utils/logging.hpp>
#include <paio/utils/status.hpp>
#include <unordered_map>

namespace paio::rules {

/**
 * HousekeepingTable class.
 * An HousekeepingTable stores and manages all HousekeepingRules of a PAIO data plane stage. Rules
 * can be placed already enforced or scheduled to be later enforced.
 * This class holds four main instance variables, namely:
 *  - m_housekeeping_rules: Hashmap/Unordered map that contains all HousekeepingRules installed by
 *  the control plane;
 *  - m_number_of_rules: Atomic instance that defines the number of HousekeepingRules in the table,
 *  both enforced and not enforced;
 *  - m_number_of_rules_left_to_employ: Atomic instance that defines the total of HousekeepingRules
 *  that are still left to enforce;
 *  - m_lock: handle concurrency control.
 * Future work:
 *  - Scheduling rules to be later enforced.
 */
class HousekeepingTable {
    // friends of HousekeepingTable class
    friend class HousekeepingTableTest;

private:
    std::unordered_map<uint64_t, HousekeepingRule> m_housekeeping_rules {};
    std::atomic<int> m_number_of_rules { 0 };
    std::atomic<int> m_number_of_rules_left_to_employ { 0 };
    std::mutex m_lock;

    /**
     * get_begin_iterator: Get the beginning m_housekeeping_rules iterator to traverse the table.
     * @return Returns a const_iterator to the beginning of the m_housekeeping_rules container.
     */
    std::unordered_map<uint64_t, HousekeepingRule>::const_iterator get_begin_iterator () const;

    /**
     * get_end_iterator: Get the end m_housekeeping_rules iterator to traverse the table.
     * @return Returns a const_iterator to the end of the m_housekeeping_rules container.
     */
    std::unordered_map<uint64_t, HousekeepingRule>::const_iterator get_end_iterator () const;

public:
    /**
     * HousekeepingTable default constructor.
     */
    HousekeepingTable ();

    /**
     * HousekeepingTable default destructor.
     */
    ~HousekeepingTable ();

    /**
     * insert_housekeeping_rule: Insert a new HousekeepingRule in the table. If the the rule already
     * exists (rule_id), the insertion will not take place; otherwise it creates the new element and
     * increments the number of existing rules parameter.
     * On creation, the method uses the HousekeepingRule's parameterized constructor.
     * The method is thread-safe.
     * @param rule_id Defines the HousekeepingRule's identifier.
     * @param housekeeping_operation Defines the operation type.
     * @param channel_id Defines the Channel identifier of which the rule corresponds to.
     * @param enforcement_object_id Defines the EnforcementObject identifier of which the rule
     * corresponds to.
     * @param properties Defines the initial configuration properties to be set.
     * @return Returns PStatus::OK if the rule was successfully inserted; PStatus::Error otherwise.
     */
    PStatus insert_housekeeping_rule (const uint64_t& rule_id,
        const HousekeepingOperation& housekeeping_operation,
        const long& channel_id,
        const long& enforcement_object_id,
        const std::vector<long>& properties);

    /**
     * insert_housekeeping_rule: Insert a new HousekeepingRule in the table. If the the rule already
     * exists (rule_id), the insertion will not take place; otherwise it creates the new element and
     * increments the number of existing rules parameter.
     * On creation, the method uses the HousekeepingRule's copy constructor.
     * The method is thread-safe.
     * @param rule HousekeepingRule object.
     * @return Returns PStatus::OK if the rule was successfully inserted; PStatus::Error otherwise.
     */
    PStatus insert_housekeeping_rule (const HousekeepingRule& rule);

    /**
     * select_housekeeping_rule: Verify and copy an housekeeping rule with rule_id, if exists.
     * The method is thread-safe.
     * @param rule_id HousekeepingRule's identifier
     * @param housekeeping_rule Reference to an HousekeepingRule to perform the copy.
     * @return Returns PStatus::OK if the rule exists; PStatus::NotFound otherwise.
     */
    PStatus select_housekeeping_rule (const uint64_t& rule_id, HousekeepingRule& housekeeping_rule);

    /**
     * get_housekeeping_table_begin_iterator: Get the beginning m_housekeeping_rules iterator to
     * traverse the table.
     * This method is thread-safe.
     * @return Returns a const_iterator to the beginning of the m_housekeeping_rules container.
     */
    std::unordered_map<uint64_t, HousekeepingRule>::const_iterator
    get_housekeeping_table_begin_iterator ();

    /**
     * get_housekeeping_table_end_iterator: Get the ending m_housekeeping_rules iterator to traverse
     * the table.
     * This method is thread-safe.
     * @return Returns a const_iterator to the ending of the m_housekeeping_rules container.
     */
    std::unordered_map<uint64_t, HousekeepingRule>::const_iterator
    get_housekeeping_table_end_iterator ();

    /**
     * mark_housekeeping_rule_as_enforced: Mark HousekeepingRule with rule_id as enforced. When
     * this operation is invoked, it means that the HousekeepingRule was employed in the data plane
     * stage.
     * The method is thread-safe.
     * @param rule_id HousekeepingRule's identifier.
     * @return Returns PStatus::OK if the rule was successfully marked as enforced; PStatus::Error
     * if the rule was already enforced; and PStatus::NotFound if the rule does not exist.
     */
    PStatus mark_housekeeping_rule_as_enforced (const uint64_t& rule_id);

    /**
     * remove_housekeeping_rule: Remove HousekeepingRule with rule_id from the table.
     * The method is thread-safe.
     * @param rule_id HousekeepingRule's identifier
     * @return returns PStatus::OK if the rule was removed from the table; PStatus::NotFound if
     * rule_id does not exists.
     */
    PStatus remove_housekeeping_rule (const uint64_t& rule_id);

    /**
     * get_housekeeping_table_size: Get the total number of rules that exist in the
     * HousekeepingTable.
     * The method is thread-safe since the m_number_of_rules is atomic.
     * @return Returns a copy of the m_number_of_rules parameter.
     */
    int get_housekeeping_table_size () const;

    /**
     * get_total_of_rules_left_to_employ: Get the total number of rules that exist in the
     * HousekeepingTable that were still not employed in the data plane stage.
     * The method is thread-safe since the m_number_of_rules_left_to_employ is atomic.
     * @return Returns a copy of the m_number_of_rules_left_to_employ parameter.
     */
    int get_total_of_rules_left_to_employ () const;

    /**
     * to_string: Generate a string with all housekeeping rules present in the table.
     * @return Returns a string-based format of all rules present in the table.
     */
    std::string to_string ();
};
} // namespace paio::rules

#endif // PAIO_HOUSEKEEPING_TABLE_HPP
