/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_OPTIONS_HPP
#define PAIO_OPTIONS_HPP

#include <filesystem>
#include <paio/core/context_propagation_definitions.hpp>
#include <paio/statistics/channel_statistics.hpp>
#include <string>

using namespace paio::statistics;
namespace fs = std::filesystem;

#define PAIO_MAJOR 0
#define PAIO_MINOR 1
#define PAIO_PATCH 0

namespace paio::options {

// typedef uint32_t diff_token_t;
using diff_token_t = uint32_t;

static const int kMajorVersion = PAIO_MAJOR;
static const int kMinorVersion = PAIO_MINOR;
static const int kPatchVersion = PAIO_PATCH;

/**
 * EnforcementObjectType enum class.
 * Defines the type of available enforcement objects.
 *  - DRL respects to the DynamicRateLimiting enforcement object;
 *  - Noop respects to the Noop enforcement object;.
 */
enum class EnforcementObjectType { DRL = 1, NOOP = 0 };

/**
 * CommunicationType enum class.
 * Defines the type of supported CommunicationTypes. Specifically, the data plane stage communicates
 * with the control plane through:
 *  - unix: UNIX Domain Sockets;
 *  - inet: INET sockets;
 *  - rpc: remote procedure calls (e.g., gRPC, Cap n' Proto, etc.).
 *  TODO:
 *   - support rpc communication interface.
 */
enum class CommunicationType { unix = 1, inet = 2, rpc = 3, none = 0 };

/**
 * ChannelMode enum class.
 * Defines the operation mode of Channels.
 *  - fast_path: channels operate in synchronous fashion, where the thread that receives the request
 *  traverses all PAIO mechanisms to enforce the request (receive, differentiation, enforcement,
 *  response);
 *  - queueing: channels operate in a queue-based fashion, where the thread that receives the
 *  request emplaces it in a submission queue and waits for the request to be ready in a completion
 *  queue; the request is processed in background by a worker thread.
 */
enum class ChannelMode { fast_path = 1, queueing = 2 };

/**
 * HashingScheme enum class.
 * Defines the supported hashing schemes of the system, that can be used in the I/O differentiation
 * mechanism.
 * TODO:
 *  - add support for other hashing schemes (xxHash, Seahash, ...)
 */
enum class HashingScheme { MurmurHash_x86_32 = 1, MurmurHash_x86_128 = 2, MurmurHash_x64_128 = 3 };

/***************************************************************************************************
 * General Options: Default configurations of the data plane stage.
 **************************************************************************************************/

/**
 * option_default_communication_type: Defines the default communication type to be used.
 * As defined in the CommunicationType enum, with none the data plane stage executes without
 * connecting to the control plane, while with the other options, the data plane connects to the
 * control plane through the respective interface, namely Unix Domain Sockets (unix), Inet-based
 * communication (inet), and Remote Procedure Calls (rpc).
 */
const CommunicationType option_default_communication_type { CommunicationType::none };

/**
 * option_default_debug_log: Defines the default debug logging option. If enabled (true),
 * Logging::log_debug and Logging::log_debug_explicit messages will be written to stdout (or to a
 * logging file); if disabled (false), no debug messages will not be written.
 * It is recommended to use this option under preliminary experiments or debugging purposes. Do not
 * use it under production environments.
 */
const bool option_default_debug_log { false };

/**
 * option_environment_variable: Defines the default environment variable path for the stage name.
 * This value will be used by the StageInfo class for getting the name of the data plane stage.
 */
inline std::string option_environment_variable_name ()
{
    return "paio_name";
}

/**
 * option_environment_variable: Defines the default environment variable path.
 * This value will be used by the StageInfo class for getting additional data plane stage
 * information stored in this environment variable.
 */
inline std::string option_environment_variable_env ()
{
    return "paio_env";
}

/**
 * option_default_data_plane_stage_name: Defines the default data plane stage name. This value is
 * only used if StageInfo parameterized constructor is not used or if the
 * option_environment_variable_name is not set.
 */
inline std::string option_default_data_plane_stage_name ()
{
    return "paio-stage";
}

/**
 * main_path: Default main path.
 * This parameter defines the main path for the rules files (housekeeping, differentiation, and
 * enforcement).
 */
inline std::filesystem::path main_path ()
{
    return "../files/";
}

/**
 * option_default_housekeeping_rules_file_path: Defines the path to the default HousekeepingRules
 * file. The default HousekeepingRules will be inserted and enforced at system creation.
 */
inline fs::path option_default_housekeeping_rules_file_path ()
{
    return main_path ().string () + "default_housekeeping_rules_file";
}

/**
 * option_default_differentiation_rules_file_path: Defines the path to the default
 * DifferentiationRules file. The default DifferentiationRules will be inserted at system creation,
 * after the enforcement of HousekeepingRules.
 */
inline fs::path option_default_differentiation_rules_file_path ()
{
    return main_path ().string () + "default_differentiation_rules_file";
}

/**
 * option_default_enforcement_rules_file_path: Defines the path to the default EnforcementRules
 * file. The default EnforcementRules will be enforced at system creation, after the enforcement of
 * HousekeepingRules.
 */
inline fs::path option_default_enforcement_rules_file_path ()
{
    return main_path ().string () + "default_enforcement_rules_file";
}

/**
 * option_default_socket_name: Defines the default socket name to be used in unix Domain Socket
 * communications. It provides the full path of the socket file to be used.
 */
inline std::string option_default_socket_name ()
{
    return "/tmp/9Lq7BNBnBycd6nxy.socket";
}

/**
 * option_default_address: Defines the default address to be used in TCP-based communications.
 */
inline std::string option_default_address ()
{
    return "127.0.0.1";
}

/**
 * option_default_port: Defines the default port to be used in TCP-based communications.
 */
const int option_default_port { 12345 };

/**
 * option_execute_rule_on_receive: Defines if rules (Housekeeping, Differentiation, Enforcement) are
 * executed after being received from the control plane, or if they should be placed on their
 * respective table to be enforced at a later point.
 */
const bool option_execute_rule_on_receive { true };

/**
 * option_default_hashing_algorithm: Default hashing scheme to be used in I/O differentiation,
 * namely channel and enforcement object differentiation.
 * The available options are MurmurHash_x86_32, MurmurHash_x86_128, and MurmurHash_x64_128.
 */
const HashingScheme option_default_hashing_algorithm { HashingScheme::MurmurHash_x86_32 };

/**
 * option_has_io_transformation: Defines if the data plane has I/O transformations, to treat the
 * requests accordingly in the InstanceInterface derived classes.
 */
const bool option_default_has_io_transformation { false };

/*
 * *************************************************************************************************
 * Channel Options: Default configurations of Channels (type, differentiation, ...).
 * *************************************************************************************************
 */

/**
 * option_create_default_channels: Defines if channels (in the Core class) should be created by
 * default, or explicitly created through HousekeepingRules submitted from the control plane or from
 * local files.
 */
const bool option_create_default_channels { true };

/**
 * option_default_channel_mode: Defines the default operation mode of channel to be used, namely
 * fast path (ChannelMode::fast_path) or use queueing (ChannelMode::queueing).
 */
const ChannelMode option_default_channel_mode { ChannelMode::fast_path };

/**
 * option_define_default_object_differentiation_on_create_channel: Defines if the EnforcementObject
 * I/O differentiation should be default and made at Channel creation time.
 */
const bool option_define_default_object_differentiation_on_create_channel { true };

/**
 * option_default_channel_differentiation_workflow: Defines if the workflow I/O classifier should be
 * considered for channel selection, and the respective classification and differentiation of I/O
 * requests.
 */
const bool option_default_channel_differentiation_workflow { true };

/**
 * option_default_channel_differentiation_operation_type: Defines if the operation type I/O
 * classifier should be considered for channel selection, and the respective classification and
 * differentiation of I/O requests.
 */
const bool option_default_channel_differentiation_operation_type { false };

/**
 * option_default_channel_differentiation_operation_context: Defines if the operation context I/O
 * classifier should be considered for channel selection, and the respective classification and
 * differentiation of I/O requests.
 */
const bool option_default_channel_differentiation_operation_context { false };

/**
 * option_default_enforcement_object_differentiation_operation_type: Defines if the operation type
 * I/O classifier should be considered for EnforcementObject selection, and the respective
 * classification and differentiation of I/O requests.
 */
const bool option_default_enforcement_object_differentiation_operation_type { true };

/**
 * option_default_enforcement_object_differentiation_operation_context: Defines if the operation
 * type I/O classifier should be considered for EnforcementObject selection, and the respective
 * classification and differentiation of I/O requests.
 */
const bool option_default_enforcement_object_differentiation_operation_context { true };

/**
 * option_default_channel_thread_pool_size: Defines the size of the channel's thread pool to dequeue
 * when operating in ChannelMode::Queueing.
 * It is recommended to divide the std::thread::hardware_concurrency() by the number of existing
 * channels in the data plane stage.
 */
const int option_default_channel_thread_pool_size { 4 };

/**
 * option_default_channel_statistic_collection: Enable/disable I/O statistics collection at
 * channels.
 */
const bool option_default_channel_statistic_collection { true };

/**
 * option_default_object_statistic_collection: Enable/disable I/O statistic collection at
 * EnforcementObjects.
 */
const bool option_default_object_statistic_collection { false };

/**
 * option_default_statistic_metric: Defines the metric to be used on statistic collection.
 * The available options are StatisticMetric::throughput (e.g., bandwidth) and
 * StatisticMetric::counter (e.g., number of operations).
 */
const StatisticMetric option_default_statistic_metric { StatisticMetric::throughput };

/**
 * option_default_statistic_classifier: Defines the I/O classifier of which statistics should be
 * collected.
 * In case of ClassifierType::operation_type, statistics will be collected based on the operation
 * type classifier, namely read, write, put, get, etc.
 * In case of ClassifierType::operation_context, statistics will be collected based on the operation
 * context classifier, namely bg_flush, bg_compaction, etc.
 */
const ClassifierType option_default_statistic_classifier { ClassifierType::operation_type };

/**
 * option_default_context_type: Default operation context classifier type to be considered in I/O
 * differentiation (e.g., context propagation, channel differentiation, and enforcement object
 * differentiation). The available options are PAIO_GENERAL, POSIX, POSIX_META, LSM_KVS_SIMPLE,
 * LSM_LVS_DETAILED, and KVS.
 */
const ContextType option_default_context_type { ContextType::PAIO_GENERAL };

} // namespace paio::options

#endif // PAIO_OPTIONS_HPP
