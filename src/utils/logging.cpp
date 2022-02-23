/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/utils/logging.hpp>

namespace paio::utils {

// static variables
FILE* Logging::m_fd { stdout };
bool Logging::m_debug_enabled { false };

// Logging default constructor.
Logging::Logging () = default;

// Logging parameterized constructor.
Logging::Logging (bool debug)
{
    this->set_debug (debug);
}

// Logging default destructor.
Logging::~Logging () = default;

// set_debug call. Enabled debug messages.
void Logging::set_debug (bool debug)
{
    if (debug) {
        spdlog::set_level (spdlog::level::debug);
        paio::utils::Logging::m_debug_enabled = true;
    }
}

/**
 * create_formatted_message: Create a formatted log message, when not using/bypassing the
 * spdlog library.
 * @param message Message to be formatted and logged.
 * @param level Logging level.
 * @return Returns a formatted log message.
 */
std::string create_formatted_message (const std::string& message, const std::string& level)
{
    std::time_t current_time = std::time (nullptr);
    std::tm time_info = *std::localtime (&current_time);
    std::stringstream formatted_message;
    formatted_message << "[" << std::put_time (&time_info, "%F %T") << "] ";
    formatted_message << level << " ";
    formatted_message << message << "\n";

    return formatted_message.str ();
}

// log_info call. Log message with INFO qualifier.
void Logging::log_info (const std::string& message)
{
    spdlog::info (message);
}

// log_error call. Log message with ERROR qualifier.
void Logging::log_error (const std::string& message)
{
    spdlog::error (message);
}

// log_debug call. Log message with DEBUG qualifier.
void Logging::log_debug (const std::string& message)
{
    spdlog::debug (message);
}

// log_debug_explicit call. Log debug message bypassing the spdlog library.
void Logging::log_debug_explicit (const std::string& message)
{
    auto log_message = create_formatted_message (message, "[debug]");
    if (paio::utils::Logging::m_debug_enabled) {
        std::fprintf (paio::utils::Logging::m_fd, "%s", log_message.c_str ());
    }
}

// is_debug_enabled call. Validate if debug messages are enabled.
bool Logging::is_debug_enabled ()
{
    return paio::utils::Logging::m_debug_enabled;
}

} // namespace paio::utils
