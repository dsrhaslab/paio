/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/objects/noop/enforcement_object_noop.hpp>

namespace paio::enforcement {

// NoopObject default constructor.
NoopObject::NoopObject ()
{
    Logging::log_debug ("NoopObject default constructor.");
}

// NoopObject parameterized constructor.
NoopObject::NoopObject (const long& object_id) : m_object_id { object_id }
{
    Logging::log_debug ("NoopObject parameterized constructor.");
}

// NoopObject parameterized constructor.
NoopObject::NoopObject (const long& object_id, const std::string& tag_name, const bool& is_shared) :
    m_object_id { object_id },
    m_tag_name { tag_name },
    m_shared { is_shared }
{
    Logging::log_debug ("NoopObject parameterized constructor.");
}

// NoopObject default destructor.
NoopObject::~NoopObject ()
{
    std::stringstream stream;
    stream << "NoopObject destructor {" << this->m_object_id << ", ";
    (!this->m_tag_name.empty ()) ? stream << this->m_tag_name : stream << "counter";
    stream << ", " << this->m_counter << "}";

    Logging::log_debug_explicit (stream.str ());
}

// get_enforcement_object_id call. Get the EnforcementObject's identifier.
long NoopObject::get_enforcement_object_id () const
{
    return this->m_object_id;
}

// obj_enforce call. Apply the Noop enforcement mechanisms over I/O workflows.
void NoopObject::obj_enforce (const Ticket& ticket, Result& result)
{
    this->increment_counter ();

    // set the Result's result_status and has_content parameters
    bool has_content = (ticket.get_buffer_size () > 0);
    result.set_result_status (ResultStatus::success);
    result.set_has_content (has_content);

    // if the Ticket contains request's data/metadata, it will be copied to the Result object
    if (has_content) {
        result.set_content_size (ticket.get_buffer_size ());
        result.set_content (ticket.get_buffer_size (), ticket.get_buffer ());
    }
}

// obj_configure call. Configure the internal properties of the Noop enforcement object.
PStatus NoopObject::obj_configure ([[maybe_unused]] int conf,
    [[maybe_unused]] const std::vector<long>& configuration_values)
{
    return PStatus::OK ();
}

// obj_collect_statistics call. Collect I/O statistics from the Noop enforcement object.
PStatus NoopObject::obj_collect_statistics ([[maybe_unused]] ObjectStatisticsRaw& statistics_raw)
{
    return PStatus::Error ();
}

// to_string call. Convert any content of the Noop enforcement object in string-based format.
std::string NoopObject::to_string ()
{
    return std::string ("Noop enforcement object (")
        .append (std::to_string (this->m_object_id))
        .append (").\n");
}

// increment_counter call. Increment the Noop counter considering if the object is shared or not.
void NoopObject::increment_counter ()
{
    // if shared, use a mutex to ensure consistent state
    if (this->m_shared) {
        // critical section (might introduce some overhead)
        {
            std::lock_guard<std::mutex> guard (this->m_lock);
            // update m_counter
            this->m_counter++;
        }
    } else {
        this->m_counter++;
    }
}

} // namespace paio::enforcement
