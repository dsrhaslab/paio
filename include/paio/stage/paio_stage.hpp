/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_PAIO_STAGE_HPP
#define PAIO_PAIO_STAGE_HPP

#include <paio/core/agent.hpp>
#include <paio/core/core.hpp>
#include <paio/networking/connection_manager.hpp>
#include <paio/networking/southbound_connection_handler.hpp>
#include <paio/utils/logging.hpp>

using namespace paio::networking;

namespace paio {

/**
 * PaioStage class.
 * This is the main class of the Paio library. It provides all mechanisms (I/O differentiation and
 * enforcement) for system-designers to build programmable and fine-tuned data plane stages.
 * A PaioStage should be integrated within an I/O layer or simply exported with LD_PRELOAD (using
 * dedicated interfaces). It provides a set of constructors to create the Stage, and a couple of
 * methods to get information about it. The actual I/O enforcement needs to be done by an
 * InstanceInterface-based objects (PaioInstance, PosixLayer, etc.).
 * It holds seven main instance variables:
 *  - m_core: shared pointer to a core object, that contains the channels and housekeeping rules of
 *  the stage;
 *  - m_ready: shared pointer to an atomic boolean that marks the data plane stage ready to receive
 *  requests from the I/O layer;
 *  - m_shutdown: shared pointer to an atomic boolean that marks the stage interrupted, ceasing
 *  all enforcement activity;
 *  - m_stage_info: shared pointer to a StageInfo object that contains the stage's name, pid, ppid,
 *  hostname, etc.;
 *  - m_agent: shared pointer to an Agent object that bridges commands submitted from the control
 *  plane and from local rules;
 *  - m_connection_manager: ConnectionManager object that manages and establishes the connection
 *  between the stage and the control plane;
 *  - m_logging: Logging object that prints all log messages.
 * TODO:
 *  - remove get_connection_manager and get_core methods (temporary).
 */
class PaioStage {

    friend class InstanceInterface;
    friend class PosixLayerTest;
    friend class SouthboundInterfaceTest;

private:
    std::shared_ptr<Core> m_core { nullptr };
    std::shared_ptr<std::atomic<bool>> m_ready { std::make_shared<std::atomic<bool>> (false) };
    std::shared_ptr<StageInfo> m_stage_info { nullptr };
    std::shared_ptr<Agent> m_agent { nullptr };
    std::shared_ptr<std::atomic<bool>> m_shutdown { std::make_shared<std::atomic<bool>> (false) };
    ConnectionManager m_connection_manager {};
    Logging m_logging { option_default_debug_log };

    /**
     * get_stage_info: get information of the data plane stage in a StageInfo object.
     * @return Returns a StageInfo object with all stage information.
     */
    [[nodiscard]] std::shared_ptr<StageInfo> get_stage_info () const;

    /**
     * enforce_request: submit I/O request to the data plane stage to be enforced with the
     * respective storage mechanisms.
     * @param context Context object containing all necessary metadata/classifiers to enforce the
     * I/O request.
     * @param buffer Content to be enforced.
     * @param buffer_size Size of the passed data content.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     * @return Returns a PStatus object. It returns PStatus::Enforced if the request was enforced
     * (has passed throughout the enforcement mechanisms); it returns PStatus::Error otherwise.
     */
    PStatus enforce_request (const Context& context,
        const void* buffer,
        const size_t& buffer_size,
        Result& result);

    /**
     * shutdown_connection: Sets the m_shutdown shared pointer to true, which marks the interruption
     * of the data plane stage's execution.
     */
    void shutdown_connection ();

public:
    /**
     * PaioStage default constructor.
     **/
    PaioStage ();

    /**
     * PaioStage parameterized constructor. Initializes the stage elements accounting with the
     * explicit number of channel that it will handle the requests of all I/O workflows.
     * Initializes the ConnectionManager with default parameters.
     * @param channels Number of channels to be created with default options.
     * @param default_object_creation Boolean that defines if default enforcement-objects should be
     * created.
     * @param stage_name Defines the data plane stage identity.
     */
    PaioStage (const int& channels,
        const bool& default_object_creation,
        const std::string& stage_name);

    /**
     * PaioStage parameterized constructor. Initializes the stage elements accounting with the
     * explicit number of channel that it will handle the requests of all I/O workflows.
     * Initializes the ConnectionManager with default parameters. Agent will be initialized with
     * specific housekeeping, differentiation, and enforcement rules' files.
     * @param channels Number of channels to be created with default options.
     * @param default_object_creation Boolean that defines if default enforcement-objects should be
     * created.
     * @param stage_name Defines the data plane stage identity.
     * @param housekeeping_rules_file_path Absolute path to the default HousekeepingRule file.
     * @param differentiation_rules_file_path Absolute path to the default DifferentiationRule file.
     * @param enforcement_rules_file_path Absolute path to the default EnforcementRule file.
     * @param execute_on_receive Boolean that defines if HousekeepingRules should be enforced on
     * receive or stored in the HousekeepingRulesTable.
     */
    PaioStage (const int& channels,
        const bool& default_object_creation,
        const std::string& stage_name,
        const std::string& housekeeping_rules_file_path,
        const std::string& differentiation_rules_file_path,
        const std::string& enforcement_rules_file_path,
        const bool& execute_on_receive);

    /**
     * PaioStage parameterized constructor. Initializes the stage elements accounting with the
     * explicit number of channel that it will handle the requests of all I/O workflows.
     * Initializes the ConnectionManager with the respective parameters, namely the connection type,
     * the connection address and the port.
     * @param channels Number of channels to be created with default options.
     * @param default_object_creation Boolean that defines if default enforcement-objects should be
     * created.
     * @param stage_name Defines the data plane stage identity.
     * @param connection_type Defines the type of communication to be made with the control plane.
     * @param address Defines the unix/inet socket address for the data plane stage to connect.
     * @param port Defines the port that the data plane stage will handle control plane operations.
     * missing tests ...
     */
    PaioStage (const int& channels,
        const bool& default_object_creation,
        const std::string& stage_name,
        const CommunicationType& connection_type,
        const std::string& address,
        const int& port);

    /**
     * PaioStage default destructor.
     * On destroy, the PaioStage will disconnect from the SDS controller (if connected to one)
     * through the disconnect_from_controller method.
     */
    ~PaioStage ();

    /**
     * is_interrupted: Verify if the PaioStage's execution is interrupted.
     * @return Returns a const value of m_interrupted.
     */
    [[nodiscard]] bool is_interrupted () const;

    /**
     * is_ready: Verify if the PaioStage's setup is ready.
     * @return Returns a const value of the m_ready parameter.
     */
    [[nodiscard]] bool is_ready () const;

    /**
     * set_stage_description: set new description (in the StageInfo object) to the data plane stage.
     * @param description New description value to be added.
     */
    void set_stage_description (const std::string& description);

    /**
     * get_stage_info_name: get the name of the data plane stage.
     * @return Returns a const value of the m_stage_info.name parameter.
     */
    [[nodiscard]] std::string get_stage_info_name () const;

    /**
     * get_stage_info_opt: get the environment of the data plane stage.
     * @return Returns a const value of the m_stage_info.opt parameter.
     */
    [[nodiscard]] std::string get_stage_info_opt () const;

    /**
     * get_stage_info_pid: get the process identifier of the data plane stage.
     * @return Returns a const value of the m_stage_info.pid parameter.
     */
    [[nodiscard]] pid_t get_stage_info_pid () const;

    /**
     * get_stage_info_ppid: get the parent's process identifier of the data plane stage.
     * @return Returns a const value of the m_stage_info.ppid parameter.
     */
    [[nodiscard]] pid_t get_stage_info_ppid () const;

    /**
     * get_stage_info_hostname: get the hostname of the data plane stage.
     * @return Returns a const value of the m_stage_info.hostname parameter.
     */
    [[nodiscard]] std::string get_stage_info_hostname () const;

    /**
     * get_stage_info_user_name: get the login name of the data plane stage.
     * @return Returns a const value of the m_stage_info.login_name parameter.
     */
    [[nodiscard]] std::string get_stage_info_login_name () const;

    /**
     * stage_info_to_string: get the StageInfo object as a string.
     * @return Returns a const value of the m_stage_info.to_string() method.
     */
    [[nodiscard]] std::string stage_info_to_string () const;

    // FIXME: Needing refactor or cleanup -@ricardomacedo at 9/27/2022, 2:53:29 PM
    // remove get_connection_manager and get_core; only used in paio_southbound_interface_test.
    ConnectionManager* get_connection_manager ();
    Core* get_core ();
};
} // namespace paio

#endif // PAIO_PAIO_STAGE_HPP
