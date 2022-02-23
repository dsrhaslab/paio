/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/interface/paio_instance.hpp>

namespace paio {

// PaioInstance default constructor.
PaioInstance::PaioInstance () : InstanceInterface {}
{
    Logging::log_debug ("PaioInstance default constructor.");
}

// PaioInstance (explicit) parameterized constructor.
PaioInstance::PaioInstance (std::shared_ptr<PaioStage> stage_ptr) :
    InstanceInterface { std::move (stage_ptr) }
{
    Logging::log_debug ("PaioInstance parameterized constructor.");
}

// PaioInstance parameterized constructor.
PaioInstance::PaioInstance (std::shared_ptr<PaioStage> stage_ptr, const long& default_workflow_id) :
    InstanceInterface { std::move (stage_ptr) }
{
    Logging::log_debug ("PaioInstance parameterized constructor.");
    this->set_default_workflow_id (default_workflow_id);
    this->set_default_operation_type (static_cast<int> (PAIO_GENERAL::no_op));
    this->set_default_operation_context (static_cast<int> (PAIO_GENERAL::no_op));
}

// PaioInstance (fully) parameterized constructor.
PaioInstance::PaioInstance (std::shared_ptr<PaioStage> stage_ptr,
    const long& default_workflow_id,
    const int& default_operation_type,
    const int& default_operation_context) :
    InstanceInterface { std::move (stage_ptr),
        default_workflow_id,
        default_operation_type,
        default_operation_context }
{
    Logging::log_debug ("PaioInstance (full) parameterized constructor.");
}

// PaioInstance default destructor.
PaioInstance::~PaioInstance ()
{
    Logging::log_debug_explicit ("PaioInstance default destructor.");
}

// set_default_workflow_id call. Set new value for m_default_workflow_id.
void PaioInstance::set_default_workflow_id (const long& workflow_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_workflow_id (workflow_id);
}

// set_default_operation_type call. Set new value for m_default_operation_type.
void PaioInstance::set_default_operation_type (const int& operation_type)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_operation_type (operation_type);
}

// set_default_operation_context call. Set new value for m_default_operation_context.
void PaioInstance::set_default_operation_context (const int& operation_context)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_operation_context (operation_context);
}

// set_default_secondary_workflow_identifier call. Set new value for
// m_default_secondary_workflow_identifier.
void PaioInstance::set_default_secondary_workflow_identifier (
    const std::string& workflow_secondary_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_secondary_workflow_identifier (workflow_secondary_id);
}

// get_default_workflow_id call. Get value of m_default_workflow_id.
[[maybe_unused]] long PaioInstance::get_default_workflow_id ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    return InstanceInterface::get_default_workflow_id ();
}

// get_default_secondary_workflow_identifier call. Get value of
// m_default_secondary_workflow_identifier.
std::string PaioInstance::get_default_secondary_workflow_identifier ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    return InstanceInterface::get_default_secondary_workflow_identifier ();
}

// get_default_operation_type call. Get value of m_default_operation_type.
int PaioInstance::get_default_operation_type ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    return InstanceInterface::get_default_operation_type ();
}

// get_default_operation_context call. Get value of m_default_operation_context.
int PaioInstance::get_default_operation_context ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    return InstanceInterface::get_default_operation_context ();
}

// build_context_object call. Build Context object containing all classifiers to enforce request.
Context PaioInstance::build_context_object ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    // build Context object
    return this->build_context_object (this->m_default_workflow_id,
        this->m_default_operation_type,
        this->m_default_operation_context,
        1,
        1);
}

// build_context_object call. Build Context object containing all classifiers to enforce request.
Context PaioInstance::build_context_object (const long& workflow_id,
    const int& operation_type,
    const int& operation_context,
    const uint64_t& operation_size,
    const int& total_operations)
{
    // build Context object
    return Context { workflow_id,
        operation_type,
        operation_context,
        operation_size,
        total_operations };
}

// enforce call. Enforce I/O requests at the data plane stage.
void PaioInstance::enforce (const Context& context, Result& result)
{
    InstanceInterface::enforce (context, result);
}

// enforce call. Enforce I/O requests at the data plane stage.
void PaioInstance::enforce (const Context& context,
    const void* buffer,
    const size_t& size,
    Result& result)
{
    InstanceInterface::enforce (context, buffer, size, result);
}

// to_string call. Generate a string with the PaioInstance interface values.
std::string PaioInstance::to_string ()
{
    std::string message { "PaioInstance {" };
    {
        std::lock_guard<std::mutex> guard (this->m_lock);
        message.append (InstanceInterface::to_string ());
    }
    message.append ("}");
    return message;
}

} // namespace paio
