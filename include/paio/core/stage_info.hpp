/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_STAGE_IDENTIFIER_H
#define PAIO_STAGE_IDENTIFIER_H

#include <climits>
#include <paio/core/interface_definitions.hpp>
#include <paio/options/options.hpp>
#include <string>
#include <unistd.h>

using namespace paio::options;
using namespace paio::utils;

namespace paio::core {

/**
 * StageInfo class.
 * The StageInfo class characterizes a data plane stage, providing its name, description, and the
 * process identifier it is running, as well as the parent process.
 * This class is useful for performing the handshake between the data plane stage and the control
 * plane, as there can be different data plane stages executing simultaneously at the same node,
 * or across nodes. The handshake is established from the ConnectionInterface of the data plane.
 * The StageInfo class provides to serialization methods that encapsulate the StageInfo variables
 * into a StageSimplifiedHandshakeRaw or a StageDetailedHandshakeRaw object.
 * Currently, the StageInfo class provides the following variables:
 * - m_name: defines the name of the data plane stage.
 * - m_opt: defines the value of an additional option registered for the data plane stage.
 * - m_description: defines the description of the data plane stage, including what is the stage
 * used for, and more.
 * - m_pid: defines the process identifier where the data plane stage is executing.
 * - m_ppid: defines the process identifier of the parent process where the data plane stage is
 * executing.
 * - m_hostname: defines the hostname where the data plane stage is executing.
 * - m_login_name: defines the login name of the user that is running the data plane stage.
 */
class StageInfo {

private:
    const std::string m_name { this->set_name () };
    const std::string m_opt { this->set_opt () };
    std::string m_description {};
    const pid_t m_pid { ::getpid () };
    const pid_t m_ppid { ::getppid () };
    const std::string m_hostname { this->set_hostname () };
    const std::string m_login_name { this->set_login_name () };

    /**
     * set_name: set data plane stage name environment variable. This method invokes a getenv call
     * over the option_environment_variable_name value.
     * @return If the environment variable exists, returns its value; otherwise, returns the
     * option_default_data_plane_stage_name value
     */
    [[nodiscard]] std::string set_name () const;

    /**
     * set_opt: set data plane stage environment variable. This method invokes a getenv call over
     * the option_environment_variable_opt value.
     * @return If the environment variable exists, returns its value; otherwise, returns an empty
     * string.
     */
    [[nodiscard]] std::string set_opt () const;

    /**
     * set_hostname: set data plane stage hostname. This method allocates a buffer for setting the
     * hostname value using the HOST_NAME_MAX macro, and invokes a gethostname call.
     * @return If successful, returns the hostname value of the current machine; otherwise, returns
     * and empty string.
     */
    [[nodiscard]] std::string set_hostname () const;

    /**
     * set_login_name: set data plane stage login name. This method allocates a buffer for setting
     * the login name value using the LOGIN_NAME_MAX macro, and invokes a getlogin_r call.
     * @return If successful, returns the login name value of the current user; otherwise, returns
     * an empty string.
     */
    [[nodiscard]] std::string set_login_name () const;

public:
    /**
     * StageInfo default constructor.
     */
    StageInfo ();

    /**
     * StageInfo (explicit) parameterized constructor.
     * @param stage_name Defines the name of the data plane stage.
     */
    explicit StageInfo (std::string stage_name);

    /**
     * StageInfo copy constructor.
     * @param info StageInfo object to be copied.
     */
    StageInfo (const StageInfo& info);

    /**
     * StageInfo default destructor.
     */
    ~StageInfo ();

    /**
     * get_name: get the data plane stage's name.
     * @return Returns a copy of the m_name value.
     */
    [[nodiscard]] std::string get_name () const;

    /**
     * get_env: get data plane stage optional variable.
     * @return Returns a copy of the m_opt value.
     */
    [[nodiscard]] std::string get_opt () const;

    /**
     * get_description: get the data plane stage's description.
     * @return Returns a copy of the m_description value.
     */
    [[nodiscard]] std::string get_description () const;

    /**
     * set_description: Update StageInfo's m_description with new value.
     * @param description New value to be set on m_description.
     */
    void set_description (const std::string& description);

    /**
     * get_pid: get the data plane stage's process identifier where it is executing.
     * @return Returns a copy of the m_pid value.
     */
    [[nodiscard]] pid_t get_pid () const;

    /**
     * get_ppid: get the data plane stage's parent's process identifier.
     * @return Returns a copy of the m_ppid value.
     */
    [[nodiscard]] pid_t get_ppid () const;

    /**
     * get_hostname: get the data plane stage's hostname.
     * @return Returns a copy of the m_hostname value.
     */
    [[nodiscard]] std::string get_hostname () const;

    /**
     * get_login_name: get the data plane stage's login name.
     * @return Returns a copy of the m_login_name value.
     */
    [[nodiscard]] std::string get_login_name () const;

    /**
     * serialize: serialize the StageInfo's variables into a StageInfoRaw object that will be
     * submitted through the network to the SDS control plane.
     * This method will serialize all StageInfo parameters excepting the m_description.
     * @param handshake_obj Empty StageInfoRaw object to be filled.
     */
    void serialize (StageInfoRaw& handshake_obj);

    /**
     * to_string: convert StageInfo's variables in string-based format.
     * @return Returns a string with all StageInfo's variables.
     */
    std::string to_string ();
};

} // namespace paio::core

#endif // PAIO_STAGE_IDENTIFIER_H
