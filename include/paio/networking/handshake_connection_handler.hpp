/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_HANDSHAKE_CONNECTION_HANDLER_H
#define PAIO_HANDSHAKE_CONNECTION_HANDLER_H

#include <paio/networking/connection_handler.hpp>

namespace paio::networking {

/**
 * HandshakeConnectionHandler class.
 * This class implements the ConnectionHandler interface, and is used to perform the first
 * interaction with the control plane. Specifically, it is used to establish the handshake between
 * the stage and control plane, by connecting to a standard address (and port, in the case of TCP/IP
 * connections).
 */
class HandshakeConnectionHandler : public ConnectionHandler {

private:
    std::mutex m_socket_read_lock;
    std::mutex m_socket_write_lock;
    std::mutex m_southbound_info_lock;
    std::string m_southbound_socket_name;
    int m_southbound_socket_port { -1 };

    /**
     * set_southbound_socket_info: Set information regarding the connection that must be performed
     * for the SouthboundConnectionHandler. The method sets the socket name and port.
     * This method is thread-safe.
     * @param handshake_object StageHandshakeRaw object that contains the information regarding the
     * connection to be performed with the SouthboundConnectionHandler.
     */
    void set_southbound_socket_info (const StageHandshakeRaw& handshake_object);

    /**
     * read_control_operation_from_socket: read ControlOperation object from socket (which is
     * connected to a SDS control plane).
     * The method is thread-safe, i.e., ensures that no other thread is reading from the socket.
     * @param operation Reference to the ControlOperation object.
     * @return If the operation is successful, returns the number of bytes read from the socket
     * (greater or equal to zero); otherwise returns -1.
     * @throws std::runtime_error if the socket is not connected.
     */
    ssize_t read_control_operation_from_socket (ControlOperation* operation) override;

    /**
     * handle_control_operation: After the connection is established between the data plane stage
     * and the control plane, this function receives rules from the controller and applies them to
     * the current data plane stage. In the HandshakeConnectionHandler, rules can only be of type
     * stage_handshake; otherwise it throws a runtime_error exception.
     * @param operation ControlOperation structure that contains the metadata of received rules.
     * @param debug If true, prints the received rules through the logging library.
     * @return Returns the number of bytes written to the control plane. If <= 0, then the
     * operation was not achieved.
     * @throws std::logic_error if the operation is not of type stage_handshake.
     */
    ssize_t handle_control_operation (const ControlOperation& operation,
        const bool& debug) override;

    /**
     * stage_handshake: Sends the stage's identifiers (StageInfo object) to the control plane, so
     * it can identify and enforce the right rules over different stages, distributed throughout the
     * infrastructure and I/O layers. The method performs two phases. First it submits the stage's
     * identifiers, and this is protected with the m_socket_write_lock mutex. Then, it receives the
     * new name/address and port through where the SouthboundConnectionHandler should connect, and
     * its protected with the m_socket_read_lock mutex.
     * @return If the operation is successful, returns the number of bytes read from the socket.
     */
    ssize_t stage_handshake ();

public:
    /**
     * HandshakeConnectionHandler default constructor.
     */
    HandshakeConnectionHandler ();

    /**
     * HandshakeConnectionHandler fully parameterized constructor.
     * @param connection_options Defines the main options to be used to establish the connection
     * between the data plane stage and the SDS control plane.
     * @param agent_ptr Shared pointer to the Agent object.
     * @param interrupted Shared pointer to the atomic boolean that indicates if the connection is
     * interrupted.
     */
    HandshakeConnectionHandler (const ConnectionOptions& connection_options,
        std::shared_ptr<Agent> agent_ptr,
        std::shared_ptr<std::atomic<bool>> interrupted);

    /**
     * HandshakeConnectionHandler parameterized constructor.
     * @param agent_ptr Shared pointer to the Agent object.
     * @param interrupted Shared pointer to the atomic boolean that indicates if the connection is
     * interrupted.
     */
    HandshakeConnectionHandler (std::shared_ptr<Agent> agent_ptr,
        std::shared_ptr<std::atomic<bool>> interrupted);

    /**
     * HandshakeConnectionHandler default destructor.
     */
    ~HandshakeConnectionHandler () override;

    /**
     * listen: Listen for incoming ControlOperations from the control plane. This method only
     * receives a single control operation from the socket, and should be of type stage_handshake.
     * After reading the message from socket, it invokes the handle_control_operation that will
     * apply the corresponding operation.
     * @param debug If true, prints the received rules through the logging library.
     * @throws std::runtime_error if it failed to receive the control operation from the socket.
     */
    void listen (const bool& debug) override;

    /**
     * get_southbound_socket_name: get the name/address of the socket where the
     * SouthboundConnectionHandler should connect.
     * This method is thread-safe.
     * @return Returns a string with the name/address of the socket.
     */
    [[nodiscard]] std::string get_southbound_socket_name ();

    /**
     * get_southbound_socket_port: get the port of the socket where the
     * SouthboundConnectionHandler should connect.
     * This method is thread-safe.
     * @return Returns an integer with the port of the socket.
     */
    [[nodiscard]] int get_southbound_socket_port ();
};

} // namespace paio::networking
#endif // PAIO_HANDSHAKE_CONNECTION_HANDLER_H
