/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/stage/paio_stage.hpp>

namespace paio {

// PaioStage default constructor.
PaioStage::PaioStage () :
    m_core { std::make_shared<Core> () },
    m_stage_info { std::make_shared<StageInfo> (option_default_data_plane_stage_name ()) },
    m_agent { std::make_shared<Agent> (option_default_communication_type,
        this->m_core,
        this->m_ready,
        option_default_housekeeping_rules_file_path (),
        option_default_differentiation_rules_file_path (),
        option_default_enforcement_rules_file_path (),
        -1,
        this->m_stage_info,
        true) },
    m_connection_manager { this->m_agent, this->m_interrupted }
{
    // log message for printing the PaioStage properties
    std::string message { "PaioStage default constructor (" };
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_interrupted.use_count ())).append (", ");
    message.append (std::to_string (this->m_agent.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_info.use_count ())).append (")\n");
    Logging::log_debug (message);

    // log (debug) message for printing the connection manager properties
    Logging::log_debug (this->m_connection_manager.to_string ());
}

// PaioStage parameterized constructor.
PaioStage::PaioStage (const int& channels,
    const bool& default_object_creation,
    const std::string& stage_identifier) :
    m_core {
        std::make_shared<Core> (channels, option_create_default_channels, default_object_creation)
    },
    m_stage_info { std::make_shared<StageInfo> (stage_identifier) },
    m_agent { std::make_shared<Agent> (option_default_communication_type,
        this->m_core,
        this->m_ready,
        channels,
        this->m_stage_info) },
    m_connection_manager { this->m_agent, this->m_interrupted }
{
    // log message for printing the PaioStage properties
    std::string message { "PaioStage parameterized constructor (" };
    message.append (std::to_string (channels)).append (", ");
    message.append (default_object_creation ? "true" : "false").append (", ");
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_interrupted.use_count ())).append (", ");
    message.append (std::to_string (this->m_agent.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_info.use_count ())).append (")\n");
    Logging::log_debug (message);

    // log (debug) message for printing the connection manager properties
    Logging::log_debug (this->m_connection_manager.to_string ());
}

// PaioStage parameterized constructor.
PaioStage::PaioStage (const int& channels,
    const bool& default_object_creation,
    const std::string& stage_identifier,
    const std::string& housekeeping_rules_file_path,
    const std::string& differentiation_rules_file_path,
    const std::string& enforcement_rules_file_path,
    const bool& execute_on_receive) :
    m_core {
        std::make_shared<Core> (channels, option_create_default_channels, default_object_creation)
    },
    m_stage_info { std::make_shared<StageInfo> (stage_identifier) },
    m_agent { std::make_shared<Agent> (option_default_communication_type,
        this->m_core,
        this->m_ready,
        housekeeping_rules_file_path,
        differentiation_rules_file_path,
        enforcement_rules_file_path,
        channels,
        this->m_stage_info,
        execute_on_receive) },
    m_connection_manager { this->m_agent, this->m_interrupted }
{
    // log message for printing the PaioStage properties
    std::string message { "PaioStage parameterized constructor (" };
    message.append (std::to_string (channels)).append (", ");
    message.append (default_object_creation ? "true" : "false").append (", ");
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_interrupted.use_count ())).append (", ");
    message.append (std::to_string (this->m_agent.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_info.use_count ())).append (")\n");
    Logging::log_debug (message);

    // log (debug) message for printing the connection manager properties
    Logging::log_debug (this->m_connection_manager.to_string ());
}

// PaioStage parameterized constructor.
PaioStage::PaioStage (const int& channels,
    const bool& default_object_creation,
    const std::string& stage_identifier,
    const CommunicationType& connection_type,
    const std::string& address,
    const int& port) :
    m_core {
        std::make_shared<Core> (channels, option_create_default_channels, default_object_creation)
    },
    m_stage_info { std::make_shared<StageInfo> (stage_identifier) },
    m_agent { std::make_shared<Agent> (connection_type,
        this->m_core,
        this->m_ready,
        channels,
        this->m_stage_info) },
    m_connection_manager { { connection_type, address, port }, this->m_agent, this->m_interrupted }
{
    // log message for printing the PaioStage properties
    std::string message { "PaioStage parameterized constructor (" };
    message.append (std::to_string (channels)).append (", ");
    message.append (default_object_creation ? "true" : "false").append (", ");
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_interrupted.use_count ())).append (", ");
    message.append (std::to_string (this->m_agent.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_info.use_count ())).append (")\n");
    Logging::log_debug (message);

    // log (debug) message for printing the connection manager properties
    Logging::log_debug (this->m_connection_manager.to_string ());
}

// PaioStage default destructor.
PaioStage::~PaioStage ()
{
    std::string message { "PaioStage default destructor (" };
    message.append (std::to_string (this->m_core.use_count ())).append (", ");
    message.append (std::to_string (this->m_ready.use_count ())).append (", ");
    message.append (std::to_string (this->m_interrupted.use_count ())).append (", ");
    message.append (std::to_string (this->m_agent.use_count ())).append (", ");
    message.append (std::to_string (this->m_stage_info.use_count ())).append (")\n");
    Logging::log_debug_explicit (message);

    // disconnect from the control plane; mark connection with the control plane interrupted
    this->mark_connection_interrupted ();
}

// is_interrupted call. Verifies if the data plane execution was interrupted.
bool PaioStage::is_interrupted () const
{
    return this->m_interrupted->load ();
}

// is_ready call. Verifies if the data plane setup phase is made (i.e., if the data plane is ready
// to receive requests).
bool PaioStage::is_ready () const
{
    return this->m_ready->load ();
}

// mark_connection_interrupted call. Marks the data plane stage's execution as interrupted.
void PaioStage::mark_connection_interrupted ()
{
    this->m_interrupted->store (true);
}

// set_stage_description call. Update StageInfo's description.
void PaioStage::set_stage_description (const std::string& description)
{
    this->m_stage_info->set_description (description);
}

// get_stage_info call. Return a pointer to the StageInfo object.
std::shared_ptr<StageInfo> PaioStage::get_stage_info () const
{
    return this->m_stage_info;
}

// enforce_request call. Submit enforcement call to the data plane stage.
PStatus PaioStage::enforce_request (const Context& context,
    const void* buffer,
    const size_t& buffer_size,
    Result& result)
{
    PStatus status = PStatus::Error ();

    // verify if the data plane stage is ready to receive requests
    if (this->m_ready->load ()) {
        // enforce storage mechanisms over I/O request
        this->m_core->enforce_request (context, buffer, buffer_size, result);
        status = PStatus::Enforced ();
    }

    return status;
}

ConnectionManager* PaioStage::get_connection_manager ()
{
    return &(this->m_connection_manager);
}

Core* PaioStage::get_core ()
{
    return (this->m_core.get ());
}

// get_stage_info_name call. Return the StageInfo's name.
std::string PaioStage::get_stage_info_name () const
{
    return this->m_stage_info->get_name ();
}

// get_stage_info_env call. Return the StageInfo's env.
std::string PaioStage::get_stage_info_env () const
{
    return this->m_stage_info->get_env ();
}

// get_stage_info_pid call. Return the StageInfo's pid.
pid_t PaioStage::get_stage_info_pid () const
{
    return this->m_stage_info->get_pid ();
}

// get_stage_info_ppid call. Return the StageInfo's ppid.
pid_t PaioStage::get_stage_info_ppid () const
{
    return this->m_stage_info->get_ppid ();
}

// get_stage_info_hostname call. Return the StageInfo's hostname.
std::string PaioStage::get_stage_info_hostname () const
{
    return this->m_stage_info->get_hostname ();
}

// get_stage_info_login_name call. Return the StageInfo's login name.
std::string PaioStage::get_stage_info_login_name () const
{
    return this->m_stage_info->get_login_name ();
}

// stage_info_to_string call. Return the StageInfo's string representation.
std::string PaioStage::stage_info_to_string () const
{
    return this->m_stage_info->to_string ();
}

} // namespace paio
