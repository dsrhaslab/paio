/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/submission_queue.hpp>

using namespace paio::enforcement;

namespace paio::enforcement {

// SubmissionQueue default constructor
SubmissionQueue::SubmissionQueue ()
{
    // define default I/O differentiation (enforcement object)
    this->define_object_differentiation (
        option_default_enforcement_object_differentiation_operation_type,
        option_default_enforcement_object_differentiation_operation_context);
}

// SubmissionQueue parameterized constructor
SubmissionQueue::SubmissionQueue (CompletionQueue* completion_queue) :
    m_completion_queue { completion_queue }
{
    // define default I/O differentiation  (enforcement object)
    this->define_object_differentiation (
        option_default_enforcement_object_differentiation_operation_type,
        option_default_enforcement_object_differentiation_operation_context);
}

// SubmissionQueue default destructor
SubmissionQueue::~SubmissionQueue () = default;

// size call. Return the size of the submission queue.
int SubmissionQueue::size () const
{
    return static_cast<int> (this->m_queue.size ());
}

// get_size call. Return the size of the submission queue. Thread-safe.
int SubmissionQueue::get_size ()
{
    std::lock_guard<std::mutex> guard (this->m_queue_lock);
    return static_cast<int> (this->m_queue.size ());
}

// enqueue call. Enqueue a Ticket in the submission queue.
void SubmissionQueue::enqueue (Ticket* ticket)
{
    std::unique_lock<std::mutex> lock (this->m_queue_lock);

    bool is_empty = (size () == 0);
    // emplace ticket pointer in the submission queue
    this->m_queue.emplace (ticket);

    // notify threads if SubmissionQueue.size() == 0 before emplace
    if (is_empty) {
        this->m_is_empty.notify_all ();
    }
}

// enqueue_fast_path call. Directly invoke enforcement_mechanism rather than enqueueing.
void SubmissionQueue::enqueue_fast_path (const Ticket& ticket, Result& result)
{
    this->enforce_mechanism (ticket, result);
}

// dequeue call. Dequeue a Ticket from the SubmissionQueue and apply the enforcement mechanism.
bool SubmissionQueue::dequeue ()
{
    std::unique_lock<std::mutex> lock (this->m_queue_lock);

    while (this->size () == 0) {
        // wait for the condition being satisfied or getting a timeout
        std::cv_status condition_status
            = this->m_is_empty.wait_for (lock, std::chrono::microseconds (this->m_timeout_dequeue));

        // validate if whether a timeout was triggered or the condition was satisfied, and if the
        // system is still running, to prevent staying blocked
        if (condition_status == std::cv_status::timeout && !(this->m_is_running.load ())) {
            return false;
        }
    }

    // dequeue from submission queue
    Ticket* ticket = this->m_queue.front ();
    this->m_queue.pop ();

    // create Result object and apply the enforcement mechanism
    Result result { ticket->get_ticket_id () };
    this->enforce_mechanism (*ticket, result);

    // enqueue 'result' in the completion queue
    this->enqueue_in_completion_queue (result);

    return true;
}

// build_object_token call. Build differentiation token to select the enforcement object.
void SubmissionQueue::build_object_token (const uint32_t& operation_type,
    const uint32_t& operation_context,
    uint32_t& hash_value) const
{
    this->m_diff_builder->build_differentiation_token (operation_type,
        operation_context,
        hash_value);
}

// enforce_mechanism call. Employ the enforcement mechanism over the I/O request.
void SubmissionQueue::enforce_mechanism (const Ticket& ticket, Result& result)
{
    // build differentiation token to select the correct EnforcementObject
    diff_token_t object_token;
    this->build_object_token (ticket.get_operation_type (),
        ticket.get_operation_context (),
        object_token);

    { // entering critical section
        std::unique_lock<std::mutex> lock (this->m_objects_lock);

        // select EnforcementObject and get pointer
        EnforcementObject* object_ptr = this->select_enforcement_object (object_token);

        // verify pointer and enforce I/O mechanism
        if (object_ptr != nullptr) {
            object_ptr->obj_enforce (ticket, result);
        } else {
            // if a match was not found for the request identifiers, bypass request
            this->m_no_match_object->obj_enforce (ticket, result);
        }
    }
}

// enqueue_in_completion call. Enqueue Result object in the CompletionQueue.
void SubmissionQueue::enqueue_in_completion_queue (Result& result)
{
    this->m_completion_queue->enqueue (result);
}

// Operator call. Operator call to be used by the background thread.
void SubmissionQueue::operator() ()
{
    std::stringstream stream;
    stream << "Operator::" << std::this_thread::get_id ();
    Logging::log_debug (stream.str ());

    // while the system is running, call dequeue (dequeue from SubmissionQueue, apply the
    // enforcement mechanism, and enqueue in the CompletionQueue)
    while (this->m_is_running.load ()) {
        // dequeue request
        auto return_value = this->dequeue ();

        // verify result from dequeue action
        if (!return_value) {
            Logging::log_debug ("Dequeue method was interrupted.");
        }
    }
}

// stop_worker call. Set m_is_running to false for stopping the execution of the background thread
// in the next iteration.
void SubmissionQueue::stop_worker ()
{
    this->m_is_running.store (false);
    Logging::log_debug ("SubmissionQueue stopped");
}

// select_enforcement_object call. Select enforcement object to enforce request.
EnforcementObject* SubmissionQueue::select_enforcement_object (const diff_token_t& token) const
{
    for (auto& m_object : this->m_enf_objects) {
        if (m_object.first == token) {
            return m_object.second.get ();
        }
    }
    return nullptr;
}

// create_enforcement_object call. Create a new enforcement object.
PStatus SubmissionQueue::create_enforcement_object (diff_token_t token,
    std::unique_ptr<EnforcementObject> object)
{
    std::unique_lock<std::mutex> lock (this->m_objects_lock);
    // get enforcement object to configure
    if (this->select_enforcement_object (token) != nullptr) {
        // build error message
        std::stringstream stream;
        stream << "EnforcementObject with token '";
        stream << token;
        stream << "' (id::'";
        stream << object->get_enforcement_object_id ();
        stream << "') already exists.";

        Logging::log_error (stream.str ());

        return PStatus::Error ();
    }

    // emplace enforcement object to container
    m_enf_objects.emplace_back (
        std::pair<uint32_t, std::unique_ptr<EnforcementObject>> (token, std::move (object)));

    Logging::log_debug (
        "Created enforcement object (size: " + std::to_string (this->m_enf_objects.size ()) + ").");

    return PStatus::OK ();
}

// configure_enforcement_object call. Configure enforcement object.
PStatus SubmissionQueue::configure_enforcement_object (diff_token_t token,
    const int& config,
    const std::vector<long>& configurations)
{
    std::unique_lock<std::mutex> lock (this->m_objects_lock);

    // get enforcement object to configure
    auto* object_ptr = this->select_enforcement_object (token);

    // configure object
    if (object_ptr == nullptr) {
        return PStatus::Error ();
    } else {
        return object_ptr->obj_configure (config, configurations);
    }
}

// collect_statistics call. Collect statistics from the enforcement object.
PStatus SubmissionQueue::collect_enforcement_object_statistics (const diff_token_t& token,
    ObjectStatisticsRaw& statistics_raw)
{
    std::unique_lock<std::mutex> lock (this->m_objects_lock);

    // get enforcement object to configure
    auto* object_ptr = this->select_enforcement_object (token);

    // collect statistics from object
    if (object_ptr == nullptr) {
        // fixme: this branch should return PStatus::Error; call explicitly to collect this ...
        return this->m_no_match_object->obj_collect_statistics (statistics_raw);
    } else {
        return object_ptr->obj_collect_statistics (statistics_raw);
    }
}

// define_object_differentiation call. Define how requests should be differentiated.
void SubmissionQueue::define_object_differentiation (const bool& operation_type,
    const bool& operation_context)
{
    this->m_diff_builder->set_classifiers (operation_type, operation_context);
    this->m_diff_builder->bind_builder ();
}

// objects_to_string call. Convert enforcement objects content to string format.
std::string SubmissionQueue::objects_to_string ()
{
    std::unique_lock<std::mutex> lock (this->m_objects_lock);
    std::stringstream stream;
    stream << "enforcement objects: ";

    for (auto& m_enf_object : this->m_enf_objects) {
        stream << "{ " << m_enf_object.first << ";";
        stream << m_enf_object.second->to_string () << " }\n";
    }

    return stream.str ();
}

} // namespace paio::enforcement