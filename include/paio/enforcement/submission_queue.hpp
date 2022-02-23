/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_SUBMISSION_QUEUE_HPP
#define PAIO_SUBMISSION_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <paio/differentiation/enforcement_object_hashing_differentiation.hpp>
#include <paio/enforcement/channel.hpp>
#include <paio/enforcement/completion_queue.hpp>
#include <paio/enforcement/objects/drl/enforcement_object_drl.hpp>
#include <paio/enforcement/objects/enforcement_object.hpp>
#include <paio/enforcement/objects/noop/enforcement_object_noop.hpp>
#include <queue>
#include <sstream>
#include <thread>

using namespace paio::differentiation;
using namespace paio::options;
using namespace paio::utils;

namespace paio::enforcement {

/**
 * SubmissionQueue class.
 * The SubmissionQueue class provides the means to store request tickets, apply the respective I/O
 * mechanism, and return back to the client or enqueue in a CompletionQueue to be later fetched by
 * the client thread.
 * It comprises different instance parameters.
 * A first set of parameters, orchestrate how I/O requests are held and serviced.
 *  - m_queue: Queue that holds pointers to ticket objects;
 *  - m_queue_lock: Mutex to ensure concurrency control over m_queue;
 *  - m_is_empty: Condition variable that defines whether m_queue is empty or not;
 *  - m_timeout_dequeue: Time that defines the timeout for m_is_empty (using wait_for);
 *  - m_completion_queue: Queue that holds result objects;
 *  - m_is_running: Atomic boolean that defines whether the system is executing or not; used for
 *  preventing any blocking using the condition variable;
 * A second set of parameters, orchestrate the I/O mechanisms (enforcement objects) to employ over
 * I/O workflows:
 *  - m_objects_lock: Mutex to ensure concurrency control over the enforcement objects container;
 *  - m_enf-objects: Container that holds all enforcement objects to be applied over requests; it is
 *  made of a std::pair, where the key corresponds to the differentiation token (to select which
 *  enforcement object to apply) and the value a unique pointer to an enforcement object.
 *  - m_no_match_object: By default, any I/O request whose classifiers do not match with any
 *  differentiation token in m_enf_objects, will be applied over a dedicated Noop enforcement
 * object, so no I/O mechanism is applied over the request;
 *  - m_diff_builder: Differentiation builder that allows to classify and differentiate requests,
 *  based on their classifiers, and forward them to the correct enforcement object to be applied.
 *  TODO:
 *   - use 2PL (m_objects_lock) in enforce_mechanism, create_enforcement_object,
 *   configure_enforcement_object, collect_enforcement_object_statistics, and objects_to_string;
 *   - change structs to specific/dedicated objects in collect_object_statistics,
 *   collect_general_statistics, collect_detailed_statistics, and collect_statistic_entry methods;
 *   - create a dedicated object for specifying each configuration or set of configurations in
 *   create_enforcement_object and configure_enforcement_object methods;
 */
class SubmissionQueue {

    friend class ChannelDefault;

private:
    std::queue<Ticket*> m_queue {};
    std::mutex m_queue_lock;
    std::condition_variable m_is_empty;
    uint64_t m_timeout_dequeue { 500000 };
    CompletionQueue* m_completion_queue { nullptr };
    std::atomic<bool> m_is_running { true };

    // enforcement objects
    std::mutex m_objects_lock;
    std::vector<std::pair<uint32_t, std::unique_ptr<EnforcementObject>>> m_enf_objects {};
    std::unique_ptr<EnforcementObject> m_no_match_object {
        std::make_unique<NoopObject> (-1, "no_match", true)
    };
    std::unique_ptr<ObjectDifferentiationBuilder> m_diff_builder {
        std::make_unique<ObjectHashingDifferentiation> (option_default_hashing_algorithm)
    };

    /**
     * size: Return the size of the submission queue. Must be used by a thread-safe method, namely
     * enqueue and dequeue.
     * @return Return m_queue's size.
     */
    [[nodiscard]] int size () const;

    /**
     * dequeue: Dequeue a Ticket from the SubmissionQueue (m_queue) and apply the respective
     * enforcement mechanism. Specifically, first the method verifies if m_queue has elements, and
     * waits in case if it is empty. Otherwise, it dequeue the first element, creates a Result
     * object and applies the respective enforcement mechanism (through enforce_mechanism). Finally,
     * it enqueues the Result object (after the enforcement) in the completion queue.
     * Thread-safe.
     * @return Returns true if the regular behavior was achieved (described above), or false if the
     * condition variable m_is_empty achieves a timeout or the system terminated the execution.
     */
    bool dequeue ();

    /**
     * enforce_mechanism: Employ the enforcement mechanism over the I/O request.
     * First, it builds the differentiation token to select the correct EnforcementObject, gets the
     * enforcement object pointer, and enforces its mechanisms through obj_enforce.
     * Thread-safe.
     * @param ticket Ticket object containing all request's information.
     * @param result Reference to a Result object that will store the enforcement result.
     */
    void enforce_mechanism (const Ticket& ticket, Result& result);

    /**
     * enqueue_in_completion_queue: Enqueue Result object in CompletionQueue (m_completion_queue).
     * Not thread-safe.
     * @param result Result object to be enqueued.
     */
    void enqueue_in_completion_queue (Result& result);

    /**
     * select_enforcement_object: Select enforcement object from m_enf_objects container.
     * Not thread-safe.
     * @param token Differentiation token depicting the enforcement object to select.
     * @return Returns a pointer to the enforcement object. If the token does not exists, returns a
     * nullptr.
     */
    [[nodiscard]] EnforcementObject* select_enforcement_object (const diff_token_t& token) const;

public:
    /**
     * SubmissionQueue default constructor.
     */
    SubmissionQueue ();

    /**
     * SubmissionQueue parameterized constructor.
     * @param completion_queue Pointer to a completion_queue.
     */
    explicit SubmissionQueue (CompletionQueue* completion_queue);

    /**
     * SubmissionQueue default destructor.
     */
    ~SubmissionQueue ();

    /**
     * enqueue: Enqueue a Ticket in the submission queue (m_queue). If the queue was previously
     * empty, it notifies all waiting threads that a new element is ready to be consumed.
     * Thread-safe.
     * @param ticket Pointer to the Ticket to be enqueued.
     */
    void enqueue (Ticket* ticket);

    /**
     * enqueue_fast_path: Rather than enqueueing the Ticket in m_queue, directly enforce the
     * respective I/O mechanism.
     * @param ticket Ticket object containing all necessary classifiers for the enforcement.
     * @param result Reference to the Result object that will store the enforcement result.
     */
    void enqueue_fast_path (const Ticket& ticket, Result& result);

    /**
     * get_size: Return the size of the submission queue.
     * Thread-safe.
     * @return Size of m_queue.
     */
    [[nodiscard]] int get_size ();

    /**
     * operator: Operator method that is used by a background thread that continuously dequeues
     * Ticket objects from m_queue. The thread stops dequeuing when m_is_running is set to false.
     */
    void operator() ();

    /**
     * stop_worker: Set m_is_running to false.
     */
    void stop_worker ();

    /**
     * create_enforcement_object: Create new enforcement object.
     * It already receives the differentiation token of the object.
     * Thread-safe.
     * @param token Differentiation token.
     * @param object Unique pointer to the new enforcement object.
     */
    PStatus create_enforcement_object (diff_token_t token,
        std::unique_ptr<EnforcementObject> object);

    /**
     * configure_enforcement_object: Configure enforcement object.
     * Thread-safe.
     * @param token Differentiation token to perform the enforcement object selection.
     * @param config Configuration identifier i.e., identifies which tuning knob to perform.
     * @param configurations List of configurations to be set.
     * @return Returns PStatus::OK if configurations were made successfully, and PStatus::Error
     * otherwise.
     */
    PStatus configure_enforcement_object (diff_token_t token,
        const int& config,
        const std::vector<long>& configurations);

    /**
     * collect_enforcement_object_statistics: Collect statistics from enforcement object.
     * @param token Differentiation token to select the object to collect.
     * @param statistics_raw Reference to a ObjectStatisticsRaw object that stores the statistics.
     * @return Returns PStatus::OK if statistics were effectively collect, and PStatus::error
     * otherwise.
     */
    PStatus collect_enforcement_object_statistics (const diff_token_t& token,
        ObjectStatisticsRaw& statistics_raw);

    /**
     * define_object_differentiation: Defines how requests should be differentiated i.e., which
     * request classifiers should be considered for the I/O differentiation.
     * This method first sets the classifiers of the EnforcementObjectDifferentiationBuilder and
     * then binds the differentiation function to be used.
     * Further, this method should used either at the initialization of the SubmissionQueue or upon
     * the explicit submission of differentiation rules from the SDS control plane.
     * @param operation_type Boolean that defines if the operation type classifier should be
     * considered for differentiating requests.
     * @param operation_context Boolean that defines if the operation context classifier should be
     * considered for differentiating requests.
     */
    void define_object_differentiation (const bool& operation_type, const bool& operation_context);

    /**
     * build_object_token: Build differentiation token to select which enforcement object to use.
     * @param operation_type Operation type classifier.
     * @param operation_context Operation context classifier.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_object_token (const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value) const;

    /**
     * objects_to_string: get a string of all enforcement objects in this channel.
     * @return String object.
     */
    std::string objects_to_string ();
};

}; // namespace paio::enforcement

#endif // PAIO_SUBMISSION_QUEUE_HPP
