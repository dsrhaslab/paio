/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/core/stage_info.hpp>

namespace paio::core {

// StageInfo default constructor.
StageInfo::StageInfo () = default;

// StageInfo (explicit) parameterized constructor.
StageInfo::StageInfo (std::string stage_name) : m_name { std::move (stage_name) }
{ }

// StageInfo copy constructor.
StageInfo::StageInfo (const StageInfo& info) :
    m_name { info.m_name },
    m_env { info.m_env },
    m_description { info.m_description },
    m_pid { info.m_pid },
    m_ppid { info.m_ppid },
    m_hostname { info.m_hostname },
    m_login_name { info.m_login_name }
{ }

// StageInfo default destructor.
StageInfo::~StageInfo () = default;

// set_env call. Set data plane stage environment variable.
std::string StageInfo::set_name () const
{
    // get environment variable for data plane stage
    auto name_ptr = std::getenv (option_environment_variable_name ().c_str ());

    if (name_ptr != nullptr) {
        // log message
        std::string log_message { "Value of `" };
        log_message.append (option_environment_variable_name ()).append ("` env is `");
        log_message.append (name_ptr).append ("`\n");

        Logging::log_debug (log_message);

        return { name_ptr };
    } else {
        // log message
        Logging::log_debug (
            "Inaccessible environment variable (using default data plane stage name).");
        return { option_default_data_plane_stage_name () };
    }
}

// set_env call. Set data plane stage environment variable.
std::string StageInfo::set_env () const
{
    // get environment variable for data plane stage
    auto env_ptr = std::getenv (option_environment_variable_env ().c_str ());

    if (env_ptr != nullptr) {
        // log message
        std::string log_message { "Value of `" };
        log_message.append (option_environment_variable_env ()).append ("` env is `");
        log_message.append (env_ptr).append ("`\n");

        Logging::log_debug (log_message);

        return { env_ptr };
    } else {
        // log message
        Logging::log_debug ("Inaccessible environment variable.");
        return "";
    }
}

// set_hostname call. Set data plane stage hostname.
std::string StageInfo::set_hostname () const
{
    // allocate memory for hostname
    char hostname[HOST_NAME_MAX];
    // get hostname through gethostname call
    auto return_value = ::gethostname (hostname, HOST_NAME_MAX);

    // validate return value
    if (return_value == -1) {
        Logging::log_error ("Error while getting hostname: " + std::string (strerror (errno)));
        return "";
    }

    return { hostname };
}

// set_login_name call. Set data plane stage login name.
std::string StageInfo::set_login_name () const
{
    // allocate memory for login name
    char username[LOGIN_NAME_MAX];
    // get login name through getlogin_r call
    auto return_value = ::getlogin_r (username, LOGIN_NAME_MAX);

    // validate return value
    if (return_value == -1) {
        Logging::log_error ("Error while getting login name: " + std::string (strerror (errno)));
        return "";
    }

    return { username };
}

// get_name call. Return a copy of the m_name value.
std::string StageInfo::get_name () const
{
    return this->m_name;
}

// get_env call. Return a copy of the m_env value.
std::string StageInfo::get_env () const
{
    return this->m_env;
}

// get_description call. Return a copy of the m_description value.
std::string StageInfo::get_description () const
{
    return this->m_description;
}

// set_description call. Update m_description instance with new value.
void StageInfo::set_description (const std::string& description)
{
    this->m_description = description;
}

// get_pid call. Return a copy of the m_pid value.
pid_t StageInfo::get_pid () const
{
    return this->m_pid;
}

// get_ppid call. Return a copy of the m_ppid value.
pid_t StageInfo::get_ppid () const
{
    return this->m_ppid;
}

// get_hostname call. Return a copy of the m_hostname value.
std::string StageInfo::get_hostname () const
{
    return this->m_hostname;
}

// get_login_name call. Return a copy of the m_login_name value.
std::string StageInfo::get_login_name () const
{
    return this->m_login_name;
}

// serialize call. Fill StageInfoRaw object with the respective StageInfo variables.
void StageInfo::serialize (StageInfoRaw& handshake_obj)
{
    // validate StageInfo's name size
    if (this->m_name.size () > stage_name_max_size) {
        throw std::out_of_range (
            "StageInfo's name is larger than " + std::to_string (stage_name_max_size) + " bytes.");
    }
    // copy stage name to handshake_obj
    std::strcpy (handshake_obj.m_stage_name, this->m_name.c_str ());

    // validate StageInfo's env size
    if (!this->m_env.empty ()) {
        if (this->m_env.size () > stage_env_max_size) {
            throw std::out_of_range ("StageInfo's env is larger than "
                + std::to_string (stage_env_max_size) + " bytes.");
        } else {
            // copy StageInfo's env to handshake_obj
            std::strcpy (handshake_obj.m_stage_env, this->m_env.c_str ());
        }
    } else {
        std::strcpy (handshake_obj.m_stage_env, "");
    }

    // copy StageInfo's pid info to handshake_obj
    handshake_obj.m_pid = this->m_pid;
    handshake_obj.m_ppid = this->m_ppid;

    // validate StageInfo's hostname size
    if (!this->m_hostname.empty ()) {
        if (this->m_hostname.size () > HOST_NAME_MAX) {
            throw std::out_of_range ("StageInfo's hostname is larger than "
                + std::to_string (HOST_NAME_MAX) + " bytes.");
        } else {
            // copy StageInfo's hostname to handshake_obj
            std::strcpy (handshake_obj.m_stage_hostname, this->m_hostname.c_str ());
        }
    } else {
        std::strcpy (handshake_obj.m_stage_hostname, "");
    }

    // validate StageInfo's login_name size
    if (!this->m_login_name.empty ()) {
        if (this->m_login_name.size () > LOGIN_NAME_MAX) {
            throw std::out_of_range ("StageInfo's login name is larger than "
                + std::to_string (LOGIN_NAME_MAX) + " bytes.");
        } else {
            // copy StageInfo's login name to handshake_obj
            std::strcpy (handshake_obj.m_stage_login_name, this->m_login_name.c_str ());
        }
    } else {
        std::strcpy (handshake_obj.m_stage_login_name, "");
    }
}

// to_string call. Return StageInfo's variables in string-based format.
std::string StageInfo::to_string ()
{
    std::string message { "StageInfo {" };
    message.append (this->m_name).append (", ");
    message.append ((!this->m_env.empty ()) ? this->m_env : "<empty env>").append (", ");
    message.append ((!this->m_description.empty ()) ? this->m_description : "<empty description>")
        .append (", ");
    message.append (std::to_string (this->m_pid)).append (", ");
    message.append (std::to_string (this->m_ppid)).append (", ");
    message.append ((!this->m_hostname.empty ()) ? this->m_hostname : "<empty hostname>")
        .append (", ");
    message.append ((!this->m_login_name.empty ()) ? this->m_login_name : "<empty login_name>")
        .append ("}");

    return message;
}

}; // namespace paio::core
