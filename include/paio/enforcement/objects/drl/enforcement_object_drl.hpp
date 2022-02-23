/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_ENFORCEMENT_OBJECT_DRL_HPP
#define PAIO_ENFORCEMENT_OBJECT_DRL_HPP

#include <paio/core/context_propagation_definitions.hpp>
#include <paio/enforcement/objects/drl/enforcement_object_drl_options.hpp>
#include <paio/enforcement/objects/drl/token_bucket.hpp>
#include <paio/enforcement/objects/enforcement_object.hpp>
#include <paio/options/options.hpp>
#include <paio/utils/logging.hpp>
#include <thread>

namespace paio::enforcement {

/**
 * DRLConfiguration class.
 * Configurations of the DynamicRateLimiter object, to be used on 'configure'.
 *  - DRLConfiguration::init - set DRL initialization parameters;
 *  - DRLConfiguration::rate - set new rate for DRL;
 *  - DRLConfiguration::refill - set new refill period value.
 * fixme: change DRLConfiguration::refill to 3
 */
enum class DRLConfiguration { init = 1, rate = 2, refill = 4 };

/**
 * DynamicRateLimiter class. Inherited from EnforcementObject class.
 * The DynamicRateLimiter enforcement object is a performance-oriented storage service that allows
 * SDS clients to fine-tune the performance at which requests are served.
 * Currently, this enforcement object allows to fine-tune the performance rate of I/O workflows,
 * and collect statistics to improve the enforcement object by submit them back to the SDS control
 * plane. This enforcement object is thread-safe, and can be shared over different channels.
 * Each of these, is used to control the rate of a specific type of workflows. If one wants to have
 * different rates for different types of request, it should use two separate DynamicRateLimiter
 * objects (e.g., one for the high-priority flows, and another for the low-priority flows).
 * Several configurations can be specified in the DRL options file (options_object_drl.hpp). Other
 * configurations can be fine-tuned at runtime using the obj_configure call.
 * It comprises different instance parameters:
 *  - m_collect_statistics: Enable/disable the collection of statistics from the DRL enforcement
 *  object (more specifically, from the token-bucket).
 *  - m_bucket: Token-bucket that controls the rate of I/O requests.
 *  - m_refill_thread: Background thread used for refilling the token-bucket (only used if
 *  TokenBucketType::threaded).
 *  - m_cost_per_request: Token cost of each I/O request.
 *  - m_token_bucket_rate: Used to estimate the I/O cost of a request (future work).
 *  - m_previous_estimated_cost: Used to estimate the I/O cost of a request (future work).
 *  - m_convergence_factor: Used to estimate the I/O cost of a request (future work).
 *  TODO:
 *   - add token-bucket statistics options as an argument for initialization in obj_configure and
 *   initialize methods (temporarily set to false on obj_configure);
 *   - validate configure_refill_window: number of tokens passed to refill-size, token-count, and
 *   capacity should be (first) denormalized;
 *   - improve basic_io_cost: attribute different costs to requests based on their workflow-id,
 *   type, context, and size;
 *   - improve estimate_io_cost: estimate the cost of a request based on previous estimations;
 *   - change DRLConfiguration::refill to 3 (in enum class definition);
 *   - change structs to specific/dedicated objects in obj_collect_statistics method;
 *   - create a dedicated object for specifying each configuration or set of configurations in
 *   obj_configure method;
 *   - obj_collect_statistics requires further testing and validation.
 */
class DynamicRateLimiter : public EnforcementObject {
    // friend classes of DynamicRateLimiter
    friend class DrlTest;

private:
    long m_object_id { 0 };
    bool m_collect_statistics { drl_option_collect_statistics };
    TokenBucket m_bucket {};
    std::mutex m_mutex;
    std::thread m_refill_thread {};

    // I/O cost related variables
    uint32_t m_cost_per_request { 1 }; // constant cost per request in tokens
    token m_token_bucket_rate { 0 }; // rate in (tokens / time_unit)
    uint32_t m_previous_estimated_cost { 0 }; // estimated cost of the previous I/O request
    float m_convergence_factor { drl_option_convergence_factor };

    /**
     * spawn_refill_thread: This method defines how the token-bucket will be refilled.
     * If the TokenBucketType is set to Threaded, it spawns a new thread that refills the
     * token-bucket in background. If the TokenBucketType is set to Normal, than it proceeds without
     * additional threads, and the refill process is made by validating if the refill time has
     * passed upon each try_consume call.
     * @param refill_mode TokenBucketType value that defines the token-bucket's refill mode.
     */
    void spawn_refill_thread (const TokenBucketType& bucket_type);

    /**
     * join_refill_thread: If the background refill operation is running, join m_refill_thread.
     */
    void join_refill_thread ();

    /**
     * initialize: initialize configuration of the TokenBucket, specifying its refill period,
     * initial throughput value, and the maximum throughput achieved by the token-bucket.
     * This method is thread-safe.
     * @param refill_period Refill period value throughout overall execution (expressed in
     * microseconds).
     * @param rate Maximum throughput value in IOPS or bandwidth (Byte/s).
     * @param collect_statistics Enable/disable statistic collection in the token-bucket.
     * @return Returns a PStatus::OK() if the all values were set, and PStatus::Error() if
     * initial_rate value is greater than the max_rate value.
     */
    PStatus
    initialize (const long& refill_period, const long& rate, const bool& collect_statistics);

    /**
     * configure_rate: Define the new throughput value for the DynamicRateLimiter object. This
     * value is expressed in an absolute unit (bytes/s, IOPS, ...). The method only change the
     * bucket's capacity (maximum number of tokens it can hold) and updates the current token count.
     * The refill period remains unchanged.
     * This method is thread-safe.
     * @param rate_value New throughput value.
     */
    void configure_rate (const long& rate_value);

    /**
     * configure_refill_window: Define new refill period value for the DynamicRateLimiter object.
     * This method is thread-safe.
     * @param window Refill window value.
     */
    void configure_refill_window (const long& window);

    /**
     * estimate_io_cost: Estimate the cost of the I/O request.
     * Cost(N) = CostEstimated(N) + Debt(N-1) * E
     *         = (avg_cost * size) + ((bucket_rate * exec_time(N-1)) - CostEstimated(N-1)) * E
     * exec_time(N-1) > estimated_time (N-1) ---> Debt < 0
     * exec_time(N-1) < estimated_time (N-1) ---> Debt > 0
     * exec_time(N-1) +- 5% = estimated_time (N-1) ---> Debt = 0
     * We will consider that the Ticket's io_cost is the average cost of an I/O operation, so we
     * can calculate its true cost in this method.
     * @param ticket Ticket that contains the required elements to estimate the I/O cost.
     * @return Returns the cost of the I/O request in tokens.
     */
    [[maybe_unused]] token estimate_io_cost (const Ticket& ticket);

    /**
     * basic_io_cost: Estimate the cost of the I/O request.
     * Currently, this method provides a simplistic version, where we only consider the ticket's
     * payload as the variable for estimating the I/O cost. This approach provides a coarse-grained
     * estimation of the true cost of each request. However, the control plane can continuously
     * monitor and update the rate of the TokenBucket, until the workflow rate matches the targeted
     * rate. In the future, we will consider the request's type, context, and workflow-id as well.
     * @param ticket Ticket that contains the required elements to estimate the I/O cost.
     * @return Returns the cost of the I/O request in tokens.
     */
    [[nodiscard]] token basic_io_cost (const Ticket& ticket) const;

    /**
     * get_token_bucket_rate: Get the throughput value of the DynamicRateLimiter object at any
     * given instance.
     * @return Returns the current throughput value in absolut unit (bytes/s or IOPS).
     */
    [[nodiscard]] long get_token_bucket_rate () const;

public:
    /**
     * DynamicRateLimiter default constructor.
     */
    DynamicRateLimiter ();

    /**
     * DynamicRateLimiter parameterized constructor.
     */
    DynamicRateLimiter (const long& enforcement_object_id, const bool& collect_statistics);

    /**
     * DynamicRateLimiter default destructor.
     */
    ~DynamicRateLimiter () override;

    /**
     * get_enforcement_object_id: Get the EnforcementObject's identifier.
     * @return Returns a copy of the m_object_id value.
     */
    [[nodiscard]] long get_enforcement_object_id () const override;

    /**
     * obj_enforce: Enforce the rate limiting service over a specific request.
     * This method is thread-safe, since multiple threads may be operating over the same DRL object.
     * @param ticket Represents a metadata-like object that contains a set of elements that
     * characterize the request, including the request context, size, and buffer.
     * @param result Reference to a Result object that stores the result of Noop enforcement
     * mechanism, including the request content if enabled.
     */
    void obj_enforce (const Ticket& ticket, Result& result) override;

    /**
     * obj_configure: configure internal properties of the DynamicRateLimiter enforcement object.
     * Upon receiving new enforcement commands from the SDS control plane, this method adjusts the
     * respective tuning knobs of the DynamicRateLimiter object to comply with submitted rules.
     * Current supported configurations are:
     *  - init: initialize the DynamicRateLimiter's values (token-bucket);
     *  - rate: update the throughput value of the DynamicRateLimiter object;
     *  - refill: set the token-bucket's refill window to a given value.
     * This method is thread-safe, since multiple threads may be operating over shared resources.
     * @param conf DRLConfiguration to specify the enforce the respective setting.
     * @param configurations Configuration values to be used.
     * @return Returns PStatus::OK() if the configuration was successfully made, and
     * PStatus::ERROR() otherwise.
     */
    PStatus obj_configure (int conf, const std::vector<long>& configurations) override;

    /**
     * obj_collect_statistics: Collect and submit statistics from the DynamicRateLimiter object to
     * the SDS control plane.
     * This method is thread-safe, since multiple threads may be operating over shared resources.
     * @param statistics_raw
     * @return PStatus state, being PStatus::OK if statistics were effectively collected, and
     * PStatus::Error otherwise.
     */
    PStatus obj_collect_statistics (ObjectStatisticsRaw& statistics_raw) override;

    /**
     * to_string: Generate a string with the DynamicRateLimiter settings.
     * @return Returns the DynamicRateLimiter settings in string format.
     */
    std::string to_string () override;
};
} // namespace paio::enforcement

#endif // PAIO_ENFORCEMENT_OBJECT_DRL_HPP
