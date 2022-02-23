/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/interface/lsm_kvs_layer.hpp>

namespace paio {

// LsmKvsLayer default constructor.
LsmKvsLayer::LsmKvsLayer () : InstanceInterface {}
{
    Logging::log_debug ("LsmKvsLayer instance constructor.");
}

// LsmKvsLayer parameterized constructor.
LsmKvsLayer::LsmKvsLayer (std::shared_ptr<PaioStage> stage_ptr) :
    InstanceInterface { std::move (stage_ptr) }
{
    Logging::log_debug ("LsmKvsLayer (explicit) parameterized instance constructor.");
    this->set_default_operation_type (static_cast<int> (KVS::no_op));
    this->set_default_operation_context (static_cast<int> (KVS::no_op));
}

// LsmKvsLayer parameterized constructor.
LsmKvsLayer::LsmKvsLayer (std::shared_ptr<PaioStage> stage_ptr, const long& default_workflow_id) :
    InstanceInterface { std::move (stage_ptr) }
{
    Logging::log_info ("LsmKvsLayer parameterized instance constructor.");
    this->set_default_workflow_id (default_workflow_id);
    this->set_default_operation_type (static_cast<int> (KVS::no_op));
    this->set_default_operation_context (static_cast<int> (KVS::no_op));
}

// LsmKvsLayer parameterized constructor.
LsmKvsLayer::LsmKvsLayer (std::shared_ptr<PaioStage> stage_ptr,
    const long& default_workflow_id,
    const int& default_operation_type,
    const int& default_operation_context) :
    InstanceInterface (std::move (stage_ptr),
        default_workflow_id,
        default_operation_type,
        default_operation_context)
{
    Logging::log_info ("LsmKvsLayer (full) parameterized instance constructor.");
}

// LsmKvsLayer default destructor.
LsmKvsLayer::~LsmKvsLayer ()
{
    Logging::log_debug_explicit ("LsmKvsLayer destructor.");
}

// enforce call. Enforce I/O requests at the data plane stage.
void LsmKvsLayer::enforce ([[maybe_unused]] const Context& context, [[maybe_unused]] Result& result)
{
    throw std::runtime_error ("LsmKvsLayer::enforce() not implemented.");
}

// enforce call. Enforce I/O requests at the data plane stage.
void LsmKvsLayer::enforce ([[maybe_unused]] const Context& context,
    [[maybe_unused]] const void* buffer,
    [[maybe_unused]] const size_t& size,
    [[maybe_unused]] Result& result)
{
    throw std::runtime_error ("LsmKvsLayer::enforce() not implemented.");
}

// set_default_workflow_id call. Set new value for m_default_workflow_id.
void LsmKvsLayer::set_default_workflow_id (const long& workflow_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_workflow_id (workflow_id);
}

// set_default_operation_type call. Set new value for m_default_operation_type.
void LsmKvsLayer::set_default_operation_type (const int& operation_type)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_operation_type (operation_type);
}

// set_default_operation_context call. Set new value for m_default_operation_context.
void LsmKvsLayer::set_default_operation_context (const int& operation_context)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_operation_context (operation_context);
}

// set_default_secondary_workflow_identifier call. Set new value for
// m_default_secondary_workflow_identifier.
void LsmKvsLayer::set_default_secondary_workflow_identifier (
    const std::string& workflow_secondary_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_secondary_workflow_identifier (workflow_secondary_id);
}

// set_io_transformation call. Enable/disable m_has_io_transformation flag.
void LsmKvsLayer::set_io_transformation (const bool& value)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    this->m_has_io_transformation = value;
}

// build_context_object call. Build Context containing default I/O classifiers to enforce request.
Context LsmKvsLayer::build_context_object ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    // build Context object
    return this->build_context_object (this->m_default_workflow_id,
        this->m_default_operation_type,
        this->m_default_operation_context,
        1,
        1);
}

// build_context_object call. Build Context containing all I/O classifiers to enforce request.
Context LsmKvsLayer::build_context_object (const long& workflow_id,
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

// put call. (...)
PStatus LsmKvsLayer::put ([[maybe_unused]] const void* key, [[maybe_unused]] const void* value)
{
    return PStatus::NotSupported ();
}

// get call. (...)
PStatus LsmKvsLayer::get ([[maybe_unused]] const void* key, [[maybe_unused]] Result& result)
{
    return PStatus::NotSupported ();
}

// delete_ call. (...)
PStatus LsmKvsLayer::delete_ ([[maybe_unused]] const void* key)
{
    return PStatus::NotSupported ();
}

// to_string call. Generate a string with the LsmKvsLayer interface values.
std::string LsmKvsLayer::to_string ()
{
    std::string message { "LsmKvsLayer {" };
    {
        std::lock_guard<std::mutex> guard (this->m_lock);
        message.append (InstanceInterface::to_string ()).append ("}");
    }
    return message;
}

} // namespace paio
