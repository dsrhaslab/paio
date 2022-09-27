/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/networking/southbound_connection_handler.hpp>
#include <utility>

namespace paio::networking {

// SouthboundConnectionHandler default constructor.
SouthboundConnectionHandler::SouthboundConnectionHandler ()
{
    Logging::log_debug ("SouthboundConnectionHandler default constructor.");
}

// SouthboundConnectionHandler fully parameterized constructor.
SouthboundConnectionHandler::SouthboundConnectionHandler (
    const ConnectionOptions& connection_options,
    std::shared_ptr<Agent> agent_ptr,
    std::shared_ptr<std::atomic<bool>> shutdown) :
    ConnectionHandler { connection_options, agent_ptr, ConnectionHandlerType::southbound_handler },
    m_stage_shutdown { shutdown }
{
    Logging::log_debug ("SouthboundConnectionHandler fully parameterized constructor");
}

// SouthboundConnectionHandler parameterized constructor.
SouthboundConnectionHandler::SouthboundConnectionHandler (std::shared_ptr<Agent> agent_ptr,
    std::shared_ptr<std::atomic<bool>> shutdown) :
    ConnectionHandler { agent_ptr, ConnectionHandlerType::southbound_handler },
    m_stage_shutdown { shutdown }
{
    Logging::log_debug ("SouthboundConnectionHandler parameterized constructor");
}

// SouthboundConnectionHandler default destructor.
SouthboundConnectionHandler::~SouthboundConnectionHandler ()
{
    Logging::log_debug_explicit ("SouthboundConnectionHandler default destructor");
}

// read_control_operation_from_socket call. Read ControlOperation object from socket.
ssize_t SouthboundConnectionHandler::read_control_operation_from_socket (
    ControlOperation* operation)
{
    ssize_t return_value;
    // acquire read lock
    std::unique_lock<std::mutex> read_lock (this->m_socket_read_lock);

    // verify if m_socket is valid
    if (ConnectionHandler::m_socket->load () > 0) {
        // read instruction from socket
        return_value = ConnectionHandler::socket_read (operation, sizeof (ControlOperation));

        // create debug message
        std::string log_message = "southbound_handler::socket_read (";
        log_message.append (std::to_string (return_value)).append (",");
        log_message.append (std::to_string (operation->m_operation_type)).append (",");
        log_message.append (std::to_string (operation->m_operation_subtype)).append (",");
        log_message.append (std::to_string (operation->m_size)).append (")");
        Logging::log_debug (log_message);

        if (return_value < 0) {
            Logging::log_error (
                "SouthboundConnectionHandler: error while reading bytes from control plane.");
        }
    } else {
        throw std::runtime_error ("SouthboundConnectionHandler: invalid socket ("
            + std::string (std::strerror (errno)) + ")");
    }

    return return_value;
}

// mark_stage_as_ready call. Mark data plane stage as ready.
ssize_t SouthboundConnectionHandler::mark_stage_as_ready (const ControlOperation& operation)
{
    PStatus status = PStatus::Error ();
    ACK response {};
    ssize_t return_value;

    // Receive Phase
    StageReadyRaw mark_stage {};

    { // entering critical section
        // acquire read lock
        std::unique_lock<std::mutex> read_lock (this->m_socket_read_lock);
        // read StageReadyRaw structure from socket
        return_value = ConnectionHandler::socket_read (&mark_stage, operation.m_size);
    }

    // validate return value
    if (return_value <= 0) {
        Logging::log_error (
            "Error while reading stage-ready message (" + std::to_string (return_value) + ").");
    } else {
        // mark data plane stage as ready
        if (mark_stage.m_mark_stage) {
            // mark data plane stage as ready
            this->m_agent_ptr->mark_data_plane_stage_ready ();
            status = PStatus::OK ();
        }
    }

    // validate if the rule was successfully enforced
    status.is_ok () ? response.m_message = static_cast<int> (AckCode::ok)
                    : response.m_message = static_cast<int> (AckCode::error);

    { // entering critical section
        // acquire write lock
        std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
        // send ACK message to the control plane
        return_value = ConnectionHandler::socket_write (&response, sizeof (ACK));

        if (return_value <= 0) {
            Logging::log_error ("Error while writing ACK message to control plane ("
                + std::to_string (return_value) + ").");
        }
    }

    return return_value;
}

// collect_statistics call. Collect statistics from each channel of the data plane stage.
ssize_t SouthboundConnectionHandler::collect_statistics (const ControlOperation& operation)
{
    CollectStatisticsMetadata collect_stats_meta {};
    PStatus status = PStatus::Error ();
    ssize_t return_value;

    { // entering critical section
        // acquire read lock
        std::unique_lock<std::mutex> read_lock (this->m_socket_read_lock);
        return_value = ConnectionHandler::socket_read (&collect_stats_meta, operation.m_size);
    }

    // validate number of bytes read
    if (return_value <= 0) {
        throw std::runtime_error ("Error while reading CollectStatisticsMetadata object ("
            + std::to_string (return_value) + ").");
    }

    // create empty vector for storing channel statistics
    std::vector<ChannelStatsRaw> channel_stats {};
    // collect statistics from each channel through the agent
    status = this->m_agent_ptr->collect_channel_statistics (collect_stats_meta.m_channel_id,
        channel_stats);

    ssize_t total_of_written_bytes = 0;
    // validate status and aggregate statistics
    if (status.is_ok ()) {
        { // entering critical section
            CollectStatisticsMetadata response { -1, static_cast<int> (channel_stats.size ()) };
            // acquire write lock
            std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
            // send the total number of collected channel statistics (prepare the control plane)
            return_value = ConnectionHandler::socket_write (&response, operation.m_size);

            // verify return value of socket write
            if (return_value != sizeof (CollectStatisticsMetadata)) {
                throw std::runtime_error (
                    "CollectStatistics: failed to send the number of collected statistics.");
            } else {
                total_of_written_bytes += return_value;
            }

            for (auto& channel_stat : channel_stats) {
                return_value
                    = ConnectionHandler::socket_write (&channel_stat, sizeof (ChannelStatsRaw));

                // verify return value of socket write
                if (return_value != sizeof (ChannelStatsRaw)) {
                    throw std::runtime_error (
                        "CollectStatistics: failed to send the collected statistics.");
                } else {
                    total_of_written_bytes += return_value;
                }
            }
        }
    } else {
        Logging::log_error ("CollectStatistics: failed to collect statistics.");
    }

    return total_of_written_bytes;
}

// collect_instance_statistics call. Collect statistics from channels of the data plane stage.
ssize_t SouthboundConnectionHandler::collect_instance_statistics (const ControlOperation& operation)
{
    PStatus status;
    ssize_t return_value = -1;

    // check subtype
    switch (static_cast<ControlPlaneOperationSubtype> (operation.m_operation_subtype)) {
        case ControlPlaneOperationSubtype::collect_stats_rocksdb: {
            // create container to store temporary metrics in raw format
            std::map<long, std::vector<double>> instance_stats_detailed {};
            // collect instance statistics through the Agent module
            status = this->m_agent_ptr->collect_detailed_channel_statistics (-1,
                instance_stats_detailed);

            // create temporary StatsSilkRaw object
            StatsSilkRaw stats_kvs_raw {};

            // verify exit status of CollectInstanceStatistics call
            if (status.is_ok ()) {
                // aggregate before sending through socket
                this->aggregate_kvs_statistics (instance_stats_detailed, stats_kvs_raw);

            } else {
                Logging::log_error ("CollectInstanceStatistics: error on collecting "
                                    "instance statistics.");
            }

            { // entering critical section
                // acquire write lock
                std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
                // send statistics object through socket
                return_value
                    = ConnectionHandler::socket_write (&stats_kvs_raw, sizeof (StatsSilkRaw));
            }

            // verify return value of socket write
            if (return_value != sizeof (StatsSilkRaw)) {
                throw std::runtime_error (
                    "CollectInstanceStatistics: failed to write KVS statistics.");
            } else {
                // logging debug message
                if (Logging::is_debug_enabled ()) {
                    std::string message { "CollectInstanceStatistics: " };
                    message.append (std::to_string (stats_kvs_raw.m_fg_tasks)).append (" -- ");
                    message.append (std::to_string (stats_kvs_raw.m_bg_tasks_flush))
                        .append (" -- ");
                    message.append (std::to_string (stats_kvs_raw.m_bg_tasks_compaction_l0))
                        .append (" -- ");
                    message.append (std::to_string (stats_kvs_raw.m_bg_tasks_compaction_lN));

                    status = PStatus::OK ();
                    Logging::log_debug (message);
                }
            }

            break;
        }

        case ControlPlaneOperationSubtype::collect_stats_tensorflow: {
            // create container to store temporary metrics in raw format
            std::map<long, std::vector<double>> detailed_stats {};
            // collect instance statistics through the Agent module
            status = this->m_agent_ptr->collect_detailed_channel_statistics (-1, detailed_stats);

            // create temporary StatsTensorFlowRaw object
            StatsTensorFlowRaw stats_tensorflow {};

            // verify exit status of CollectInstanceStatistics call
            if (status.is_ok ()) {
                // aggregate statistics before sending them through the socket
                status = this->aggregate_tf_statistics (detailed_stats, stats_tensorflow);

                // validate result of the aggregation operation
                if (!status.is_ok ()) {
                    Logging::log_error ("collect_instance_statistics: error while aggregating "
                                        "TensorFlow statistics.");
                }
            } else {
                Logging::log_error ("collect_instance_statistics: error while collecting detailed "
                                    "channel statistics.");
            }

            // submit statistics to the controller
            { // entering critical section
                // acquire write lock
                std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
                // send statistics object through socket
                return_value = ConnectionHandler::socket_write (&stats_tensorflow,
                    sizeof (StatsTensorFlowRaw));
            }

            // verify return value of socket write
            if (return_value <= 0) {
                status = PStatus::Error ();
                Logging::log_error ("Error while writing TensorFlow statistics to the control plane"
                                    " ("
                    + std::to_string (return_value) + ")");
            } else {
                status = PStatus::OK ();
                // logging debug message
                Logging::log_debug ("collect_instance_statistics: "
                    + std::to_string (stats_tensorflow.m_read_rate / 1024 / 1024) + " MiB/s read; "
                    + std::to_string (stats_tensorflow.m_write_rate / 1024 / 1024)
                    + " MiB/s write.");
            }

            break;
        }

        case ControlPlaneOperationSubtype::collect_stats_global: {
            // create container to store temporary metrics in raw format
            std::map<long, std::vector<double>> detailed_stats {};
            // collect instance statistics through the Agent module
            status = this->m_agent_ptr->collect_detailed_channel_statistics (-1, detailed_stats);

            // create temporary StatsGlobalRaw object
            StatsGlobalRaw stats_global {};

            // verify exit status of collect_detailed_channel_statistics
            if (status.is_ok ()) {
                // aggregate statistics before sending them through the socket
                status = this->aggregate_global_statistics (detailed_stats, stats_global);

                // validate result of the aggregation operation
                if (!status.is_ok ()) {
                    Logging::log_error ("collect_instance_statistics: error while aggregating "
                                        "global statistics.");
                } else {
                    // TODO: Tasks pending completion -@gsd at 6/17/2022, 2:15:07 PM
                    // remove this log message
                    Logging::log_debug (
                        "global-statistics :: " + std::to_string (stats_global.m_total_rate));
                }
            } else {
                Logging::log_error ("collect_instance_statistics: error while collecting detailed "
                                    "channel statistics.");
            }

            // submit statistics to the controller
            { // entering critical section
                // acquire write lock
                std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
                // send statistics object through socket
                return_value
                    = ConnectionHandler::socket_write (&stats_global, sizeof (StatsGlobalRaw));
            }

            // verify return value of socket write
            if (return_value <= 0) {
                status = PStatus::Error ();
                Logging::log_error ("Error while writing TensorFlow statistics to the control plane"
                                    " ("
                    + std::to_string (return_value) + ")");
            } else {
                status = PStatus::OK ();
                // logging debug message
                Logging::log_debug ("collect_instance_statistics: "
                    + std::to_string (stats_global.m_total_rate) + " IOPS/s | Bytes/s");
            }

            break;
        }

        case ControlPlaneOperationSubtype::collect_stats_metadata_data: {
            // create container to store temporary metrics in raw format
            std::map<long, std::vector<double>> detailed_stats {};
            // collect instance statistics through the Agent module
            status = this->m_agent_ptr->collect_detailed_channel_statistics (-1, detailed_stats);

            // create temporary StatsDataMetadataRaw object
            StatsDataMetadataRaw stats_metadata_data {};

            // verify exit status of collect_detailed_channel_statistics
            if (status.is_ok ()) {
                // aggregate statistics before sending them through the socket
                status = this->aggregate_metadata_data_statistics (detailed_stats,
                    stats_metadata_data);

                // validate result of the aggregation operation
                if (!status.is_ok ()) {
                    Logging::log_error ("collect_instance_statistics: error while aggregating "
                                        "global statistics.");
                } else {
                    // TODO: Tasks pending completion -@gsd at 6/17/2022, 2:15:07 PM
                    // remove this log message
                    Logging::log_debug ("data-metadata-statistics :: "
                        + std::to_string (stats_metadata_data.m_total_data_rate) + " - "
                        + std::to_string (stats_metadata_data.m_total_metadata_rate));
                }
            } else {
                Logging::log_error ("collect_instance_statistics: error while collecting detailed "
                                    "channel statistics.");
            }

            // submit statistics to the controller
            { // entering critical section
                // acquire write lock
                std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
                // send statistics object through socket
                return_value = ConnectionHandler::socket_write (&stats_metadata_data,
                    sizeof (StatsDataMetadataRaw));
            }

            // verify return value of socket write
            if (return_value <= 0) {
                status = PStatus::Error ();
                Logging::log_error ("Error while writing TensorFlow statistics to the control plane"
                                    " ("
                    + std::to_string (return_value) + ")");
            } else {
                status = PStatus::OK ();
                // logging debug message
                Logging::log_debug ("collect_instance_statistics: "
                    + std::to_string (stats_metadata_data.m_total_metadata_rate) + " IOPS/s; "
                    + std::to_string (stats_metadata_data.m_total_data_rate) + " Bytes/s; ");
            }

            break;
        }

        default:
            Logging::log_error ("collect_instance_statistics: subtype not supported.");
            break;
    }

    return return_value;
}

// create_housekeeping_rule call. Create a HousekeepingRule to be inserted in the PAIO Stage.
ssize_t SouthboundConnectionHandler::create_housekeeping_rule (const ControlOperation& operation)
{
    PStatus status = PStatus::Error ();
    ACK response {};
    ssize_t return_value;

    // validate subtype and provide different treatment depending on it
    switch (static_cast<ControlPlaneOperationSubtype> (operation.m_operation_subtype)) {
        case ControlPlaneOperationSubtype::hsk_create_channel: {
            // create HousekeepingCreateChannelRaw object for HousekeepingRule
            HousekeepingCreateChannelRaw create_channel_rule {};

            { // entering critical section
                // acquire read lock
                std::unique_lock<std::mutex> read_lock (this->m_socket_read_lock);
                // read structure from socket
                return_value
                    = ConnectionHandler::socket_read (&create_channel_rule, operation.m_size);
            }

            // validate number of bytes read
            if (return_value <= 0) {
                status = PStatus::Error ();
                Logging::log_error (
                    "Error while reading create housekeeping rule (channel) message ("
                    + std::to_string (return_value) + ").");
                break;
            }

            // only for testing (southbound-interface-test::create_housekeeping_rule_channel)
            // HousekeepingCreateChannelRaw create_channel_rule;
            // create_channel_rule.m_rule_id = 10;
            // create_channel_rule.m_rule_type =
            // static_cast<int>(HousekeepingOperation::create_channel);
            // create_channel_rule.m_channel_id = 123;
            // create_channel_rule.m_context_definition = static_cast<int> (ContextType::POSIX);
            // create_channel_rule.m_workflow_id = 100;
            // create_channel_rule.m_operation_type = static_cast<uint32_t> (POSIX::read);
            // create_channel_rule.m_operation_context = static_cast<uint32_t> (POSIX::no_op);

            // create differentiation properties vector
            std::vector<long> diff_config {};
            diff_config.push_back (static_cast<long> (create_channel_rule.m_context_definition));
            diff_config.push_back (static_cast<long> (create_channel_rule.m_workflow_id));
            diff_config.push_back (static_cast<long> (create_channel_rule.m_operation_type));
            diff_config.push_back (static_cast<long> (create_channel_rule.m_operation_context));

            // create HousekeepingRule object
            HousekeepingRule hsk_rule { create_channel_rule.m_rule_id,
                HousekeepingOperation::create_channel,
                create_channel_rule.m_channel_id,
                -1,
                diff_config };

            // submit request to the Agent module to create and employ the HousekeepingRule
            status = this->m_agent_ptr->employ_housekeeping_rule (hsk_rule);

            break;
        }

        case ControlPlaneOperationSubtype::hsk_create_object: {
            // create HousekeepingCreateObjectRaw object for HousekeepingRule
            HousekeepingCreateObjectRaw create_object_rule {};

            { // entering critical section
                // acquire read lock
                std::unique_lock<std::mutex> read_lock (this->m_socket_read_lock);
                // read structure from socket
                return_value
                    = ConnectionHandler::socket_read (&create_object_rule, operation.m_size);
            }

            // validate number of bytes read
            if (return_value <= 0) {
                status = PStatus::Error ();
                Logging::log_error ("Error while reading housekeeping rule (object) message ("
                    + std::to_string (return_value) + ").");
                break;
            }

            // only for testing (southbound-interface-test::create_housekeeping_rule_channel)
            // HousekeepingCreateObjectRaw create_object_rule;
            // create_object_rule.m_rule_id = 11;
            // create_object_rule.m_rule_type =
            // static_cast<int>(HousekeepingOperation::create_object);
            // create_object_rule.m_channel_id = 123;
            // create_object_rule.m_enforcement_object_id = 10;
            // create_object_rule.m_context_definition = static_cast<int> (ContextType::POSIX);
            // create_object_rule.m_operation_type = static_cast<uint32_t> (POSIX::read);
            // create_object_rule.m_operation_context = static_cast<uint32_t> (POSIX::no_op);
            // create_object_rule.m_enforcement_object_type = static_cast<long>
            // (EnforcementObjectType::drl); create_object_rule.m_property_first = 10000;
            // create_object_rule.m_property_second = 120000;

            // create differentiation properties vector
            std::vector<long> config {};
            // push differentiation properties to container
            config.push_back (static_cast<long> (create_object_rule.m_context_definition));
            config.push_back (static_cast<long> (create_object_rule.m_operation_type));
            config.push_back (static_cast<long> (create_object_rule.m_operation_context));
            // push enforcement object properties to container
            config.push_back (create_object_rule.m_enforcement_object_type);
            config.push_back (create_object_rule.m_property_first);
            config.push_back (create_object_rule.m_property_second);

            // create HousekeepingRule object
            HousekeepingRule hsk_rule { static_cast<std::uint64_t> (create_object_rule.m_rule_id),
                HousekeepingOperation::create_object,
                create_object_rule.m_channel_id,
                create_object_rule.m_enforcement_object_id,
                config };

            // submit request to the Agent module to create and employ the HousekeepingRule
            status = this->m_agent_ptr->employ_housekeeping_rule (hsk_rule);

            break;
        }

        case ControlPlaneOperationSubtype::no_op:
        case ControlPlaneOperationSubtype::collect_stats_rocksdb:
        case ControlPlaneOperationSubtype::collect_stats_tensorflow:
        case ControlPlaneOperationSubtype::collect_stats_global:
        case ControlPlaneOperationSubtype::collect_stats_metadata_data:
        case ControlPlaneOperationSubtype::collect_stats_mds:
            throw std::runtime_error ("Unsupported operation type.");
    }

    // validate if the rule was successfully inserted in the HousekeepingTable
    status.is_ok () ? response.m_message = static_cast<int> (AckCode::ok)
                    : response.m_message = static_cast<int> (AckCode::error);

    // send an ACK message to the controller
    { // entering critical section
        // acquire write lock
        std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
        return_value = ConnectionHandler::socket_write (&response, sizeof (ACK));

        if (return_value <= 0) {
            Logging::log_error ("Error while writing ACK message to control plane ("
                + std::to_string (return_value) + ").");
        }
    }

    return return_value;
}

// create_differentiation_rule. Create a create_differentiation_rule for a given Channel or
// EnforcementObject.
ssize_t SouthboundConnectionHandler::create_differentiation_rule (const ControlOperation& operation)
{
    throw std::runtime_error ("create_differentiation_rule not implemented.");
}

// create_enforcement_rule call. Create an EnforcementRule to configure existing EnforcementObjects.
ssize_t SouthboundConnectionHandler::create_enforcement_rule (const ControlOperation& operation)
{
    PStatus status;
    ACK response {};
    ssize_t return_value;

    // Receive Phase
    EnforcementRuleRaw enforcement_rule {};

    { // entering critical section
        // acquire read lock
        std::unique_lock<std::mutex> read_lock (this->m_socket_read_lock);
        // read binary structure
        return_value = ConnectionHandler::socket_read (&enforcement_rule, operation.m_size);
    }

    // validate number of bytes read
    if (return_value <= 0) {
        status = PStatus::Error ();
        Logging::log_error ("Error while reading create enforcement rule message ("
            + std::to_string (return_value) + ").");
    }

    // only for testing (southbound-interface-test::create_enforcement_rule)
    // enforcement_rule.m_rule_id = 10;
    // enforcement_rule.m_channel_id = 123;
    // enforcement_rule.m_enforcement_object_id = 10;
    // enforcement_rule.m_enforcement_operation = static_cast<int> (DRLConfiguration::rate);
    // enforcement_rule.m_property_first = 150000;

    if (return_value > 0) {
        // create enforcement rule with raw constructor
        EnforcementRule rule { enforcement_rule };
        // submit rule to the Agent module
        status = this->m_agent_ptr->employ_enforcement_rule (rule);
    }

    // validate if the rule was successfully enforced
    status.is_ok () ? response.m_message = static_cast<int> (AckCode::ok)
                    : response.m_message = static_cast<int> (AckCode::error);

    // send an ACK message to the controller
    { // entering critical section
        // acquire write lock
        std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
        return_value = ConnectionHandler::socket_write (&response, sizeof (ACK));
    }

    if (return_value <= 0) {
        Logging::log_error ("Error while writing ACK message to control plane ("
            + std::to_string (return_value) + ").");
    }

    return return_value;
}

// aggregate_kvs_statistics call. Aggregate RocksDB-based stats into a StatsSilkRaw object.
PStatus SouthboundConnectionHandler::aggregate_kvs_statistics (
    const std::map<long, std::vector<double>>& detailed_instance_stats,
    StatsSilkRaw& stats_kvs_raw)
{
    PStatus status = PStatus::Error ();

    int valid_l0_compaction_io_flows = 0;
    double rate_fg_tasks = 0;
    double rate_bg_tasks_flush = 0;
    double rate_bg_tasks_compaction_l0 = 0;
    double rate_bg_tasks_compaction_lN = 0;

    if (!detailed_instance_stats.empty ()) {
        for (auto& entry : detailed_instance_stats) {
            long entry_flow_id = entry.first;
            // aggregate statistics for foreground I/O flows
            if (entry_flow_id < 2000) {
                rate_fg_tasks += entry.second[static_cast<int> (LSM_KVS_DETAILED::foreground)
                    % paio::core::lsm_kvs_detailed_size];
            }
            // aggregate statistics for the background flush I/O flow
            else if (entry_flow_id == 2000) {
                rate_bg_tasks_flush += entry.second[static_cast<int> (LSM_KVS_DETAILED::bg_flush)
                    % paio::core::lsm_kvs_detailed_size];
            }
            // aggregate statistics for the background compaction I/O flows
            else if (entry_flow_id > 2000) {
                // high-priority compactions
                auto previous_value = rate_bg_tasks_compaction_l0;
                rate_bg_tasks_compaction_l0
                    += (entry.second[static_cast<int> (LSM_KVS_DETAILED::bg_compaction_L0_L0)
                            % paio::core::lsm_kvs_detailed_size]
                        + entry.second[static_cast<int> (LSM_KVS_DETAILED::bg_compaction_L0_L1)
                            % paio::core::lsm_kvs_detailed_size]);

                // validate if I/O flow had high-priority compactions in the last instance since
                // high-priority compactions run sequentially, only count the number of I/O flows
                // that actually have valid L0 compactions
                if (rate_bg_tasks_compaction_l0 > previous_value) {
                    // update total number of background compaction I/O flows
                    valid_l0_compaction_io_flows++;
                }

                // low-priority compactions
                rate_bg_tasks_compaction_lN
                    += (entry.second[static_cast<int> (LSM_KVS_DETAILED::bg_compaction_L1_L2)
                            % paio::core::lsm_kvs_detailed_size]
                        + entry.second[static_cast<int> (LSM_KVS_DETAILED::bg_compaction_L2_L3)
                            % paio::core::lsm_kvs_detailed_size]
                        + entry.second[static_cast<int> (LSM_KVS_DETAILED::bg_compaction_LN)
                            % paio::core::lsm_kvs_detailed_size]);
            }
        }

        // compute performance of foreground workflows
        stats_kvs_raw.m_fg_tasks = rate_fg_tasks;
        // compute performance of background flush workflows
        stats_kvs_raw.m_bg_tasks_flush = rate_bg_tasks_flush;
        // compute performance of compactions L0 workflows
        stats_kvs_raw.m_bg_tasks_compaction_l0 = rate_bg_tasks_compaction_l0 != 0
            ? (rate_bg_tasks_compaction_l0 / valid_l0_compaction_io_flows)
            : rate_bg_tasks_compaction_l0;
        // compute performance of compactions LN workflows
        stats_kvs_raw.m_bg_tasks_compaction_lN = rate_bg_tasks_compaction_lN;

        status = PStatus::OK ();
    } else {
        Logging::log_error ("aggregate_kvs_statistics: channel stats is empty; "
                            "could not compute statistics.");
    }

    return status;
}

// aggregate_tf_statistics call. Aggregate TensorFlow-based stats into a StatsTensorFlowRaw object.
PStatus SouthboundConnectionHandler::aggregate_tf_statistics (
    const std::map<long, std::vector<double>>& detailed_instance_stats,
    StatsTensorFlowRaw& stats_obj)
{
    PStatus status = PStatus::Error ();
    double read_io_rate = 0;
    double write_io_rate = 0;

    // validate if detailed_instance_stats container is not empty
    if (!detailed_instance_stats.empty ()) {
        // aggregate statistics for foreground I/O flows
        for (auto& entry : detailed_instance_stats) {
            Logging::log_debug (
                "Aggregating statistics of channel-" + std::to_string (entry.first));
            // aggregate read rate (including no_op)
            read_io_rate += entry.second[static_cast<int> (POSIX::no_op) % paio::core::posix_size];
            read_io_rate += entry.second[static_cast<int> (POSIX::read) % paio::core::posix_size];
            read_io_rate += entry.second[static_cast<int> (POSIX::pread) % paio::core::posix_size];
            read_io_rate
                += entry.second[static_cast<int> (POSIX::pread64) % paio::core::posix_size];
            // aggregate write rate
            write_io_rate += entry.second[static_cast<int> (POSIX::write) % paio::core::posix_size];
            write_io_rate
                += entry.second[static_cast<int> (POSIX::pwrite) % paio::core::posix_size];
            write_io_rate
                += entry.second[static_cast<int> (POSIX::pwrite64) % paio::core::posix_size];
        }

        // update aggregated statistic of StatsTensorFlowRaw object
        stats_obj.m_read_rate = read_io_rate;
        stats_obj.m_write_rate = write_io_rate;

        status = PStatus::OK ();
    } else {
        Logging::log_error ("aggregate_tf_statistics: detailed stats container is empty; "
                            "could not compute statistics.");
    }

    return status;
}

// aggregate_global_statistics call. Aggregate global stats into a StatsGlobalRaw object.
PStatus SouthboundConnectionHandler::aggregate_global_statistics (
    const std::map<long, std::vector<double>>& detailed_instance_stats,
    StatsGlobalRaw& stats_global)
{
    PStatus status = PStatus::Error ();
    double global_rate = 0;

    // validate if detailed_instance_stats container is not empty
    if (!detailed_instance_stats.empty ()) {
        // aggregate statistics for foreground I/O flows
        for (auto& entry : detailed_instance_stats) {
            // aggregate all operations (including no_op)
            global_rate += std::accumulate (entry.second.begin (), entry.second.end (), 0.0);
        }

        // update aggregated statistic of StatsGlobalRaw object
        stats_global.m_total_rate = global_rate;

        status = PStatus::OK ();
    } else {
        Logging::log_error ("aggregate_global_statistics: detailed stats container is empty; "
                            "could not compute statistics.");
    }

    return status;
}

// aggregate_global_statistics call. Aggregate global stats into a StatsDataMetadataRaw object.
PStatus SouthboundConnectionHandler::aggregate_metadata_data_statistics (
    const std::map<long, std::vector<double>>& detailed_instance_stats,
    StatsDataMetadataRaw& stats_data_metadata)
{
    PStatus status = PStatus::Error ();
    double data_rate = 0;
    double metadata_rate = 0;

    // validate if detailed_instance_stats container is not empty
    if (!detailed_instance_stats.empty ()) {
        // aggregate statistics for data and metadata flows
        for (auto& entry : detailed_instance_stats) {
            data_rate += entry.second[static_cast<int> (POSIX_META::data_op)
                % paio::core::posix_meta_size];
            metadata_rate += entry.second[static_cast<int> (POSIX_META::meta_op)
                % paio::core::posix_meta_size];
        }

        // update aggregated statistic of StatsDataMetadataRaw object
        stats_data_metadata.m_total_data_rate = data_rate;
        stats_data_metadata.m_total_metadata_rate = metadata_rate;

        status = PStatus::OK ();
    } else {
        Logging::log_error ("aggregate_metadata_data_statistics: detailed stats container is "
                            "empty; could not compute statistics.");
    }

    return status;
}

// execute_housekeeping_rules call. Execute postponed or pending HousekeepingRules.
ssize_t SouthboundConnectionHandler::execute_housekeeping_rules (const ControlOperation& operation)
{
    PStatus status;
    ACK response {};
    ssize_t return_value;

    // execute all pending housekeeping rules
    status = this->m_agent_ptr->execute_housekeeping_rules ();

    // validate if the rules were executed
    status.is_error () ? response.m_message = static_cast<int> (AckCode::error)
                       : response.m_message = static_cast<int> (AckCode::ok);

    // send an ACK message to the controller
    { // entering critical section
        // acquire write lock
        std::unique_lock<std::mutex> write_lock (this->m_socket_write_lock);
        return_value = ConnectionHandler::socket_write (&response, sizeof (ACK));
    }

    // validate return value
    if (return_value <= 0) {
        Logging::log_error (
            "Error while writing ack message (" + std::to_string (return_value) + ").");
    }

    return return_value;
}

// remove_rule call. Remove rule, which can be of type Housekeeping, Differentiation, or
// Enforcement.
ssize_t SouthboundConnectionHandler::remove_rule (const ControlOperation& operation)
{
    throw std::runtime_error ("remove_rule not implemented.");
}

// handle_control_operation call. Handle operation received from the control plane.
ssize_t SouthboundConnectionHandler::handle_control_operation (const ControlOperation& operation,
    const bool& debug)
{
    ssize_t return_value;

    // log control operation message
    ConnectionHandler::log_control_operation (debug, operation);

    switch (static_cast<ControlPlaneOperationType> (operation.m_operation_type)) {
        case ControlPlaneOperationType::mark_stage_ready:
            // call mark_stage_as_ready
            return_value = this->mark_stage_as_ready (operation);
            break;

        case ControlPlaneOperationType::collect_stats:
            // call collect_statistics
            return_value = this->collect_statistics (operation);
            break;

        case ControlPlaneOperationType::collect_detailed_stats:
            // call collect_instance_statistics
            return_value = this->collect_instance_statistics (operation);
            break;

        case ControlPlaneOperationType::create_hsk_rule:
            // call create_housekeeping_rule
            return_value = this->create_housekeeping_rule (operation);
            break;

        case ControlPlaneOperationType::create_dif_rule:
            // call create_differentiation_rule
            return_value = this->create_differentiation_rule (operation);
            break;

        case ControlPlaneOperationType::create_enf_rule:
            // call create_enforcement_rule
            return_value = this->create_enforcement_rule (operation);
            break;

        case ControlPlaneOperationType::exec_hsk_rules:
            // call execute_housekeeping_rules
            return_value = this->execute_housekeeping_rules (operation);
            break;

        case ControlPlaneOperationType::remove_rule:
            // call remove_rule
            return_value = this->remove_rule (operation);
            break;

        default:
            throw std::logic_error ("SouthboundConnectionHandler: unknown operation type");
    }

    // logging return-value message
    ConnectionHandler::log_return_value (debug, operation, return_value);

    return return_value;
}

// listen call. Listen for incoming ControlOperations from the control plane.
void SouthboundConnectionHandler::listen (const bool& debug)
{
    ControlOperation control_operation {};

    // read stage handshake operation from socket
    auto read_bytes = this->read_control_operation_from_socket (&control_operation);

    // validate bytes read and connection state
    while (read_bytes > 0 && this->m_stage_shutdown->load () == false) {
        // Receive and handle the rule submitted by the controller
        read_bytes = this->handle_control_operation (control_operation, debug);

        // validate bytes read
        if (read_bytes <= 0) {
            throw std::runtime_error ("ConnectionManager: failed to receive control operation.");
        }

        // read next rule from the socket
        read_bytes = this->read_control_operation_from_socket (&control_operation);
    }
}

} // namespace paio::networking
