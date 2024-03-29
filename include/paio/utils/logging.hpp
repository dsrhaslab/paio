/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_LOGGING_HPP
#define PAIO_LOGGING_HPP

#include <dlfcn.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <paio/options/libc_headers.hpp>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unistd.h>

using namespace paio::headers;

namespace paio::utils {

/**
 * Logging class.
 * This class contains the primitives to write the data plane stage logging messages. For ease of
 * use, it resorts to the spdlog logging library. Currently, it supports log messages of the
 * following qualifiers: INFO, ERROR, and DEBUG. Log messages can be written to stdout or file.
 * TODO:
 *  - Logger only reports to stdout; add option to persist logging messages.
 *  - Add options to bypass the spdlog library.
 */
class Logging {

private:
    std::shared_ptr<spdlog::logger> m_logger {};
    static bool m_is_ld_preloaded;
    static int m_fd;
    static void* m_dl_handle;
    static std::string m_log_file_path;

    /**
     * set_debug: enable/disable logging debug messages.
     * @param debug Boolean value that enables (if true) the debugging mode, and disable otherwise.
     */
    void set_debug (bool debug);

    /**
     * create_formatted_info_message:
     */
    static std::string create_formatted_info_message (const std::string& message);

    /**
     * create_formatted_warn_message:
     */
    static std::string create_formatted_warn_message (const std::string& message);

    /**
     * create_formatted_debug_message:
     */
    static std::string create_formatted_debug_message (const std::string& message);

    /**
     * create_formatted_error_message:
     */
    static std::string create_formatted_error_message (const std::string& message);

    /**
     * dlsym_write_message:
     */
    static ssize_t dlsym_write_message (const std::string& message);

    /**
     * initialize:
     */
    void initialize ();

public:
    static bool m_debug_enabled;

    /**
     * Logging default constructor.
     */
    Logging ();

    /**
     * Logging parameterized constructor.
     * If @param debug is true, the logging mode is set to debug.
     * @param debug Boolean value that defines if the debug is enabled or disabled.
     */
    explicit Logging (bool debug);

    /**
     * Logging default destructor.
     */
    ~Logging ();

    /**
     * log_info: Log a message with the INFO qualifier.
     * @param message Log message.
     */
    static void log_info (const std::string& message);

    /**
     * log_warn: Log a message with the WARN qualifier.
     * @param message Log message.
     */
    static void log_warn (const std::string& message);

    /**
     * log_error: Log a message with the ERROR qualifier.
     * @param message Log message.
     */
    static void log_error (const std::string& message);

    /**
     * log_debug: Log a message with the DEBUG qualifier.
     * @param message Log message.
     */
    static void log_debug (const std::string& message);

    /**
     * log_debug: Log debug messages bypassing the spdlog library (to stdout or to a given file).
     * @param message Log message.
     */
    static void log_debug_explicit (const std::string& message);

    /**
     * is_debug_enabled: Validate if debugging is enabled (i.e., writing messages with the DEBUG
     * qualifier). This is useful for preventing the construction of debug messages, which can incur
     * high overhead if called for each I/O request.
     * @return Returns a boolean that defines whether debug is enabled or not.
     */
    static bool is_debug_enabled ();
};
} // namespace paio::utils

#endif // PAIO_LOGGING_HPP
