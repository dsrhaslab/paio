/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CONNECTION_OPTIONS_HPP
#define PAIO_CONNECTION_OPTIONS_HPP

#include <cstdio>
#include <paio/options/options.hpp>
#include <sstream>
#include <string>

using namespace paio::options;
using namespace paio::utils;

namespace paio::networking {

/**
 * ConnectionOptions class.
 * This class is used to define the configurations for the connection between the data plane stage
 * and the control plane. It provides three main options, namely:
 *  - m_connection_type: defines the type of connection to be established, which can be either
 *  CommunicationType::unix (Unix domain sockets), CommunicationType::inet (IPv4 sockets),
 *  CommunicationType::rpc (remote procedure call sockets), or CommunicationType::none (no
 *  connection with the controller; the stage operates in standalone mode). The default value is
 *  established by option_default_communication_type.
 *  - m_address: defines the connection address. In the case of CommunicationType::unix, the default
 *  value is established by option_default_socket_name (), while in the case of
 *  CommunicationType::inet, the default value is established by option_default_address ().
 *  - m_port: defines the connection port. The default value is established by option_default_port.
 */
class ConnectionOptions {

private:
    CommunicationType m_connection_type { option_default_communication_type };
    std::string m_address {};
    int m_port { option_default_port };

public:
    /**
     * ConnectionOptions default constructor.
     */
    ConnectionOptions ()
    {
        // check if CommunicationType::unix to select the use of default socket name or default
        // address if CommunicationType is of any other type.
        switch (m_connection_type) {
            case CommunicationType::unix:
                this->m_address = option_default_socket_name ();
                break;

            case CommunicationType::inet:
                this->m_address = option_default_address ();
                break;

            case CommunicationType::rpc:
                throw std::logic_error ("RPC connection type not implemented yet.");

            default:
                this->m_address = "noaddress";
                break;
        }

        Logging::log_debug_explicit (
            "CommunicationOptions default constructor " + this->to_string ());
    }

    /**
     * ConnectionOptions parameterized constructor.
     * @param type Type of communication to be established (unix domain socket, tcp sockets, remote
     * procedure calls, or none).
     * @param address Address to be used for the connection.
     * @param port Port to be used for the connection.
     */
    ConnectionOptions (const CommunicationType& type, std::string address, const int& port) :
        m_connection_type { type },
        m_address { std::move (address) },
        m_port { port }
    {
        Logging::log_debug_explicit (
            "CommunicationOptions parameterized constructor " + this->to_string ());
    }

    /**
     * ConnectionOptions default destructor.
     */
    ~ConnectionOptions () = default;

    /**
     * get_connection_type: get ConnectionOptions' connection type value.
     * @return Returns a copy of the m_connection_type parameter.
     */
    [[nodiscard]] CommunicationType get_connection_type () const
    {
        return this->m_connection_type;
    }

    /**
     * get_address: get ConnectionOptions' address value.
     * @return Returns a copy of the m_address parameter.
     */
    [[nodiscard]] std::string get_address () const
    {
        return this->m_address;
    }

    /**
     * get_port: get ConnectionOptions' port value.
     * @return Returns a copy of the m_port parameter.
     */
    [[nodiscard]] int get_port () const
    {
        return this->m_port;
    }

    /**
     * connection_type_to_string: convert ConnectionOptions' connection type value to string.
     * @param type ConnectionOptions' connection type value.
     * @return Returns a string representation of the connection type.
     */
    std::string connection_type_to_string (const CommunicationType& type) const
    {
        switch (type) {
            case CommunicationType::unix:
                return "unix";
            case CommunicationType::inet:
                return "inet";
            case CommunicationType::rpc:
                return "rpc";
            default:
                return "none";
        }
    }

    /**
     * to_string: Convert ConnectionOptions object in string-based format.
     * @return Returns a string with the ConnectionOptions elements.
     */
    [[nodiscard]] std::string to_string () const
    {
        std::string message { "{" };
        message.append (connection_type_to_string (this->m_connection_type)).append (", ");
        message.append (this->m_address).append (", ");
        message.append (std::to_string (this->m_port)).append ("}");
        return message;
    }
};
} // namespace paio::networking

#endif // PAIO_CONNECTION_OPTIONS_HPP
