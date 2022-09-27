/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/networking/handshake_connection_handler.hpp>
#include <utility>

namespace paio::networking {

// HandshakeConnectionHandler default constructor.
HandshakeConnectionHandler::HandshakeConnectionHandler () : ConnectionHandler {}
{
    Logging::log_debug ("HandshakeConnectionHandler default constructor.");
}

// HandshakeConnectionHandler fully parameterized constructor.
HandshakeConnectionHandler::HandshakeConnectionHandler (const ConnectionOptions& connection_options,
    std::shared_ptr<Agent> agent_ptr) :
    ConnectionHandler { connection_options, agent_ptr, ConnectionHandlerType::handshake_handler }
{
    Logging::log_debug ("HandshakeConnectionHandler fully parameterized constructor.");
}

// HandshakeConnectionHandler parameterized constructor.
HandshakeConnectionHandler::HandshakeConnectionHandler (std::shared_ptr<Agent> agent_ptr) :
    ConnectionHandler { agent_ptr, ConnectionHandlerType::handshake_handler }
{
    Logging::log_debug ("HandshakeConnectionHandler parameterized constructor.");
}

// HandshakeConnectionHandler default destructor.
HandshakeConnectionHandler::~HandshakeConnectionHandler ()
{
    Logging::log_debug_explicit ("HandshakeConnectionHandler destructor.");
}

// read_control_operation_from_socket call. Read ControlOperation object from socket.
ssize_t HandshakeConnectionHandler::read_control_operation_from_socket (ControlOperation* operation)
{
    ssize_t return_value;
    // acquire read lock
    std::unique_lock<std::mutex> read_lock (this->m_socket_read_lock);

    // verify if m_socket is valid
    if (ConnectionHandler::m_socket->load () > 0) {
        // read instruction from socket
        return_value = ConnectionHandler::socket_read (operation, sizeof (ControlOperation));

        // create debug message
        std::string log_message = "handshake_handler::socket_read (";
        log_message.append (std::to_string (return_value)).append (",");
        log_message.append (std::to_string (operation->m_operation_type)).append (",");
        log_message.append (std::to_string (operation->m_operation_subtype)).append (",");
        log_message.append (std::to_string (operation->m_size)).append (")");
        Logging::log_debug (log_message);

        if (return_value < 0) {
            Logging::log_error (
                "HandshakeConnectionHandler: error while reading bytes from control plane.");
        }
    } else {
        throw std::runtime_error ("HandshakeConnectionHandler: invalid socket ("
            + std::string (std::strerror (errno)) + ")");
    }

    return return_value;
}

// handle_control_operation call. Handle operation received from the control plane.
ssize_t HandshakeConnectionHandler::handle_control_operation (const ControlOperation& operation,
    const bool& debug)
{
    ssize_t return_value;
    // log control operation message
    ConnectionHandler::log_control_operation (debug, operation);

    switch (static_cast<ControlPlaneOperationType> (operation.m_operation_type)) {
        case ControlPlaneOperationType::stage_handshake:
            // call stage_handshake
            return_value = this->stage_handshake ();
            break;

        default:
            throw std::logic_error ("HandshakeConnectionHandler: unknown operation type ("
                + std::to_string (operation.m_operation_type) + ")");
    }

    // logging return-value message
    ConnectionHandler::log_return_value (debug, operation, return_value);

    return return_value;
}

// stage_handshake call. Submit the stage information to the control plane, and receive new address
// and port for the southbound interface connection.
ssize_t HandshakeConnectionHandler::stage_handshake ()
{
    ssize_t return_value;

    // copy Stage identifier to stage_handshake object
    StageInfoRaw info_obj {};
    this->m_agent_ptr->get_stage_info (info_obj);

    // create StageHandshakeRaw object
    StageHandshakeRaw handshake_obj {};

    // debug message
    Logging::log_debug (paio::core::stage_info_raw_string (info_obj));

    { // entering critical section
        // acquire write lock
        std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
        // write StageInfoRaw object to socket
        return_value = ConnectionHandler::socket_write (&info_obj, sizeof (StageInfoRaw));
    }

    // validate return value
    if (return_value <= 0) {
        Logging::log_error (
            "Error while writing stage handshake message (" + std::to_string (return_value) + ").");
        return return_value;
    }

    { // entering critical section
        // acquire read lock
        std::unique_lock<std::mutex> read_lock (this->m_socket_read_lock);

        // read handshake response (StageHandshakeRaw object) from socket
        return_value = ConnectionHandler::socket_read (&handshake_obj, sizeof (StageHandshakeRaw));
    }

    // validate return value
    if (return_value <= 0) {
        Logging::log_error (
            "Error while writing stage handshake message (" + std::to_string (return_value) + ").");
    } else {
        Logging::log_debug (
            "Received handshake object: " + paio::core::stage_handshake_raw_string (handshake_obj));

        // update southbound socket info with new address and port
        this->set_southbound_socket_info (handshake_obj);
    }

    return return_value;
}

// listen call. Listen for incoming ControlOperations (stage_handshake) from the control plane.
void HandshakeConnectionHandler::listen (const bool& debug)
{
    ControlOperation control_operation {};

    // read stage handshake operation from socket
    auto read_bytes = this->read_control_operation_from_socket (&control_operation);

    // validate bytes read and connection state
    if (read_bytes > 0) {
        // Receive and handle the rule submitted by the controller
        read_bytes = this->handle_control_operation (control_operation, debug);
    }

    // validate bytes read
    if (read_bytes <= 0) {
        throw std::runtime_error ("ConnectionManager: failed to receive control operation.");
    }
}

// get_southbound_socket_name call. Return the name/address of the southbound connection socket.
std::string HandshakeConnectionHandler::get_southbound_socket_name ()
{
    std::lock_guard<std::mutex> guard (this->m_southbound_info_lock);
    return this->m_southbound_socket_name;
}

// get_southbound_socket_port call. Return the port of the southbound connection socket.
int HandshakeConnectionHandler::get_southbound_socket_port ()
{
    std::lock_guard<std::mutex> guard (this->m_southbound_info_lock);
    return this->m_southbound_socket_port;
}

// set_southbound_socket_info call. Set the name/address and port of the southbound connection.
void HandshakeConnectionHandler::set_southbound_socket_info (
    const StageHandshakeRaw& handshake_object)
{
    std::lock_guard<std::mutex> guard (this->m_southbound_info_lock);
    this->m_southbound_socket_name = std::string (handshake_object.m_address);
    this->m_southbound_socket_port = handshake_object.m_port;
}

} // namespace paio::networking
