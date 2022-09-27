/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CONNECTION_HANDLER_HPP
#define PAIO_CONNECTION_HANDLER_HPP

#include <arpa/inet.h>
#include <dlfcn.h>
#include <memory>
#include <netinet/in.h>
#include <paio/core/agent.hpp>
#include <paio/networking/connection_options.hpp>
#include <paio/options/libc_headers.hpp>
#include <sys/un.h>
#include <utility>

namespace paio::networking {

/**
 * ConnectionHandlerType enum class.
 * Enum class that defines the type of connection handlers permitted in the data plane stage.
 *  - handshake_handler: refers to HandshakeConnectionHandler classes;
 *  - southbound_handler: refer to SouthboundConnectionHandler classes.
 */
enum class ConnectionHandlerType { handshake_handler = 1, southbound_handler = 2, no_op = 0 };

/**
 * ConnectionHandler class.
 * Abstract class that defines the interface for creating different handlers to connect with the
 * control plane. Each handler will be responsible for interacting with the control plane in a very
 * specific way. Currently, it provides the following variables and virtual functions:
 *  - m_socket: shared pointer to the socket used to connect with the control plane.
 *  - m_connection_options: defines the connection options to be used;
 *  - m_inet_socket: provides a sockaddr_in struct to perform INET (TCP/IO) based connections.
 *  - m_unix_socket: provides a sockaddr_un struct to perform UNIX (UDS) based connections.
 *  - m_agent_ptr: shared pointer to the Agent class.
 *  - m_connection_interrupted: shared pointer to an atomic boolean that indicates if the connection
 *  is interrupted.
 *  - virtual ssize_t read_control_operation_from_socket.
 *  - virtual ssize_t handle_control_operation.
 *  - virtual ~ConnectionHandler.
 *  - virtual void listen.
 */
class ConnectionHandler {

protected:
    std::shared_ptr<std::atomic<int>> m_socket { std::make_shared<std::atomic<int>> (-1) };
    ConnectionOptions m_connection_options {};
    struct sockaddr_in m_inet_socket { };
    struct sockaddr_un m_unix_socket { };
    std::shared_ptr<Agent> m_agent_ptr { nullptr };
    void* m_dl_ptr { RTLD_NEXT };

    /**
     * socket_read: read bytes from socket.
     * Missing description ...
     * @param buf buffer to store the read bytes.
     * @param count number of bytes to read.
     * @return number of bytes read.
     */
    ssize_t socket_read (void* buf, size_t count)
    {
        // validate if socket is valid
        if (this->m_socket == nullptr || this->m_socket->load () == -1) {
            return -1;
        }

        // validate if ld_preload is enabled and read from socket accordingly
        if (option_default_ld_preload_enabled) {
            // submit read operation through lib_ptr
            if (this->m_dl_ptr != nullptr) {
                return ((libc_read_t)::dlsym (this->m_dl_ptr,
                    "read")) (this->m_socket->load (), buf, count);
            } else {
                // submit read operation through RTLD_NEXT (rigid)
                return (
                    (libc_read_t)::dlsym (RTLD_NEXT, "read")) (this->m_socket->load (), buf, count);
            }
        } else {
            // submit read operation through libc
            return ::read (this->m_socket->load (), buf, count);
        }
    }

    /**
     * socket_write: write bytes to socket.
     * Missing description ...
     * @param buf buffer to write.
     * @param count number of bytes to write.
     * @return number of bytes written.
     */
    ssize_t socket_write (const void* buf, size_t count)
    {
        // validate if socket is valid
        if (this->m_socket == nullptr || this->m_socket->load () == -1) {
            return -1;
        }

        // validate if ld_preload is enabled and write to socket accordingly
        if (option_default_ld_preload_enabled) {
            // submit write operation through lib_ptr
            if (this->m_dl_ptr != nullptr) {
                return ((libc_write_t)::dlsym (this->m_dl_ptr,
                    "write")) (this->m_socket->load (), buf, count);
            } else {
                // submit write operation through RTLD_NEXT (rigid)
                return ((libc_write_t)::dlsym (RTLD_NEXT,
                    "write")) (this->m_socket->load (), buf, count);
            }
        } else {
            // submit write operation through libc
            return ::write (this->m_socket->load (), buf, count);
        }
    }

    /**
     * read_control_operation_from_socket: read ControlOperation object from socket (which is
     * connected to a SDS control plane).
     * The method is thread-safe, i.e., ensures that no other thread is reading from the socket.
     * @param operation
     * @return If the operation is successful, returns the number of bytes read from the socket
     * (greater or equal to zero); otherwise returns -1.
     */
    virtual ssize_t read_control_operation_from_socket (ControlOperation* operation) = 0;

    /**
     * handle_control_operation: After the connection is established between the data plane stage
     * and the control plane, this function receives rules from the controller and applies them to
     * the current data plane stage.
     * Rules can be of type CREATE_HSK_RULE, CREATE_DIF_RULE, CREATE_ENF_RULE, COLLECT_STATS,
     * REMOVE_RULE, and EXEC_HSK_RULES.
     * @param operation ControlOperation structure that contains the metadata of received rules.
     * @return Returns the number of bytes written to the control plane. If <= 0, then the
     * operation was not achieved.
     */
    virtual ssize_t handle_control_operation (const ControlOperation& operation, const bool& debug)
        = 0;

    /**
     * convert_operation_type: converts a ControlPlaneOperationType to its string representation.
     * @param operation_type ControlPlaneOperationType to be converted.
     * @return String representation of the operation type.
     */
    [[nodiscard]] std::string convert_operation_type (
        const ControlPlaneOperationType& operation_type) const
    {
        switch (operation_type) {
            case ControlPlaneOperationType::stage_handshake:
                return "stage-handshake";

            case ControlPlaneOperationType::mark_stage_ready:
                return "stage-ready";

            case ControlPlaneOperationType::collect_stats:
                return "collect-stats";

            case ControlPlaneOperationType::collect_detailed_stats:
                return "collect-detailed-stats";

            case ControlPlaneOperationType::create_hsk_rule:
                return "create-housekeeping-rule";

            case ControlPlaneOperationType::create_dif_rule:
                return "create-differentiation-rule";

            case ControlPlaneOperationType::create_enf_rule:
                return "create-enforcement-rule";

            case ControlPlaneOperationType::exec_hsk_rules:
                return "execute-housekeeping-rules";

            case ControlPlaneOperationType::remove_rule:
                return "remove-rule";

            default:
                throw std::logic_error ("ConnectionHandler: unrecognized operation type ("
                    + std::to_string (static_cast<int> (operation_type)) + ")");
        }
    }

    /**
     * convert_operation_subtype: converts a ControlPlaneOperationSubtype to its string
     * representation.
     * @param operation_type ControlPlaneOperationType to be used to identify the subtype.
     * @param operation_subtype ControlPlaneOperationSubtype to be converted.
     * @return String representation of the operation subtype.
     */
    [[nodiscard]] std::string convert_operation_subtype (
        const ControlPlaneOperationType& operation_type,
        const ControlPlaneOperationSubtype& operation_subtype) const
    {
        switch (operation_type) {
            case ControlPlaneOperationType::collect_detailed_stats:
                switch (operation_subtype) {
                    case ControlPlaneOperationSubtype::collect_stats_rocksdb:
                        return "collect-rocksdb-statistics";

                    case ControlPlaneOperationSubtype::collect_stats_tensorflow:
                        return "collect-tensorflow-statistics";

                    case ControlPlaneOperationSubtype::collect_stats_global:
                        return "collect-global-statistics";

                    default:
                        Logging::log_warn ("ConnectionHandler: unrecognized operation subtype ("
                            + std::to_string (static_cast<int> (operation_subtype)) + ")");
                        return "<undefined>";
                }

            case ControlPlaneOperationType::create_hsk_rule:
                switch (operation_subtype) {
                    case ControlPlaneOperationSubtype::hsk_create_channel:
                        return "create-channel";

                    case ControlPlaneOperationSubtype::hsk_create_object:
                        return "create-object";

                    default:
                        Logging::log_warn ("ConnectionHandler: unrecognized operation subtype ("
                            + std::to_string (static_cast<int> (operation_subtype)) + ")");
                        return "<undefined>";
                }

            default:
                Logging::log_warn ("ConnectionHandler: unrecognized operation type ("
                    + std::to_string (static_cast<int> (operation_type)) + ")");
                return "<undefined>";
        }
    }

    /**
     * log_control_operation: Auxiliary method to log (debug) messages of a given ControlOperation
     * object.
     * @param debug_mode Defines if the debug mode is enabled, to prevent unnecessary creation of
     * strings.
     * @param control_operation ControlOperation object that contains all information about the
     * control plane request.
     */
    void log_control_operation (bool debug_mode, const ControlOperation& control_operation)
    {
        if (debug_mode) {
            std::string message { "(" };
            message
                .append (this->convert_operation_type (
                    static_cast<ControlPlaneOperationType> (control_operation.m_operation_type)))
                .append (",");
            message
                .append (this->convert_operation_subtype (
                    static_cast<ControlPlaneOperationType> (control_operation.m_operation_type),
                    static_cast<ControlPlaneOperationSubtype> (
                        control_operation.m_operation_subtype)))
                .append (",");
            message.append (this->connection_handler_type_string ())
                .append (") : Control operation {");
            message.append (std::to_string (control_operation.m_operation_id)).append (", ");
            message.append (std::to_string (control_operation.m_operation_type)).append (", ");
            message.append (std::to_string (control_operation.m_operation_subtype)).append (", ");
            message.append (std::to_string (control_operation.m_size)).append ("}");
            Logging::log_debug (message);
        }
    }

    /**
     * log_return_value: Auxiliary method to log (debug) messages with the operation's result.
     * @param debug_mode Defines if the debug mode is enabled, to prevent unnecessary creation of
     * strings.
     * @param control_operation ControlOperation object that contains all information about the
     * control plane request.
     * @param return_value Return value to be logged.
     */
    void log_return_value (bool debug_mode,
        const ControlOperation& control_operation,
        const ssize_t& return_value)
    {
        if (debug_mode) {
            std::string message { "(" };
            message
                .append (this->convert_operation_type (
                    static_cast<ControlPlaneOperationType> (control_operation.m_operation_type)))
                .append (",");
            message.append (this->connection_handler_type_string ()).append (") : return value {");
            message.append (std::to_string (return_value)).append ("}");
            Logging::log_debug (message);
        }
    }

private:
    ConnectionHandlerType m_handler_type { ConnectionHandlerType::no_op };

    /**
     * connection_handler_type_string: Auxiliary method to return the connection handler type in
     * string format.
     * @return Returns a string of m_handler_type instance variable.
     */
    [[nodiscard]] std::string connection_handler_type_string () const
    {
        switch (this->m_handler_type) {
            case ConnectionHandlerType::handshake_handler:
                return "handshake_handler";
            case ConnectionHandlerType::southbound_handler:
                return "southbound_handler";
            default:
                return "no_op";
        }
    }

    /**
     * connect_to_control_plane: select which type of connection to establish between the data plane
     * stage and the control plane. The method verifies the connection type (m_connection_type) and
     * establishes the respective communication channel (none, unix Domain Sockets, or TCP).
     * CommunicationType::none sets the data plane stage to execute without a controller.
     * @param address Address to be used for the connection; socket name for unix Domain Sockets
     * and IP address for TCP connection.
     * @param port Port to be used for the connection (only used in the TCP-based connection).
     */
    void connect_to_control_plane (const std::string& address, const int& port)
    {
        PStatus status = PStatus::Error ();
        switch (this->m_connection_options.get_connection_type ()) {
            // create a Unix Domain Socket connection
            case CommunicationType::_unix:
                status = this->establish_unix_domain_socket_connection (address.data ());
                break;

                // create an inet socket connection
            case CommunicationType::inet:
                status = this->establish_inet_connection (address.data (), port);
                break;

            case CommunicationType::rpc:
                Logging::log_error ("RPC communication not supported");
                return;

                // proceed without control plane management
            case CommunicationType::none:
                Logging::log_debug (
                    "ConnectionManager: data plane stage running without controller.");
                status = PStatus::OK ();
                break;

            default:
                Logging::log_error ("Communication type not supported.");
                break;
        }

        if (status.is_error ()) {
            throw std::runtime_error ("Error while creating connection.");
        }
    }

    /**
     * establish_inet_connection: Establish a new connection between the data plane stage and the
     * SDS control plane through an inet socket.
     * @param address inet connection's address.
     * @param port inet connection's port.
     * @return Returns PStatus::OK if connection was successfully created; and PStatus::Error
     * otherwise.
     */
    PStatus establish_inet_connection (const char* address, int port)
    {
        Logging::log_debug ("ConnectionHandler: establishing inet connection with controller.");

        // assign inet socket settings
        this->m_inet_socket.sin_family = AF_INET;
        this->m_inet_socket.sin_port = htons (port);

        // create socket
        this->m_socket->store (::socket (AF_INET, SOCK_STREAM, 0));

        // validate socket creation
        if (this->m_socket->load () < 0) {
            Logging::log_error ("Socket creation error.");
            return PStatus::Error ();
        }

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton (AF_INET, address, &this->m_inet_socket.sin_addr) <= 0) {
            Logging::log_error ("Invalid address or address not supported.");
            return PStatus::Error ();
        }

        // establish connection with control plane
        if (::connect (this->m_socket->load (),
                (struct sockaddr*)&this->m_inet_socket,
                sizeof (this->m_inet_socket))
            < 0) {
            Logging::log_error ("Connection Failed.");
            return PStatus::Error ();
        }

        return PStatus::OK ();
    }

    /**
     * establish_unix_domain_socket_connection: Establish a new connection between the data plane
     * stage and the SDS control plane through an unix domain socket.
     * @param socket_name Unix Domain Socket name.
     * @return Returns PStatus::OK if connection was successfully created; and PStatus::Error
     * otherwise.
     */
    PStatus establish_unix_domain_socket_connection (const char* socket_name)
    {
        Logging::log_debug (
            "ConnectionHandler: establishing unix connection with controller through "
            + std::string { socket_name });

        // assign unix domain socket settings
        this->m_unix_socket.sun_family = AF_UNIX;
        std::strncpy (this->m_unix_socket.sun_path,
            socket_name,
            sizeof (this->m_unix_socket.sun_path) - 1);

        // create socket
        this->m_socket->store (::socket (AF_UNIX, SOCK_STREAM, 0));

        // validate socket creation
        if (this->m_socket->load () < 0) {
            Logging::log_error ("Socket creation error.");
            return PStatus::Error ();
        }

        // establish connection with control plane
        auto connect_result = ::connect (this->m_socket->load (),
            (const struct sockaddr*)&this->m_unix_socket,
            sizeof (struct sockaddr_un));

        if (connect_result < 0) {
            Logging::log_error ("Connection Failed.");
            return PStatus::Error ();
        }

        return PStatus::OK ();
    }

public:
    /**
     * ConnectionHandler default constructor.
     */
    ConnectionHandler ()
    {
        Logging::log_debug ("ConnectionManager default constructor.");

        // connect to the SDS control plane
        this->connect_to_control_plane (this->m_connection_options.get_address (),
            this->m_connection_options.get_port ());
    }

    /**
     * ConnectionHandler parameterized constructor.
     * @param connection_options Defines the options to establish the connection, including the
     * address name and port number.
     * @param agent_ptr Shared pointer to the Agent object, originally created at PaioStage.
     * @param interrupted Shared pointer to the atomic boolean that indicates if the connection was
     * interrupted.
     * @param connection_handler_type Defines which handler should be used in the connection, namely
     * HandshakeHandler or SouthboundHandler.
     */
    ConnectionHandler (const ConnectionOptions& connection_options,
        std::shared_ptr<Agent> agent_ptr,
        const ConnectionHandlerType& connection_handler_type) :
        m_socket { std::make_shared<std::atomic<int>> (-1) },
        m_connection_options { connection_options },
        m_agent_ptr { agent_ptr },
        m_handler_type { connection_handler_type }
    {
        Logging::log_debug ("ConnectionHandler (full) parameterized constructor.");

        // connect to the SDS control plane
        this->connect_to_control_plane (connection_options.get_address (),
            connection_options.get_port ());
    }

    /**
     * ConnectionHandler parameterized constructor.
     * It connects to the control plane using default connection options.
     * @param agent_ptr Pointer to the Agent object.
     * @param interrupted Shared pointer to the atomic boolean that indicates if the connection is
     * interrupted.
     * @param ready Shared pointer to the atomic boolean that indicates if the data plane stage is
     * ready to receive control operations.
     */
    ConnectionHandler (std::shared_ptr<Agent> agent_ptr,
        const ConnectionHandlerType& connection_handler_type) :
        m_socket { std::make_shared<std::atomic<int>> (-1) },
        m_connection_options {},
        m_agent_ptr { agent_ptr },
        m_handler_type { connection_handler_type }
    {
        Logging::log_debug ("ConnectionHandler parameterized constructor.");

        // connect to the SDS control plane
        this->connect_to_control_plane (this->m_connection_options.get_address (),
            this->m_connection_options.get_port ());
    }

    /**
     * ConnectionHandler default constructor.
     */
    virtual ~ConnectionHandler () = default;

    /**
     * listen: Defines the logic to communicate with the control plane.
     */
    virtual void listen (const bool& debug) = 0;

    /**
     * is_configured: Validate if ConnectionHandler is valid and properly configured.
     * @return Returns true if m_socket's value is >= 0, and m_agent_ptr is not nullptr; returns
     * false otherwise.
     */
    [[nodiscard]] bool is_configured () const
    {
        return (this->m_socket->load () >= 0) && (this->m_agent_ptr != nullptr);
    }
};

} // namespace paio::networking

#endif // PAIO_CONNECTION_HANDLER_HPP
