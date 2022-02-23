/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CHANNEL_DEFAULT_HPP
#define PAIO_CHANNEL_DEFAULT_HPP

#include <paio/enforcement/channel.hpp>
#include <paio/enforcement/completion_queue.hpp>
#include <paio/enforcement/submission_queue.hpp>
#include <paio/statistics/channel_statistics.hpp>
#include <thread>

using namespace paio::differentiation;
using namespace paio::options;
using namespace paio::statistics;
using namespace paio::utils;

namespace paio::enforcement {

/**
 * ChannelDefault class.
 * A ChannelDefault provides a stream-like abstraction through where requests flow, and respects to
 * the default Channel object in PAIO data plane stages. It extends the Channel interface, and thus,
 * each channel contains one or more enforcement objects, as well as a differentiation rule that
 * maps a request to the respective enforcement object, so it can be enforced. The combination of
 * channels and enforcement objects is designed to ease the implementation of new storage services,
 * while promoting their re-utilization and applicability.
 * Internally, it is organized with several instance variables, categorized in three axis, namely
 * channel management variables, statistic collection variables, and parallelism-related variables.
 * - Channel management variables
 *  - m_channel_id: Defines the channel identifier.
 *  - m_completion_queue: CompletionQueue object that stores the results of enforced requests.
 *  - m_submission_queue: SubmissionQueue object that handles I/O requests (enforces them).
 *  - m_use_fast_path: Boolean that defines if the Channel should use fast path option to enforce
 *  requests (directly enforce them); or use the queue option, where requests are enqueued and
 *  enforced by a separate thread.
 *  - m_ticket_id: Unique ticket identifier.
 *  - m_object_id_to_token_linkers: Container that maps EnforcementObject identifier to the
 *  respective differentiation token.
 *  - object_token_linker_lock: Mutex for controlling access to m_object_id_to_token_linkers
 *  container.
 * - Statistic collection variables
 *  - m_collect_channel_statistics: Boolean that defines if statistic collection should be made at
 *  the Channel level.
 *  - m_collect_object_statistics: Boolean that defines if statistic collection should be made at
 *  the EnforcementObject level.
 *  - m_channel_statistics: Container to store the statistics of this Channel.
 * - Parallelism-related variables
 *  - m_worker_pool: Thread-pool for handling requests from the SubmissionQueue.
 *  - m_parallelism_level: Number of threads to be used in the m_worker_pool.
 * TODO:
 *  - change structs to specific/dedicated objects in collect_object_statistics,
 *  collect_general_statistics, collect_detailed_statistics, and collect_statistic_entry methods;
 *  - create a dedicated object for specifying each configuration or set of configurations in
 *  create_enforcement_object and configure_enforcement_object methods;
 *  - integrate collect_statistic_entry method in Core class;
 *  - add tests for create_new_enforcement_object_linker and get_enforcement_object_diff_token
 *  methods.
 */
class ChannelDefault : public Channel {
    // friend classes of ChannelDefault
    friend class ChannelDefaultTest;

private:
    uint32_t m_channel_id { 0 };
    CompletionQueue m_completion_queue {};
    SubmissionQueue m_submission_queue { &m_completion_queue };
    bool m_use_fast_path { (option_default_channel_mode == ChannelMode::fast_path) };
    std::atomic<uint64_t> m_ticket_id { 0 };
    std::vector<std::pair<long, diff_token_t>> m_object_id_to_token_linkers {};
    std::mutex object_token_linker_lock;

    // Statistic-related variables
    bool m_collect_channel_statistics { option_default_channel_statistic_collection };
    bool m_collect_object_statistics { option_default_object_statistic_collection };
    ChannelStatistics m_channel_statistics { "channel-stats",
        option_default_statistic_metric,
        option_default_statistic_classifier };

    // Parallelism-related variables
    std::vector<std::thread> m_worker_pool;
    int m_parallelism_level { option_default_channel_thread_pool_size };

    /**
     * start_workers: Spawn worker threads to handle requests from the SubmissionQueue in parallel.
     */
    void start_workers ();

    /**
     * join_workers: Join worker threads from m_worker_pool.
     */
    void join_workers ();

    /**
     * gen_ticket_id: Atomically generate a ticket identifier.
     * @return Return new ticket identifier.
     */
    uint64_t gen_ticket_id ();

    /**
     * build_ticket: Build Ticket object based on the request's context and buffer.
     * Leverages from Return Value Optimization (RVO).
     * @param context Context object that includes all necessary classifiers to enforce the request.
     * @param buf Pointer to a buffer that contains all request data/metadata.
     * @param size Size of the buffer.
     * @return Returns a new Ticket object to be enforced.
     */
    Ticket build_ticket (const Context& context, const void* buf, const std::size_t& size);

    /**
     * build_object_differentiation_token: Build differentiation token to select which enforcement
     * object to use.
     * @param operation_type Operation type classifier.
     * @param operation_context Operation context classifier.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_object_differentiation_token (const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value);

    /**
     * create_new_enforcement_object_linker: Upon the creation of an EnforcementObject, the method
     * maps the enforcement object identifier with the generate differentiation token. This eases
     * the search for the correct enforcement object to operate upon requests from the control
     * plane. This method is thread-safe.
     * @param object_id Enforcement object identifier.
     * @param object_token Differentiation token of the enforcement object.
     */
    void create_new_enforcement_object_linker (const long& object_id,
        const diff_token_t& object_token);

    /**
     * get_enforcement_object_diff_token: Get the differentiation token of a given enforcement
     * object with the identifier 'object_id'.
     * This method is thread-safe.
     * @param object_id Enforcement object identifier.
     * @return Returns the corresponding differentiation token. If the enforcement object's
     * identifier does not exists, it returns -1.
     */
    diff_token_t get_enforcement_object_diff_token (const long& object_id);

public:
    /**
     * ChannelDefault default constructor.
     */
    ChannelDefault ();

    /**
     * ChannelDefault parameterized constructor.
     * Initializes channel identifier and main configurations (statistic collection at channel and
     * enforcement objects; type of channel to use).
     */
    ChannelDefault (const uint32_t& channel_id,
        const bool& use_fast_path,
        const bool& collect_channel_statistics,
        const bool& collect_object_statistics);

    /**
     * ChannelDefault default destructor.
     */
    ~ChannelDefault () override;

    /**
     * get_channel_id: Get channel identifier.
     * @return Returns a copy of m_channel_identifier parameter.
     */
    uint32_t get_channel_id () const;

    /**
     * create_enforcement_object: Create new EnforcementObject and add it to the enforcement
     * object's list in the SubmissionQueue.
     * @param enforcement_object_id EnforcementObject identifier.
     * @param differentiation_pair Defines the classifiers of the EnforcementObject (to be used in
     * I/O classification and differentiation).
     * @param type Type of the EnforcementObject. Currently, supports DRL and Noop objects.
     * @param configurations Container that holds the initial configurations (initial state) of the
     * enforcement object.
     * @return Returns PStatus::OK if enforcement object was correctly created; and PStatus::Error
     * otherwise.
     */
    PStatus create_enforcement_object (const long& enforcement_object_id,
        const ObjectDifferentiationPair& differentiation_pair,
        const EnforcementObjectType& object_type,
        const std::vector<long>& configurations) override;

    /**
     * configure_enforcement_object: Configure enforcement object. The method receives an
     * EnforcementObject id (from the control plane) and converts it into the respective
     * differentiation token.
     * @param enforcement_object_id Enforcement object id to perform the configuration.
     * @param config Configuration identifier i.e., identifies which tuning knob to perform.
     * @param configurations List of configurations to be set.
     * @return Returns PStatus::OK if configurations were made successfully, and PStatus::Error
     * otherwise.
     */
    PStatus configure_enforcement_object (const long& enforcement_object_id,
        const int& config,
        const std::vector<long>& configurations) override;

    /**
     * channel_enforce: This method is used to employ enforcement mechanisms over I/O requests.
     * First, based on the request classifiers obtained from the Context object, it generates a
     * Ticket. This Ticket will contain the necessary metadata to enforce the request, as well as
     * the request's content (buffer). Then, it verifies if the fast path is enabled
     * (m_use_fast_path), namely if the Ticket should be directly enforced or enqueued in the
     * SubmissionQueue. Finally, the method also collect statistics if m_collect_channel_statistics
     * is enabled. No need for concurrency control since methods are protected in the respective
     * classes or are atomic. Specifically, build_ticket's shared variable is atomic, and the
     * remainder calls (enqueue_fast_path, enqueue, dequeue, and update_statistic_entry) are lock
     * protected in the respective classes. m_use_fast_path and m_collect_channel_statistics are
     * only set at initialization, so not thread will change it at runtime.
     * @param request_context Context object that contains all request classifiers, including total
     * operations, operation size, operation type, and operation context.
     * @param buffer Buffer that contains the request's content.
     * @param buffer_size Size of the buffer.
     * @param result Reference to a Result object to store the result of the enforcement mechanism.
     */
    void channel_enforce (const Context& request_context,
        const void* buffer,
        const std::size_t& buffer_size,
        Result& result) override;

    /**
     * collect_object_statistics: Collect statistics from enforcement object. The method receives an
     * EnforcementObject id (from the control plane) and converts it into the respective
     * differentiation token.
     * @param enforcement_object_id Enforcement object id to perform the configuration.
     * @param statistics_raw
     * @return Returns PStatus::OK if statistics were effectively collect, and PStatus::error
     * otherwise.
     */
    PStatus collect_object_statistics (const long& enforcement_object_id,
        ObjectStatisticsRaw& statistics_raw) override;

    /**
     * collect_general_statistics: Collect the global I/O statistics collected in this channel,
     * which are stored in the m_channel_statistics container.
     * This method invokes ChannelStatistics::collect (thread-safe) that computes the overall and
     * windowed metric values and stores them in the general_stats object.
     * @param general_stats Address to the object where statistics will be stored.
     * @return Returns PStatus::OK if statistics were effectively collected; and PStatus::Error
     * otherwise (e.g., if channel statistic collection is disabled).
     */
    PStatus collect_general_statistics (ChannelStatsRaw& general_stats) override;

    /**
     * collect_detailed_statistics: Collect the detailed (full) I/O statistics collected in this
     * channel, which are stored in the m_channel_statistics container.
     * This method invokes ChannelStatistics::collect_detailed_windowed_entries (thread-safe) that
     * computes the windowed metric value for each statistic entry. It will then store them in the
     * detailed_stat_entries container.
     * @param detailed_stat_entries Container that stores all statistic entries. The method expects
     * a "fresh" vector i.e., no elements in the container.
     * @return Returns PStatus::OK if statistics were effectively collected; and PStatus::Error
     * otherwise (e.g., if channel statistic collection is disabled).
     */
    PStatus collect_detailed_statistics (std::vector<double>& detailed_stat_entries) override;

    /**
     * collect_statistic_entry: Collect a single statistic entry from the m_channel_statistics
     * container. This method invokes ChannelStatistics::collect_single_entry (thread-safe) that
     * computes the overall and windowed metric values of a specific I/O operation (operation), and
     * stores them in the entry_stats object.
     * Attention: ChannelStatistics::collect_single_entry does not record the last collection
     * period, since only a single entry was collected.
     * @param entry_stats
     * @param operation Operation type to be considered.
     * @return Returns PStatus::OK if statistics were effectively collected; and PStatus::Error
     * otherwise (e.g., if channel statistic collection is disabled).
     */
    PStatus collect_statistic_entry (ChannelStatsRaw& entry_stats, const int& operation) override;

    /**
     * define_object_differentiation: Defines how requests should be differentiated i.e., which
     * request classifiers should be considered for the I/O differentiation.
     * @param operation_type Object that contains the differentiation rule to set how
     * classification and differentiation of requests is made for enforcement objects.
     * @param operation_type Boolean that defines if the operation type classifier should be
     * considered for differentiating requests (in enforcement objects).
     * @param operation_context Boolean that defines if the operation context classifier should be
     * considered for differentiating requests (in enforcement objects).
     */
    void define_object_differentiation (const bool& operation_type, const bool& operation_context);

    /**
     * to_string: Wrap ChannelDefault elements into string format.
     * @return Returns a string with the ChannelDefault information.
     */
    std::string to_string () override;
};
} // namespace paio::enforcement

#endif // PAIO_CHANNEL_DEFAULT_HPP
