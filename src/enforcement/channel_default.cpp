/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/channel_default.hpp>

namespace paio::enforcement {

// ChannelDefault constructor.
ChannelDefault::ChannelDefault ()
{
    // spawn worker threads if using completion queue mode
    if (!this->m_use_fast_path) {
        this->start_workers ();
    }

    // initialize statistic collection at channel
    if (this->m_collect_channel_statistics) {
        this->m_channel_statistics.initialize (option_default_context_type);
    }

    // debug message
    Logging::log_debug ("ChannelDefault constructor");
}

// ChannelDefault parameterized constructor.
ChannelDefault::ChannelDefault (const uint32_t& channel_id,
    const bool& use_fast_path,
    const bool& collect_channel_statistics,
    const bool& collect_object_statistics) :
    m_channel_id { channel_id },
    m_use_fast_path { use_fast_path },
    m_collect_channel_statistics { collect_channel_statistics },
    m_collect_object_statistics { collect_object_statistics }
{
    // spawn worker threads if using completion queue mode
    if (!use_fast_path) {
        this->start_workers ();
    }

    // initialize statistic collection at channel
    if (collect_channel_statistics) {
        this->m_channel_statistics.initialize (option_default_context_type);
    }

    // debug message
    Logging::log_debug ("ChannelDefault (" + std::to_string (channel_id) + ") constructor.");
}

// ChannelDefault destructor.
ChannelDefault::~ChannelDefault ()
{
    // join worker threads if using completion queue mode
    if (!this->m_use_fast_path) {
        this->join_workers ();
    }

    // terminate (collect and aggregate) statistic collection at channel
    if (this->m_collect_channel_statistics) {
        // terminate statistic collection
        std::string stats = this->m_channel_statistics.terminate ();

        // debug message reporting the statistic counters
        Logging::log_debug (
            "ChannelDefault destructor (" + std::to_string (this->m_channel_id) + ")::" + stats);
    }
}

// get_channel_id call. Get the channel identifier value.
uint32_t ChannelDefault::get_channel_id () const
{
    return this->m_channel_id;
}

// build_ticket call. Build new Ticket object.
Ticket ChannelDefault::build_ticket (const Context& request_context,
    const void* buffer,
    const std::size_t& size)
{
    // create Ticket with content buffer
    if (size > 0 && buffer != nullptr) {
        return { this->gen_ticket_id (),
            request_context.get_total_operations (),
            static_cast<long> (
                request_context.get_operation_size () * request_context.get_total_operations ()),
            request_context.get_operation_type (),
            request_context.get_operation_context (),
            size,
            static_cast<const unsigned char*> (buffer) };
    } else {
        // note: further validation and testing
        // verify if the operation size is greater than 0
        uint64_t operation_size = request_context.get_operation_size () == 0
            ? 1
            : request_context.get_operation_size ();

        // create Ticket without content buffer
        return { this->gen_ticket_id (),
            request_context.get_total_operations (),
            static_cast<long> (operation_size * request_context.get_total_operations ()),
            request_context.get_operation_type (),
            request_context.get_operation_context () };
    }
}

// channel_enforce call. Enforce request through channel.
void ChannelDefault::channel_enforce (const Context& context,
    const void* buffer,
    const std::size_t& buffer_size,
    Result& result)
{
    // build Ticket object
    Ticket ticket = this->build_ticket (context, buffer, buffer_size);
    // update Result's ticket identifier
    result.set_ticket_id (ticket.get_ticket_id ());

    // use fast path or enqueue/dequeue
    if (this->m_use_fast_path) {
        // enqueue request using fast path
        this->m_submission_queue.enqueue_fast_path (ticket, result);
    } else {
        // enqueue ticket and wait until it's enforced
        this->m_submission_queue.enqueue (&ticket);

        // dequeue result from completion queue; current_thread is waiting for result
        this->m_completion_queue.dequeue (ticket.get_ticket_id (), result);
    }

    // collect I/O flow statistics is enabled
    if (this->m_collect_channel_statistics) {
        uint64_t operation_size = context.get_operation_size ();
        if (option_default_statistic_metric == StatisticMetric::counter && operation_size == 0) {
            operation_size = 1;
        }

        // update statistic counters
        this->m_channel_statistics.update_statistic_entry (
            context.get_operation_type (), // operation type
            context.get_operation_context (), // operation context
            context.get_total_operations () * operation_size); // counter value
    }
}

// gen_ticket_id call. Atomically generate ticket identifier.
uint64_t ChannelDefault::gen_ticket_id ()
{
    return this->m_ticket_id++;
}

// create_enforcement_object call. Create new enforcement object in the channel.
PStatus ChannelDefault::create_enforcement_object (const long& enforcement_object_id,
    const ObjectDifferentiationPair& differentiation_pair,
    const EnforcementObjectType& object_type,
    const std::vector<long>& configurations)
{
    PStatus status;

    // generate differentiation token for enforcement object
    uint32_t hash_value;
    this->build_object_differentiation_token (differentiation_pair.get_operation_type (),
        differentiation_pair.get_operation_context (),
        hash_value);

    switch (object_type) {
        case EnforcementObjectType::drl:
            status = this->m_submission_queue.create_enforcement_object (hash_value,
                std::make_unique<DynamicRateLimiter> (enforcement_object_id,
                    this->m_collect_object_statistics));
            break;

        case EnforcementObjectType::noop:
            status = this->m_submission_queue.create_enforcement_object (hash_value,
                std::make_unique<NoopObject> (enforcement_object_id));
            break;

        default:
            status = PStatus::NotSupported ();
            break;
    }

    // apply initialization configuration to the enforcement object
    if (status.is_ok ()) {
        // create new entry in the object-id to diff-token mapper
        this->create_new_enforcement_object_linker (enforcement_object_id, hash_value);

        // apply existing configurations to the enforcement object
        if (!configurations.empty ()) {
            if (object_type == EnforcementObjectType::drl) {
                status = m_submission_queue.configure_enforcement_object (hash_value,
                    static_cast<int> (DRLConfiguration::init),
                    configurations);
            }
        }
    }

    return status;
}

// configure_enforcement_object call. Configure enforcement object.
PStatus ChannelDefault::configure_enforcement_object (const long& enforcement_object_id,
    const int& config,
    const std::vector<long>& configurations)
{
    // get enforcement object's differentiation token
    diff_token_t object_token = this->get_enforcement_object_diff_token (enforcement_object_id);

    // validate if object-token is valid
    if (object_token == static_cast<diff_token_t> (-1)) {
        Logging::log_error (
            "EnforcementObject-" + std::to_string (enforcement_object_id) + " does not exist.");
        return PStatus::Error ();
    }

    // configure enforcement object in the SubmissionQueue
    return this->m_submission_queue.configure_enforcement_object (object_token,
        config,
        configurations);
}

// collect_object_statistics call. Collect statistics from enforcement object.
PStatus ChannelDefault::collect_object_statistics (const long& enforcement_object_id,
    ObjectStatisticsRaw& statistics_raw)
{
    // validate if object statistic collection is enabled
    if (this->m_collect_object_statistics) {
        // get enforcement object's differentiation token
        diff_token_t object_token = this->get_enforcement_object_diff_token (enforcement_object_id);

        // validate if object-token is valid
        if (object_token == static_cast<diff_token_t> (-1)) {
            Logging::log_error (
                "EnforcementObject-" + std::to_string (enforcement_object_id) + " does not exist.");
            return PStatus::Error ();
        }

        // collect object statistics from SubmissionQueue
        return this->m_submission_queue.collect_enforcement_object_statistics (object_token,
            statistics_raw);
    } else {
        return PStatus::Error ();
    }
}

// collect_general_statistics call. Collect the global I/O statistics collected in this channel.
PStatus ChannelDefault::collect_general_statistics (ChannelStatsRaw& general_stats)
{
    // verify if statistics collection is enabled
    if (this->m_collect_channel_statistics) {
        // collect general statistics from channel
        this->m_channel_statistics.collect (general_stats);
        return PStatus::OK ();
    }

    return PStatus::Error ();
}

// collect_detailed_statistics call. Collect the all I/O statistics collected in this channel.
PStatus ChannelDefault::collect_detailed_statistics (std::vector<double>& detailed_stat_entries)
{
    // verify if statistics collection is enabled
    if (this->m_collect_channel_statistics) {
        // collect detailed statistic entries
        this->m_channel_statistics.collect_detailed_windowed_entries (detailed_stat_entries);
        return PStatus::OK ();
    }

    return PStatus::Error ();
}

// collect_statistic_entry call. Collect a single statistic entry.
PStatus ChannelDefault::collect_statistic_entry (ChannelStatsRaw& stats, const int& operation)
{
    // verify if statistics collection is enabled
    if (this->m_collect_channel_statistics) {
        // collect single statistic entry
        this->m_channel_statistics.collect_single_entry (stats, operation);
        return PStatus::OK ();
    }

    return PStatus::Error ();
}

// define_object_differentiation call. Define how request should be classified and differentiated.
void ChannelDefault::define_object_differentiation (const bool& operation_type,
    const bool& operation_context)
{
    // set differentiation parameters
    this->m_submission_queue.define_object_differentiation (operation_type, operation_context);
}

// build_object_differentiation_token call. Build differentiation token to select which enforcement
// object to use.
void ChannelDefault::build_object_differentiation_token (const uint32_t& operation_type,
    const uint32_t& operation_context,
    uint32_t& hash_value)
{
    // build (enforcement object) differentiation token
    this->m_submission_queue.build_object_token (operation_type, operation_context, hash_value);
}

// to_string call. Wrap ChannelDefault information in string format.
std::string ChannelDefault::to_string ()
{
    return "ChannelDefault:: " + this->m_submission_queue.objects_to_string ();
}

// start_workers call. Spawn worker threads to handle requests in parallel.
void ChannelDefault::start_workers ()
{
    // start worker threads to continuously handle requests from the submission queue
    for (int i = 0; i < this->m_parallelism_level; i++) {
        this->m_worker_pool.emplace_back (std::ref (this->m_submission_queue));
    }
}

// join_workers call. Join workers from the m_worker_pool.
void ChannelDefault::join_workers ()
{
    // set execution of background threads to false
    this->m_submission_queue.stop_worker ();

    // stop all workers
    for (int i = 0; i < this->m_parallelism_level; i++) {
        this->m_worker_pool[i].join ();
    }
}

// create_new_enforcement_object_linker call. Maps an enforcement object id to the respective
// differentiation token.
void ChannelDefault::create_new_enforcement_object_linker (const long& object_id,
    const diff_token_t& object_token)
{
    std::lock_guard<std::mutex> guard (this->object_token_linker_lock);
    this->m_object_id_to_token_linkers.emplace_back (std::make_pair (object_id, object_token));
}

// get_enforcement_object_diff_token call. Get the differentiation token of the corresponding
// enforcement object id.
diff_token_t ChannelDefault::get_enforcement_object_diff_token (const long& object_id)
{
    std::lock_guard<std::mutex> guard (this->object_token_linker_lock);
    // initialize an iterator to traverse the vector
    // since the function is marked as const, it will use a const_iterator
    auto iterator = this->m_object_id_to_token_linkers.begin ();
    for (; iterator != this->m_object_id_to_token_linkers.end (); iterator++) {
        // if found, return enforcement object's differentiation token
        if (iterator->first == object_id) {
            return iterator->second;
        }
    }

    // if not found, return -1
    return static_cast<diff_token_t> (-1);
}

}; // namespace paio::enforcement
