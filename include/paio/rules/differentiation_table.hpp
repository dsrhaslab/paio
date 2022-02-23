/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_DIFFERENTIATION_TABLE_HPP
#define PAIO_DIFFERENTIATION_TABLE_HPP

#include <memory>
#include <paio/rules/differentiation_rule.hpp>
#include <paio/utils/status.hpp>
#include <unordered_map>

namespace paio::rules {

/**
 * DifferentiationTable class.
 * A DifferentiationTable stores and manages all DifferentiationRules of a PAIO data plane stage.
 * A differentiation table is unique to the data plane stage contains the rules that define how
 * I/O requests should be handled.
 * This class holds three main instance variable, namely:
 *  - m_differentiation_rules: Unordered map (hashmap) that contains all DifferentiationRule
 *  objects installed at the data plane stage, which were submitted by the control plane or locally
 *  generated from a differentiation rule's file;
 *  - m_number_of_rules: Atomic instance that defines the number of DifferentiationRules in the
 *  table;
 *  - m_lock: mutual exclusion primitive to ensure concurrency control.
 * TODO:
 *  - decouple the I/O classification and differentiation part of HousekeepingRule objects.
 */
class DifferentiationTable {
    // friends of DifferentiationTable class
    friend class DifferentiationTableTest;

private:
    std::unordered_map<uint64_t, DifferentiationRule> m_differentiation_rules {};
    std::atomic<int> m_number_of_rules { 0 };
    std::mutex m_lock;

public:
    /**
     * DifferentiationTable default constructor.
     */
    DifferentiationTable ();

    /**
     * DifferentiationTable default destructor.
     */
    ~DifferentiationTable ();

    /**
     * insert_differentiation_rule: Insert a new DifferentiationRule in the table. If the rule
     * already exists (rule_id), the insertion will not take place; otherwise, it creates the new
     * element and increments the number of existing rules parameter.
     * On creation, the method uses the DifferentiationRule's parameterized constructor.
     * The method is thread-safe.
     * @param rule DifferentiationRule object.
     * @return Returns PStatus::OK if the rule was successfully inserted; PStatus::Error otherwise.
     */
    PStatus insert_differentiation_rule (const DifferentiationRule& rule);

    /**
     * insert_differentiation_rule: Insert a new DifferentiationRule in the table. If the rule
     * already exists (rule_id), the insertion will not take place; otherwise, it creates the new
     * element and increments the number of existing rules parameter.
     * On creation, the method uses the DifferentiationRule's parameterized constructor.
     * The method is thread-safe.
     * @param rule_id Defines the DifferentiationRule's identifier
     * @param rule_type Defines the differentiation type.
     * @param channel_id Defines the Channel identifier of which the rule corresponds to.
     * @param enforcement_object_id Defines the EnforcementObject identifier of which the rule
     * corresponds to.
     * @param workflow_id Defines the workflow identifier classifier to perform the differentiation.
     * @param operation_type Defines the operation type classifier to perform the differentiation.
     * @param operation_context Defines the operation context classifier to perform the
     * differentiation.
     * @return Returns PStatus::OK if the rule was successfully inserted; PStatus::Error otherwise.
     */
    PStatus insert_differentiation_rule (const uint64_t& rule_id,
        const DifferentiationRuleType& rule_type,
        const long& channel_id,
        const long& enforcement_object_id,
        const uint32_t& workflow_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context);

    /**
     * select_differentiation_rule: Verify and copy an differentiation rule with rule_id, if exists.
     * The method is thread-safe.
     * @param rule_id DifferentiationRule's identifier
     * @param differentiation_rule Reference to an DifferentiationRule to perform the copy.
     * @return Returns PStatus::OK if the rule exists; PStatus::NotFound otherwise.
     */
    PStatus select_differentiation_rule (const uint64_t& rule_id,
        DifferentiationRule& differentiation_rule);

    /**
     * remove_differentiation_rule: Remove DifferentiationRule with rule_id from the table.
     * The method is thread-safe.
     * @param rule_id DifferentiationRule's identifier
     * @return returns PStatus::OK if the rule was removed from the table; PStatus::NotFound if
     * rule_id does not exists.
     */
    PStatus remove_differentiation_rule (const uint64_t& rule_id);

    /**
     * get_differentiation_table_size: Get the total number of rules that exist in the
     * DifferentiationTable.
     * The method is thread-safe since the m_number_of_rules is atomic.
     * @return Returns a copy of the m_number_of_rules parameter.
     */
    int get_differentiation_table_size () const;

    /**
     * get_differentiation_table_begin_iterator: Get the beginning m_differentiation_rules iterator
     * to traverse the table.
     * @return Returns a const_iterator to the beginning of the m_differentiation_rules container.
     */
    std::unordered_map<uint64_t, DifferentiationRule>::const_iterator
    get_differentiation_table_begin_iterator () const;

    /**
     * get_differentiation_table_end_iterator: Get the ending m_differentiation_rules iterator to
     * traverse the table.
     * @return Returns a const_iterator to the ending of the m_differentiation_rules container.
     */
    std::unordered_map<uint64_t, DifferentiationRule>::const_iterator
    get_differentiation_table_end_iterator () const;

    /**
     * to_string: Generate a string with all differentiation rules present in the table.
     * @return Returns a string-based format of all rules present in the table.
     */
    std::string to_string ();
};
} // namespace paio::rules

#endif // PAIO_DIFFERENTIATION_TABLE_HPP
