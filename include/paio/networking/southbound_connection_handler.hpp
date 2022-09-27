/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_SOUTHBOUND_CONNECTION_HANDLER_HPP
#define PAIO_SOUTHBOUND_CONNECTION_HANDLER_HPP

#include <cstdio>
#include <paio/core/agent.hpp>
#include <paio/core/interface_definitions.hpp>
#include <paio/networking/connection_handler.hpp>
#include <unistd.h>

namespace paio::networking {

/**
 * SouthboundConnectionHandler class.
 * The SouthboundConnectionHandler bridges the communication between the data plane stage and the
 * control plane, and establishes the operators that can be used for direct control (e.g., policy
 * definition, monitoring, storage fine-tuning).
 * TODO:
 *  - remove aggregate_kvs_statistics and aggregate_tf_statistics, since they are tightly coupled
 *  to the use cases of the PAIO paper;
 *  - update collect_instance_statistics to use be more generic.
 */
class SouthboundConnectionHandler : public ConnectionHandler {

    friend class SouthboundInterfaceTest;

private:
    std::mutex m_socket_read_lock;
    std::mutex m_socket_write_lock;
    std::shared_ptr<std::atomic<bool>> m_stage_shutdown { nullptr };

    /**
     * aggregate_kvs_statistics: Auxiliary method to aggregate and compute the statistics of use
     * case 1 (of PAIO's paper) before dispatching them to the control plane.
     * This will soon become deprecated, as it is tightly coupled to the use case.
     * @param detailed_channel_stats Container that comprises the raw metrics collected from each
     * channel installed in the data plane stage.
     * @param stats_kvs_raw RAW object containing the aggregated results of the statistics.
     * @return Returns PStatus::OK if the aggregation was successfully made; PStatus::Error
     * otherwise.
     */
    PStatus aggregate_kvs_statistics (
        const std::map<long, std::vector<double>>& detailed_channel_stats,
        StatsSilkRaw& stats_kvs_raw);

    /**
     * aggregate_tf_statistics: Auxiliary method to aggregate and compute the statistics of use
     * case 1 (of PAIO's paper) before dispatching them to the control plane.
     * This will soon become deprecated, as it is tightly coupled to the use case.
     * @param detailed_channel_stats Container that comprises the raw metrics collected from each
     * channel installed in the data plane stage.
     * @param stats_tf_raw RAW object containing the aggregated results of the statistics.
     * @return Returns PStatus::OK if the aggregation was successfully made; PStatus::Error
     * otherwise.
     */
    PStatus aggregate_tf_statistics (
        const std::map<long, std::vector<double>>& detailed_instance_stats,
        StatsTensorFlowRaw& stats_tf);

    /**
     * aggregate_global_statistics: Auxiliary method to aggregate and compute the statistics in
     * all channels before dispatching them to the control plane.
     * @param detailed_channel_stats Container that comprises the raw metrics collected from each
     * channel installed in the data plane stage.
     * @param stats_global RAW object containing the aggregated results of the statistics.
     * @return Returns PStatus::OK if the aggregation was successfully made; PStatus::Error
     * otherwise.
     */
    PStatus aggregate_global_statistics (
        const std::map<long, std::vector<double>>& detailed_instance_stats,
        StatsGlobalRaw& stats_global);

    // TODO: Tasks pending completion -@gsd at 6/14/2022, 10:32:29 PM
    // implement the function to aggregate statistics based on data and metadata operations
    PStatus aggregate_metadata_data_statistics (
        const std::map<long, std::vector<double>>& detailed_instance_stats,
        StatsDataMetadataRaw& stats_data_metadata);

    // TODO: Tasks pending completion -@gsd at 6/14/2022, 10:32:58 PM
    // implement the function to aggregate statistics based on their MDS
    PStatus aggregate_mds_statistics ();

    /**
     * mark_stage_as_ready: this method marks the data plane stage ready to receive requests from
     * the target I/O layer.
     * @param control_operation ControlOperation object that contains the size of the message that
     * will be received.
     * @return Returns the amount of written bytes to the socket.
     */
    ssize_t mark_stage_as_ready (const ControlOperation& operation);

    /**
     * create_housekeeping_rule: Create a HousekeepingRule to be inserted in the PAIO Stage.
     * @param operation ControlOperation structure that contains important information about the
     * incoming Control plane's instruction.
     * @return Returns the amount of written bytes to the socket.
     */
    ssize_t create_housekeeping_rule (const ControlOperation& operation);

    /**
     * create_differentiation_rule: Create a create_differentiation_rule to for a given Channel or
     * an EnforcementObject.
     * @param operation ControlOperation structure that contains important information about the
     * incoming Control plane's instruction.
     * @return Returns the amount of written bytes to the socket.
     */
    ssize_t create_differentiation_rule (const ControlOperation& operation);

    /**
     * create_enforcement_rule: Create an EnforcementRule to configure existing EnforcementObjects.
     * @param operation ControlOperation structure that contains important information about the
     * incoming Control plane's instruction.
     * @return Returns the amount of written bytes to the socket.
     */
    ssize_t create_enforcement_rule (const ControlOperation& operation);

    /**
     * collect_statistics: Collect general statistics from the data plane.
     * @param operation ControlOperation structure that contains important information about the
     * incoming control plane's instruction.
     * @return Returns the number of bytes written to the socket.
     */
    ssize_t collect_statistics (const ControlOperation& operation);

    /**
     * collect_instance_statistics: Collect statistics from data plane instances.
     * @param operation ControlOperation structure that contains important information about the
     * incoming control plane's instruction.
     * Additionally, in the subtype field, identify the use case (1 or 2) the collection is intended
     * to (this needs to be adjusted, since it is highly hardcoded).
     * @return Returns the number of bytes written to the socket.
     */
    ssize_t collect_instance_statistics (const ControlOperation& operation);

    /**
     * execute_housekeeping_rules: Execute postponed/pending HousekeepingRules.
     * @param operation ControlOperation structure that contains important information about the
     * incoming control plane's instruction.
     * @return Returns the number of bytes written to the socket.
     */
    ssize_t execute_housekeeping_rules (const ControlOperation& operation);

    /**
     * remove_rule: Remove rule, which can be of type Housekeeping, Differentiation, or Enforcement.
     * @param operation ControlOperation structure that contains important information about the
     * incoming control plane's instruction.
     * @return Returns the number of bytes written to the socket.
     */
    ssize_t remove_rule (const ControlOperation& operation);

public:
    /**
     * SouthboundConnectionHandler default constructor.
     */
    SouthboundConnectionHandler ();

    /**
     * SouthboundConnectionHandler fully parameterized constructor.
     * @param connection_options Defines the main options to be used to establish the connection
     * between the data plane stage and the SDS control plane.
     * @param agent_ptr Shared pointer to the Agent object.
     * @param shutdown Shared pointer to the atomic boolean that indicates if the data plane stage
     * should move to a shutdown state.
     */
    SouthboundConnectionHandler (const ConnectionOptions& connection_options,
        std::shared_ptr<Agent> agent_ptr,
        std::shared_ptr<std::atomic<bool>> shutdown);

    /**
     * SouthboundConnectionHandler parameterized constructor.
     * @param agent_ptr Shared pointer to the Agent object.
     * @param shutdown Shared pointer to the atomic boolean that indicates if the data plane stage
     * should move to a shutdown state.
     */
    SouthboundConnectionHandler (std::shared_ptr<Agent> agent_ptr,
        std::shared_ptr<std::atomic<bool>> shutdown);

    /**
     * SouthboundConnectionHandler default destructor.
     */
    ~SouthboundConnectionHandler () override;

    /**
     * read_control_operation_from_socket: read ControlOperation object from socket (which is
     * connected to a SDS control plane).
     * The method is thread-safe, i.e., ensures that no other thread is reading from the socket.
     * @param operation ControlOperation structure that contains important information about the
     * incoming Control plane's instruction.
     * @return If the operation is successful, returns the number of bytes read from the socket
     * (greater or equal to zero); otherwise returns -1.
     */
    ssize_t read_control_operation_from_socket (ControlOperation* operation) override;

    /**
     * handle_control_operation: After the connection is established between the data plane stage
     * and the control plane, this function receives rules from the controller and applies them to
     * the current data plane stage. In the SouthboundConnectionHandler, rules can be of all types
     * except for stage_handshake; otherwise it throws a runtime_error exception.
     * @param operation ControlOperation structure that contains the metadata of received rules.
     * @param debug If true, prints the received rules through the logging library.
     * @return Returns the number of bytes written to the control plane. If <= 0, then the
     * operation was not achieved.
     * @throws std::logic_error if the operation is not of type stage_handshake.
     */
    ssize_t handle_control_operation (const ControlOperation& operation,
        const bool& debug) override;

    /**
     * listen: this function is used to listen to incoming operations from the control plane.
     * @param debug If true, prints the received rules through the logging library.
     */
    void listen (const bool& debug) override;
};
} // namespace paio::networking

#endif // PAIO_SOUTHBOUND_CONNECTION_HANDLER_HPP
