/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/utils/logging.hpp>

namespace paio::utils {

// FIXME: Needing refactor or cleanup -@gsd at 4/19/2022, 2:24:26 PM
// This is hard-coded; should be a parameter of the constructor.
bool Logging::m_debug_enabled { true };
// FIXME: Needing refactor or cleanup -@gsd at 4/19/2022, 2:16:49 PM
// This is hard-coded; should be a parameter of the constructor.
bool Logging::m_is_ld_preloaded { true };
int Logging::m_fd { STDOUT_FILENO };
void* Logging::m_dl_handle { nullptr };
// FIXME: Needing refactor or cleanup -@gsd at 4/19/2022, 2:19:08 PM
// This is hard-coded; should be a parameter of the constructor.
std::string Logging::m_log_file_path { "/tmp/paio" };

// Logging default constructor.
Logging::Logging ()
{
    this->initialize ();
}

// Logging parameterized constructor.
Logging::Logging (bool debug)
{
    this->initialize ();
    // this->set_debug (debug);
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

// create_file_name call. (...)
std::string create_file_name (const std::string& file_name)
{
    std::string name {};
    if (!file_name.empty ()) {
        name = file_name + "-" + std::to_string (::getpid ()) + ".log";
    }

    return name;
}

// initialize call. (...)
void Logging::initialize ()
{
    // enable debug level at spdlog
    if (paio::utils::Logging::m_debug_enabled) {
        spdlog::set_level (spdlog::level::debug);
    }

    // bypass spdlog to prevent recursive dependency and/or null pointers to libc functions
    if (paio::utils::Logging::m_is_ld_preloaded) {
        // point m_dl_handle to next library in the stack
        paio::utils::Logging::m_dl_handle = RTLD_NEXT;

        if (!paio::utils::Logging::m_log_file_path.empty ()) {
            auto file_name = create_file_name (paio::utils::Logging::m_log_file_path);
            // open file using dlsym'ed close
            paio::utils::Logging::m_fd
                = ((libc_open_variadic_t)::dlsym (paio::utils::Logging::m_dl_handle,
                    "open")) (file_name.c_str (), O_CREAT | O_WRONLY | O_APPEND, 0666);

            // verify file descriptor result
            if (paio::utils::Logging::m_fd == -1) {
                perror ("Log::Error in initialize");
                paio::utils::Logging::m_fd = STDOUT_FILENO;
            }
        }
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

// create_formatted_info_message call.
std::string Logging::create_formatted_info_message (const std::string& message)
{
    return create_formatted_message (message, "[paio:info] ");
}

// create_formatted_warn_message call.
std::string Logging::create_formatted_warn_message (const std::string& message)
{
    return create_formatted_message (message, "[paio:warn] ");
}

// create_formatted_error_message call.
std::string Logging::create_formatted_error_message (const std::string& message)
{
    return create_formatted_message (message, "[paio:error] ");
}

// create_formatted_debug_message call.
std::string Logging::create_formatted_debug_message (const std::string& message)
{
    return create_formatted_message (message, "[paio:debug] ");
}

// dlsym_write_message call.
ssize_t Logging::dlsym_write_message (const std::string& message)
{
    return ((libc_write_t)::dlsym (RTLD_NEXT,
        "write")) (paio::utils::Logging::m_fd, message.c_str (), message.size ());
}

// log_info call. Log message with INFO qualifier.
void Logging::log_info (const std::string& message)
{
    if (paio::utils::Logging::m_is_ld_preloaded) {
        // generate formatted info message
        auto msg = paio::utils::Logging::create_formatted_info_message (message);
        // write log message to file descriptor using dlsym'ed write
        auto return_value = paio::utils::Logging::dlsym_write_message (msg);

        // verify return value of write operation
        if (return_value < 0) {
            perror ("Log::Error on log_info");
        }
    } else {
        spdlog::info (message);
    }
}

// log_warn call. Log message with WARN qualifier.
void Logging::log_warn (const std::string& message)
{
    if (paio::utils::Logging::m_is_ld_preloaded) {
        // generate formatted warn message
        auto msg = paio::utils::Logging::create_formatted_warn_message (message);
        // write log message to file descriptor using dlsym'ed write
        auto return_value = paio::utils::Logging::dlsym_write_message (msg);

        // verify return value of write operation
        if (return_value < 0) {
            perror ("Log::Error on log_warn");
        }
    } else {
        spdlog::warn (message);
    }
}

// log_error call. Log message with ERROR qualifier.
void Logging::log_error (const std::string& message)
{
    if (paio::utils::Logging::m_is_ld_preloaded) {
        // generate formatted error message
        auto msg = paio::utils::Logging::create_formatted_error_message (message);
        // write log message to file descriptor using dlsym'ed write
        auto return_value = paio::utils::Logging::dlsym_write_message (msg);

        // verify return value of write operation
        if (return_value < 0) {
            perror ("Log::Error on log_error");
        }
    } else {
        spdlog::error (message);
    }
}

// log_debug call. Log message with DEBUG qualifier.
void Logging::log_debug (const std::string& message)
{
    if (paio::utils::Logging::m_is_ld_preloaded) {
        // generate formatted debug message
        auto msg = paio::utils::Logging::create_formatted_debug_message (message);
        // write log message to file descriptor using dlsym'ed write
        auto return_value = paio::utils::Logging::dlsym_write_message (msg);

        // verify return value of write operation
        if (return_value < 0) {
            perror ("Log::Error on log_debug");
        }
    } else {
        spdlog::debug (message);
    }
}

// log_debug_explicit call. Log debug message bypassing the spdlog library.
void Logging::log_debug_explicit (const std::string& message)
{
    auto log_message = create_formatted_message (message, "[paio:debug_explicit]");
    if (paio::utils::Logging::m_debug_enabled) {
        std::fprintf (stderr, "%s", log_message.c_str ());
    }
}

// is_debug_enabled call. Validate if debug messages are enabled.
bool Logging::is_debug_enabled ()
{
    return paio::utils::Logging::m_debug_enabled;
}

} // namespace paio::utils
