/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/rules/housekeeping_table.hpp>

namespace paio::rules {

// HousekeepingTable default constructor.
HousekeepingTable::HousekeepingTable ()
{
    Logging::log_debug ("HousekeepingTable default constructor.");
}

// HousekeepingTable default destructor.
HousekeepingTable::~HousekeepingTable () = default;

// insert_housekeeping_rule call. Inserts a new HousekeepingRule (parameterized) in the table.
PStatus HousekeepingTable::insert_housekeeping_rule (const uint64_t& rule_id,
    const HousekeepingOperation& operation,
    const long& channel_id,
    const long& enforcement_object_id,
    const std::vector<long>& properties)
{
    std::unique_lock<std::mutex> lock (this->m_lock);

    // insert HousekeepingRule with parameterized constructor
    auto return_pair = this->m_housekeeping_rules.emplace (std::piecewise_construct,
        std::forward_as_tuple (rule_id),
        std::forward_as_tuple (rule_id, operation, channel_id, enforcement_object_id, properties));

    // out of critical section - release lock
    lock.unlock ();

    // validate return value and increment number of rules
    if (return_pair.second) {
        this->m_number_of_rules++;
        this->m_number_of_rules_left_to_employ++;
        return PStatus::OK ();
    } else {
        // create error message
        std::stringstream error_message_stream;
        error_message_stream << "Error on inserting housekeeping rule (";
        error_message_stream << rule_id;
        error_message_stream << ")";

        if (return_pair.first != this->get_housekeeping_table_end_iterator ()) {
            error_message_stream << ": rule already exists.";
        }

        // log error message
        Logging::log_error (error_message_stream.str ());
        return PStatus::Error ();
    }
}

// insertRule call. Inserts an HousekeepingRule with copy constructor.
PStatus HousekeepingTable::insert_housekeeping_rule (const HousekeepingRule& rule)
{
    std::unique_lock<std::mutex> lock (this->m_lock);

    // insert HousekeepingRule with copy constructor
    auto return_pair = this->m_housekeeping_rules.emplace (std::piecewise_construct,
        std::forward_as_tuple (rule.get_rule_id ()),
        std::forward_as_tuple (rule));

    // out of critical section - release lock
    lock.unlock ();

    // validate return value and increment number of rules
    if (return_pair.second) {
        this->m_number_of_rules++;
        this->m_number_of_rules_left_to_employ++;
        return PStatus::OK ();
    } else {
        // create error message
        std::stringstream error_message_stream;
        error_message_stream << "Error on inserting housekeeping rule (";
        error_message_stream << rule.get_rule_id ();
        error_message_stream << ")";

        if (return_pair.first != this->get_housekeeping_table_end_iterator ()) {
            error_message_stream << ": rule already exists.";
        }

        // log error message
        Logging::log_error (error_message_stream.str ());
        return PStatus::Error ();
    }
}

// select_housekeeping_rule call. Validate if a rule exists and copy it.
PStatus HousekeepingTable::select_housekeeping_rule (const uint64_t& rule_id,
    HousekeepingRule& rule)
{
    std::lock_guard<std::mutex> guard (this->m_lock);

    // create iterator that points to the address of the housekeeping rule
    auto iter = this->m_housekeeping_rules.find (rule_id);

    // verify if rule exists and copy it
    if (iter != this->m_housekeeping_rules.end ()) {
        // use HousekeepingRule's copy constructor to pass the rule
        rule = this->m_housekeeping_rules.at (rule_id);
        return PStatus::OK ();
    } else {
        // create error logging message
        Logging::log_error ("Error on selecting housekeeping rule (" + std::to_string (rule_id)
            + "): does not exist.");

        return PStatus::NotFound ();
    }
}

// get_begin_iterator call. Returns a const_iterator to the beginning of the HousekeepingTable.
std::unordered_map<uint64_t, HousekeepingRule>::const_iterator
HousekeepingTable::get_begin_iterator () const
{
    return this->m_housekeeping_rules.cbegin ();
}

// get_housekeeping_table_begin_iterator call. Returns the begin iterator of the HousekeepingTable.
std::unordered_map<uint64_t, HousekeepingRule>::const_iterator
HousekeepingTable::get_housekeeping_table_begin_iterator ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    return this->get_begin_iterator ();
}

// get_end_iterator call. Returns const_iterator to the end of the HousekeepingTable.
std::unordered_map<uint64_t, HousekeepingRule>::const_iterator
HousekeepingTable::get_end_iterator () const
{
    return this->m_housekeeping_rules.cend ();
}

// get_housekeeping_table_end_iterator call. Returns the end iterator of the HousekeepingTable.
std::unordered_map<uint64_t, HousekeepingRule>::const_iterator
HousekeepingTable::get_housekeeping_table_end_iterator ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    return this->get_end_iterator ();
}

// mark_housekeeping_rule_as_enforced call. Mark HousekeepingRule as enforced.
PStatus HousekeepingTable::mark_housekeeping_rule_as_enforced (const uint64_t& rule_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);

    // validate if rule exists
    auto iter = this->m_housekeeping_rules.find (rule_id);

    //  update HousekeepingRule's m_enforced
    if (iter != this->m_housekeeping_rules.end ()) {
        if (!iter->second.get_enforced ()) {
            iter->second.set_enforced (true);
            this->m_number_of_rules_left_to_employ--;
            return PStatus::OK ();
        } else {
            Logging::log_error ("Error on enforcing housekeeping rule (" + std::to_string (rule_id)
                + "): already enforced.");
            return PStatus::Error ();
        }
    } else {
        Logging::log_error ("Error on enforcing housekeeping rule (" + std::to_string (rule_id)
            + "): does not exist.");
        return PStatus::NotFound ();
    }
}

// remove_housekeeping_rule call. Remove an HousekeepingRule from the table.
PStatus HousekeepingTable::remove_housekeeping_rule (const uint64_t& rule_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);

    // validate if rule exists
    auto iter = this->m_housekeeping_rules.find (rule_id);

    // erase rule from the table
    if (iter != this->m_housekeeping_rules.end ()) {
        // verify if rule was not enforced and update m_number_of_rules_left_to_employ counter
        if (!iter->second.get_enforced ()) {
            this->m_number_of_rules_left_to_employ--;
        }

        // remove rule from container and update m_number_of_rules counter
        this->m_housekeeping_rules.erase (iter);
        this->m_number_of_rules--;

        return PStatus::OK ();
    } else {
        Logging::log_error ("Error on removing housekeeping rule (" + std::to_string (rule_id)
            + "): does not exist.");
        return PStatus::NotFound ();
    }
}

// get_housekeeping_table_size call. Get the number of HousekeepingRules in the table.
int HousekeepingTable::get_housekeeping_table_size () const
{
    return this->m_number_of_rules.load ();
}

// get_total_of_rules_left_to_employ call. Get the number of HousekeepingRules in the table that are
// left to employ.
int HousekeepingTable::get_total_of_rules_left_to_employ () const
{
    return this->m_number_of_rules_left_to_employ.load ();
}

// to_string call. Generate a string with all HousekeepingRules present on the table.
std::string HousekeepingTable::to_string ()
{
    std::stringstream stream;
    stream << "Housekeeping table (";
    stream << this->m_number_of_rules << ", " << this->m_number_of_rules_left_to_employ << ")\n";

    {
        std::unique_lock<std::mutex> lock (this->m_lock);
        for (auto& rule : this->m_housekeeping_rules) {
            stream << "\t" << rule.first << " : {" << rule.second.to_string () << "}\n";
        }
    }

    return stream.str ();
}

} // namespace paio::rules
