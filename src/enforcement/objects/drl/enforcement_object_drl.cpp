/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/objects/drl/enforcement_object_drl.hpp>

namespace paio::enforcement {

// DynamicRateLimiter default constructor.
DynamicRateLimiter::DynamicRateLimiter ()
{
    Logging::log_debug ("DynamicRateLimiter default constructor.");
}

// DynamicRateLimiter parameterized constructor.
DynamicRateLimiter::DynamicRateLimiter (const long& object_id, const bool& collect_statistics) :
    m_object_id { object_id },
    m_collect_statistics { collect_statistics },
    m_bucket { collect_statistics, drl_option_gc_sliding_window }
{
    Logging::log_debug ("DynamicRateLimiter parameterized constructor.");

    // launch thread to refill token-bucket
    this->spawn_refill_thread (drl_option_token_bucket_type);
}

// DynamicRateLimiter default destructor.
DynamicRateLimiter::~DynamicRateLimiter ()
{
    Logging::log_debug_explicit ("DynamicRateLimiter destructor.");
    this->join_refill_thread ();
}

// spawn_refill_thread call. Defines how the token-bucket's refill process is made, spawning a new
// thread in case of TokenBucketType::threaded, and none otherwise.
void DynamicRateLimiter::spawn_refill_thread (const TokenBucketType& bucket_type)
{
    switch (bucket_type) {
        case TokenBucketType::normal:
            Logging::log_debug ("TokenBucketType::normal threaded mode is disabled.");
            break;

        case TokenBucketType::threaded:
            Logging::log_debug ("TokenBucketType::threaded refill operation thread "
                                "running in background.");
            this->m_refill_thread = std::thread (std::ref (this->m_bucket));
            break;
    }
}

// join_refill_thread call. Join the refill thread, if TokenBucketType::threaded.
void DynamicRateLimiter::join_refill_thread ()
{
    // verify if thread is joinable (thread is running and has completed its work)
    if (this->m_refill_thread.joinable ()) {
        Logging::log_debug_explicit ("DynamicRateLimiter:: joining (background) refill thread.");
        this->m_refill_thread.join ();
    }
}

// get_enforcement_object_id call. Get the EnforcementObject's identifier.
long DynamicRateLimiter::get_enforcement_object_id () const
{
    return this->m_object_id;
}

// obj_enforce call. Enforce the dynamic rate limiting service over I/O workflows.
void DynamicRateLimiter::obj_enforce (const Ticket& ticket, Result& result)
{
    { // critical section
        std::unique_lock<std::mutex> lock (this->m_mutex);
        // consume tokens from the token-bucket
        this->m_bucket.try_consume (this->basic_io_cost (ticket));
    }

    // update Result's status and has_content
    bool has_content = (ticket.get_buffer_size () > 0);
    result.set_result_status (ResultStatus::success);
    result.set_has_content (has_content);

    // update Result's content and content size
    if (has_content) {
        result.set_content_size (ticket.get_buffer_size ());
        result.set_content (ticket.get_buffer_size (), ticket.get_buffer ());
    }
}

// obj_configure call. Configure the tuning knobs of the DynamicRateLimiter enforcement object.
PStatus DynamicRateLimiter::obj_configure (int conf, const std::vector<long>& conf_values)
{
    PStatus status;

    switch (static_cast<DRLConfiguration> (conf)) {
        case DRLConfiguration::init:
            status = this->initialize (conf_values[0], conf_values[1], false);
            break;

        case DRLConfiguration::rate:
            this->configure_rate (conf_values[0]);
            status = PStatus::OK ();
            break;

        case DRLConfiguration::refill:
            this->configure_refill_window (conf_values[0]);
            status = PStatus::OK ();
            break;

        default:
            status = PStatus::NotSupported ();
            break;
    }

    // update m_token_bucket_rate value
    { // critical section
        std::unique_lock<std::mutex> lock (this->m_mutex);
        this->m_token_bucket_rate
            = (this->m_bucket.get_capacity () / (double)this->m_bucket.get_refill_period ());
    }

    return status;
}

// obj_collect_statistics call. Collect statistics from the DynamicRateLimiter/token-bucket object.
PStatus DynamicRateLimiter::obj_collect_statistics (ObjectStatisticsRaw& statistics_raw)
{
    PStatus status { PStatus::Error () };
    std::unique_lock<std::mutex> lock (this->m_mutex);

    // verify if statistic collection is enabled
    if (this->m_collect_statistics) {
        // try to collect statistics
        this->m_bucket.try_collect_statistics (statistics_raw);
        status = PStatus::OK ();
    }

    return status;
}

// initialize call. Initialize values of the token-bucket.
PStatus DynamicRateLimiter::initialize (const long& refill_period,
    const long& rate,
    [[maybe_unused]] const bool& collect_statistics)
{
    { // critical section
        std::unique_lock<std::mutex> lock (m_mutex);
        // set token-bucket's m_refill_period with the new value
        this->m_bucket.set_refill_period (refill_period);

        // the token-bucket's m_tokens begins with the same value as m_capacity
        this->m_bucket.set_token_count (static_cast<token> (rate));

        // set token-bucket's capacity (given a refill_period and max_throughput)
        this->m_bucket.set_capacity (static_cast<token> (rate));
    }

    // log debug message
    std::string message { "DynamicRateLimiter::Initialize (" };
    message.append (std::to_string (this->m_bucket.get_capacity ())).append (", ");
    message.append (std::to_string (this->m_bucket.get_token_count ())).append (", ");
    message.append (std::to_string (this->m_bucket.get_refill_period ())).append (")");
    Logging::log_debug (message);

    return PStatus::OK ();
}

// configure_rate call. Define new maximum throughput value.
void DynamicRateLimiter::configure_rate (const long& rate_value)
{
    std::unique_lock<std::mutex> lock (m_mutex);

    // normalize rate_value
    double tokens = this->m_bucket.normalize_tokens (static_cast<token> (rate_value));

    // set bucket's capacity
    this->m_bucket.set_capacity (static_cast<token> (rate_value));

    // verify the number of tokens in the bucket
    if (this->m_bucket.get_token_count () > tokens) {
        this->m_bucket.set_token_count (static_cast<token> (rate_value));
    }
}

// configure_refill_window call. Define new refill_period value for the token-bucket.
void DynamicRateLimiter::configure_refill_window (const long& window)
{
    std::unique_lock<std::mutex> lock (m_mutex);

    // get current token-bucket rate
    long rate = this->get_token_bucket_rate ();

    // update refill_period
    this->m_bucket.set_refill_period (window);
    // update the number of tokens
    this->m_bucket.set_token_count (static_cast<token> (rate));
    // update capacity size to keep the same max_throughput
    this->m_bucket.set_capacity (static_cast<token> (rate));
}

// get_token_bucket_rate call. Return the current throughput of the DynamicRateLimiter object.
long DynamicRateLimiter::get_token_bucket_rate () const
{
    return static_cast<long> (this->m_bucket.get_capacity ()
        / (static_cast<double> (this->m_bucket.get_refill_period ()) / 1000 / 1000));
}

// estimate_io_cost call. Estimate the cost of I/O requests.
token DynamicRateLimiter::estimate_io_cost (const Ticket& ticket)
{
    // calculate expected cost (estimated) of request N
    auto estimated_request_cost
        = static_cast<token> (this->m_cost_per_request * ticket.get_payload ());

    // calculate debt of request N-1
    // auto debt
    //      = (m_token_bucket_rate * ticket.get_previous_exec_time ()) - m_previous_estimated_cost;
    token debt = 1;

    // apply convergence factor
    if (debt > 0) {
        debt *= this->m_convergence_factor;
    }

    // calculate cost of request N
    token cost = estimated_request_cost - debt;

    // update estimated cost of request (N-1)
    this->m_previous_estimated_cost = static_cast<uint32_t> (estimated_request_cost);

    return cost;
}

// basic_io_cost call. Estimate the cost of I/O requests.
token DynamicRateLimiter::basic_io_cost (const Ticket& ticket) const
{
    // double cost;
    // switch (static_cast<LSM_KVS_DETAILED> (ticket.get_operation_context())) {
    //     case LSM_KVS_DETAILED::bg_compaction_L1_L2:
    //         cost = 1.15;
    //         break;
    //     case LSM_KVS_DETAILED::bg_compaction_L2_L3:
    //         cost = 1.3;
    //         break;
    //     case LSM_KVS_DETAILED::bg_compaction_LN:
    //         cost = 1.5;
    //         break;
    //     default:
    //         cost = 1.0;
    //         break;
    // }
    return static_cast<token> (this->m_cost_per_request * ticket.get_payload ());
}

// to_string call. Return the DynamicRateLimiter variables in string format.
std::string DynamicRateLimiter::to_string ()
{
    std::string message { "DynamicRateLimiter object {" };
    message.append (std::to_string (this->m_object_id)).append (", ");
    message.append (this->m_bucket.to_string ()).append ("}");

    return message;
}

} // namespace paio::enforcement
