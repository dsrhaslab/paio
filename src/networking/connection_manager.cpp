/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/networking/connection_manager.hpp>
#include <utility>

namespace paio::networking {

// ConnectionManager default constructor.
ConnectionManager::ConnectionManager () :
    m_connection_interrupted { std::make_shared<std::atomic<bool>> (false) }
{
    Logging::log_debug ("ConnectionManager default constructor.");

    // connect to the SDS control plane
    this->connect (this->m_connection_options);
}

// ConnectionManager parameterized constructor. Initializes ConnectionManager object using a
// ConnectionOptions object and the interrupted shared-pointer.
ConnectionManager::ConnectionManager (const ConnectionOptions& connection_options,
    std::shared_ptr<Agent> agent_ptr,
    std::shared_ptr<std::atomic<bool>> interrupted) :
    m_socket { std::make_shared<std::atomic<int>> (-1) },
    m_connection_options { connection_options },
    m_agent_ptr { agent_ptr },
    m_connection_interrupted { interrupted },
    m_handshake_connection_handler {
        std::make_unique<HandshakeConnectionHandler> (connection_options, agent_ptr, interrupted)
    }
{
    Logging::log_debug ("ConnectionManager parameterized constructor.");

    // connect to the control plane
    this->connect (connection_options);
}

// ConnectionManager parameterized constructor. Initializes ConnectionManager object using a
// ConnectionOptions object and the interrupted shared-pointer.
ConnectionManager::ConnectionManager (std::shared_ptr<Agent> agent_ptr,
    std::shared_ptr<std::atomic<bool>> interrupted) :
    m_socket { std::make_shared<std::atomic<int>> (-1) },
    m_connection_options {},
    m_agent_ptr { agent_ptr },
    m_connection_interrupted { interrupted },
    m_handshake_connection_handler { std::make_unique<HandshakeConnectionHandler> (agent_ptr,
        interrupted) }
{
    Logging::log_debug ("ConnectionManager parameterized constructor.");

    // connect to the control plane
    this->connect (this->m_connection_options);
}

// connect call. Complementary method that creates a connection to the SDS control plane and spawns
// a dedicated thread that receives control operations (from the control plane).
void ConnectionManager::connect (const ConnectionOptions& connection_options)
{
    // spawn thread to receive control operations from the control plane
    this->spawn_handshake_listening_thread (Logging::is_debug_enabled ());

    // create ConnectionOptions for the Southbound connection handler
    ConnectionOptions southbound_connection_options { option_default_communication_type,
        this->m_handshake_connection_handler->get_southbound_socket_name (),
        this->m_handshake_connection_handler->get_southbound_socket_port () };

    // initialize SouthboundConnectionHandler
    this->m_southbound_connection_handler
        = std::make_unique<SouthboundConnectionHandler> (southbound_connection_options,
            this->m_agent_ptr,
            this->m_connection_interrupted);

    // spawn thread to receive southbound operations from the control plane
    if (this->m_southbound_connection_handler != nullptr) {
        // spawn thread to receive southbound operations from the SDS control plane
        this->spawn_southbound_listening_thread (Logging::is_debug_enabled ());
    }
}

// ConnectionManager default destructor.
ConnectionManager::~ConnectionManager ()
{
    // terminate connection between the data plane and the SDS control plane
    this->disconnect_from_control_plane ();
}

// disconnect_from_control_plane call. Terminate the communication between the data plane stage and
// the control plane.
void ConnectionManager::disconnect_from_control_plane ()
{
    // mark connection as interrupted
    this->set_connection_interrupted (true);

    // verify the type of communication
    if (this->m_connection_options.get_connection_type () != CommunicationType::none) {
        // join communication thread
        this->m_connection_thread.join ();
    }
}

// spawn_handshake_listening_thread call. Spawn a dedicated thread to receive control operations
// from the control plane, using an HandshakeConnectionHandler object.
void ConnectionManager::spawn_handshake_listening_thread (const bool& debug)
{
    Logging::log_debug ("ConnectionManager: spawning (Handshake) communication thread ...");

    // verify the type of communication
    if (this->m_connection_options.get_connection_type () != CommunicationType::none) {
        // spawn thread to listen from socket
        this->m_connection_thread = std::thread (&HandshakeConnectionHandler::listen,
            this->m_handshake_connection_handler.get (),
            debug);

        // fixme: use conditions to wait for the handshake to be completed
        while (this->is_connection_interrupted () == false) {
            std::this_thread::sleep_for (std::chrono::milliseconds (100));
        }
        // join thread
        this->m_connection_thread.join ();
    } else {
        Logging::log_info ("PaioStage running without control plane.");
    }
}

// spawn_southbound_listening_thread call. Spawn a dedicated thread to receive control operations
// from the control plane, using an SouthboundConnectionHandler object.
void ConnectionManager::spawn_southbound_listening_thread (const bool& debug)
{
    Logging::log_debug ("ConnectionManager: spawning (Southbound) communication thread ...");

    // verify the type of communication
    if (this->m_connection_options.get_connection_type () != CommunicationType::none) {
        // spawn thread to listen from socket
        this->m_connection_thread = std::thread (&SouthboundConnectionHandler::listen,
            this->m_southbound_connection_handler.get (),
            debug);

        // log message
        Logging::log_debug ("ConnectionManager: stage listening control operations from the "
                            "control plane (through a Southbound communication thread).");
    } else {
        Logging::log_info ("PaioStage running without control plane.");
    }
}

// is_connection_interrupted call. Verify if the connection between the data and control plane is
// established or was interrupted.
bool ConnectionManager::is_connection_interrupted () const
{
    return this->m_connection_interrupted->load ();
}

// set_connection_interrupted call. Atomically define a new value to m_connection_interrupted.
void ConnectionManager::set_connection_interrupted (bool value)
{
    this->m_connection_interrupted->store (value);
}

// get_socket_identifier call. Get the socket identifier (file descriptor) of the socket that
// establishes the connection between the data plane and the control plane.
int ConnectionManager::get_socket_identifier () const
{
    return this->m_socket->load ();
}

// to_string call. Returns a string representation of the ConnectionManager object.
std::string ConnectionManager::to_string () const
{
    std::string message { "ConnectionManager {" };
    message.append (
        (this->m_socket != nullptr) ? std::to_string (this->m_socket->load ()) : "nullptr");
    message.append (", ").append (this->m_connection_options.to_string ()).append (", ");
    message.append (std::to_string (this->m_connection_interrupted->load ())).append ("}");
    return message;
}

} // namespace paio::networking
