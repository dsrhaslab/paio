/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/rules/differentiation_table.hpp>

namespace paio::rules {

// DifferentiationTable default constructor.
DifferentiationTable::DifferentiationTable () = default;

// DifferentiationTable default destructor.
DifferentiationTable::~DifferentiationTable () = default;

// insertRule call. Insert a new DifferentiationRule in the table.
PStatus DifferentiationTable::insert_differentiation_rule (const DifferentiationRule& rule)
{
    std::unique_lock<std::mutex> unique_lock (this->m_lock);

    // insert DifferentiationRule with copy constructor
    auto return_pair = m_differentiation_rules.emplace (std::piecewise_construct,
        std::forward_as_tuple (rule.get_rule_id ()),
        std::forward_as_tuple (rule));

    // out of critical section - release lock
    unique_lock.unlock ();

    // validate return value and increment number of rules
    if (return_pair.second) {
        this->m_number_of_rules++;
        return PStatus::OK ();
    } else {
        // create error message
        std::stringstream error_message_stream;
        error_message_stream << "Error on inserting differentiation rule (";
        error_message_stream << rule.get_rule_id ();
        error_message_stream << ")";

        // create error logging message
        if (return_pair.first != this->get_differentiation_table_end_iterator ()) {
            error_message_stream << ": rule already exists.";
        }

        // log error message
        Logging::log_error (error_message_stream.str ());
        return PStatus::Error ();
    }
}

// insertRule call. Insert a new DifferentiationRule in the table.
PStatus DifferentiationTable::insert_differentiation_rule (const uint64_t& rule_id,
    const DifferentiationRuleType& rule_type,
    const long& channel_id,
    const long& enforcement_object_id,
    const uint32_t& workflow_id,
    const uint32_t& operation_type,
    const uint32_t& operation_context)
{
    std::unique_lock<std::mutex> unique_lock (this->m_lock);

    // insert DifferentiationRule with parameterized constructor
    auto return_pair = m_differentiation_rules.emplace (std::piecewise_construct,
        std::forward_as_tuple (rule_id),
        std::forward_as_tuple (rule_id,
            rule_type,
            channel_id,
            enforcement_object_id,
            workflow_id,
            operation_type,
            operation_context));

    // out of critical section - release lock
    unique_lock.unlock ();

    // validate return value and increment number of rules
    if (return_pair.second) {
        this->m_number_of_rules++;
        return PStatus::OK ();
    } else {
        // create error message
        std::stringstream error_message_stream;
        error_message_stream << "Error on inserting differentiation rule (";
        error_message_stream << rule_id;
        error_message_stream << ")";

        // create error logging message
        if (return_pair.first != this->get_differentiation_table_end_iterator ()) {
            error_message_stream << ": rule already exists.";
        }

        // log error message
        Logging::log_error (error_message_stream.str ());
        return PStatus::Error ();
    }
}

// select_differentiation_rule call. Validate if a rule exists and copy it.
PStatus DifferentiationTable::select_differentiation_rule (const uint64_t& rule_id,
    DifferentiationRule& differentiation_rule)
{
    std::lock_guard<std::mutex> guard (this->m_lock);

    // create iterator that points to the address of the differentiation rule
    auto iter = this->m_differentiation_rules.find (rule_id);

    // verify if rule exists and copy it
    if (iter != this->m_differentiation_rules.end ()) {
        // use DifferentiationRule's copy constructor to pass the rule
        differentiation_rule = this->m_differentiation_rules.at (rule_id);
        return PStatus::OK ();
    } else {
        // create error logging message
        Logging::log_error ("Error on selecting differentiation rule (" + std::to_string (rule_id)
            + "): does not exist.");

        return PStatus::NotFound ();
    }
}

// remove_differentiation_rule call. Remove a DifferentiationRule from the table.
PStatus DifferentiationTable::remove_differentiation_rule (const uint64_t& rule_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);

    // validate if rule exists
    auto iter = this->m_differentiation_rules.find (rule_id);

    // erase rule from the table
    if (iter != this->m_differentiation_rules.end ()) {
        // remove rule from container and update m_number_of_rules counter
        this->m_differentiation_rules.erase (iter);
        this->m_number_of_rules--;

        return PStatus::OK ();
    } else {
        Logging::log_error ("Error on removing differentiation rule (" + std::to_string (rule_id)
            + "): does not exist.");
        return PStatus::NotFound ();
    }
}

// get_differentiation_table_size call. Return the number of DifferentiationRules in the table.
int DifferentiationTable::get_differentiation_table_size () const
{
    return this->m_number_of_rules.load ();
}

// get_differentiation_table_begin_iterator call. Get the beginning m_differentiation_rules iterator
// to traverse the table.
std::unordered_map<uint64_t, DifferentiationRule>::const_iterator
DifferentiationTable::get_differentiation_table_begin_iterator () const
{
    return this->m_differentiation_rules.cbegin ();
}

// get_differentiation_table_end_iterator call. Get the ending m_differentiation_rules iterator to
// traverse the table.
std::unordered_map<uint64_t, DifferentiationRule>::const_iterator
DifferentiationTable::get_differentiation_table_end_iterator () const
{
    return this->m_differentiation_rules.cend ();
}

// to_string call. Generate a string with all DifferentiationRules present on the table.
std::string DifferentiationTable::to_string ()
{
    std::stringstream stream;
    stream << "Differentiation table (";
    stream << this->m_number_of_rules << ")\n";

    {
        std::unique_lock<std::mutex> unique_lock (this->m_lock);
        for (auto& rule : this->m_differentiation_rules) {
            stream << "\t" << rule.first << " : {" << rule.second.to_string () << "}\n";
        }
    }

    return stream.str ();
}

} // namespace paio::rules
