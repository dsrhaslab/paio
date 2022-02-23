/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CONNECTION_MANAGER_HPP
#define PAIO_CONNECTION_MANAGER_HPP

#include <arpa/inet.h>
#include <atomic>
#include <cstdio>
#include <netinet/in.h>
#include <paio/core/interface_definitions.hpp>
#include <paio/networking/connection_options.hpp>
#include <paio/networking/handshake_connection_handler.hpp>
#include <paio/networking/southbound_connection_handler.hpp>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

namespace paio::networking {

/**
 * ConnectionManager class.
 * This class provides the main building blocks to establish the connection with the control plane.
 * The connection is established in a twofold. First, the connection is established through a
 * HandshakeConnectionHandler, which is responsible for the handshake process. After this, the
 * data plane stage will have access to the new address and port to where the data plane stage
 * should connect next. Then, a SouthboundConnectionHandler is created to handle the main control
 * operations submitted from the control plane, including HousekeepingRules, DifferentiationRules,
 * EnforcementRules, and collection of statistics.
 * The method provides the following instance variables:
 *  - m_socket: shared pointer to the file descriptor that corresponds to the socket;
 *  - m_connection_options: provides the ConnectionOptions object that provides the main connection
 *  options in both connection phases;
 *  - m_agent_ptr: shared pointer to the Agent object;
 *  - m_connection_interrupted: shared pointer to a boolean that indicates if the connection is
 *  interrupted or not;
 *  - m_handshake_connection_handler: unique pointer to the HandshakeConnectionHandler object;
 *  - m_southbound_connection_handler: unique pointer to the SouthboundConnectionHandler object.
 * TODO:
 *  - add further testing;
 *  - update disconnect_from_control_plane
 */
class ConnectionManager {

    friend class SouthboundInterfaceTest;

private:
    std::shared_ptr<std::atomic<int>> m_socket { std::make_shared<std::atomic<int>> (-1) };
    ConnectionOptions m_connection_options {};
    std::shared_ptr<Agent> m_agent_ptr { nullptr };
    std::shared_ptr<std::atomic<bool>> m_connection_interrupted { nullptr };
    std::unique_ptr<HandshakeConnectionHandler> m_handshake_connection_handler { nullptr };
    std::unique_ptr<SouthboundConnectionHandler> m_southbound_connection_handler { nullptr };
    std::thread m_connection_thread;

    /**
     * disconnect_from_control_plane (). Close communication channel between the data plane stage
     * and the SDS control plane. If the data plane is operating without support of an external
     * controller (i.e., CommunicationType::none), it does not need to join the communication
     * thread.
     */
    void disconnect_from_control_plane ();

    /**
     * spawn_handshake_listening_thread: Spawn a dedicated thread to handle the first communication
     * phase between the data plane stage and the control plane. The method will receive control
     * operations to establish the communication handshake between both parties.
     * @param debug Boolean that indicates if debug messages should be printed.
     */
    void spawn_handshake_listening_thread (const bool& debug);

    /**
     * spawn_southbound_listening_thread: Spawn a dedicated thread to handle the second
     * communication  phase between the data plane stage and the control plane. The method will
     * receive control operations to manage and monitor the data plane stage.
     * @param debug Boolean that indicates if debug messages should be printed.
     */
    void spawn_southbound_listening_thread (const bool& debug);

    /**
     * connect: complementary method that (1) creates/establishes a connection between the data
     * plane stage and the SDS control plane; and (2) spawns a dedicated thread that receives
     * control operations (from the SDS control plane) to adjust the data plane internals to comply
     * with application requirements.
     * @param connection_options Connection options to be used to establish the connection,
     */
    void connect (const ConnectionOptions& connection_options);

public:
    /**
     * ConnectionManager default constructor.
     * Initializes parameters with default configuration values defined in the Options header.
     * It connects to the control plane using default connection options.
     */
    ConnectionManager ();

    /**
     * ConnectionManager parameterized constructor. It connects to the control plane using
     * connection_options properties.
     * @param connection_options Defines the main options to be used to establish the connection
     * between the data plane stage and the SDS control plane.
     * @param agent_ptr Pointer to the Agent object.
     * @param interrupted Shared pointer to the atomic boolean that indicates if the connection is
     * interrupted.
     */
    ConnectionManager (const ConnectionOptions& connection_options,
        std::shared_ptr<Agent> agent_ptr,
        std::shared_ptr<std::atomic<bool>> interrupted);

    /**
     * ConnectionManager parameterized constructor. It connects to the control plane using default
     * connection options.
     * @param agent_ptr Pointer to the Agent object.
     * @param interrupted Shared pointer to the atomic boolean that indicates if the connection is
     * interrupted.
     */
    ConnectionManager (std::shared_ptr<Agent> agent_ptr,
        std::shared_ptr<std::atomic<bool>> interrupted);

    /**
     * ConnectionManager default destructor. It disconnects from the control plane.
     */
    ~ConnectionManager ();

    /**
     * is_connection_interrupted: Verify if the connection between the data and control plane is
     * established or was interrupted. This is useful for finishing the endless loop launched by
     * the listen_socket call, thus terminating the communication between planes of functionality.
     * @return Returns a const value of the connection_interrupted_ variable.
     */
    [[nodiscard]] bool is_connection_interrupted () const;

    /**
     * set_connection_interrupted: Atomically defines a new value for the shared
     * connection_interrupted parameter. This will allow to terminate the connection between the
     * PAIO data plane stage and the SDS controller.
     * @param value New value to be updated.
     */
    void set_connection_interrupted (bool value);

    /**
     * get_socket_identifier: get the socket identifier (file descriptor) of the socket that
     * establishes the connection between the data plane and the control plane.
     * @return Return a copy of the value pointed by the m_socket (shared pointer) parameter.
     */
    [[nodiscard]] int get_socket_identifier () const;

    /**
     * to_string: converts the ConnectionManager object to a string.
     * @return Returns a string with the ConnectionManager object's information.
     */
    [[nodiscard]] std::string to_string () const;
};
} // namespace paio::networking

#endif // PAIO_CONNECTION_MANAGER_HPP
