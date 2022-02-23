/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/core/core.hpp>

namespace paio::core {

// Core default constructor.
Core::Core ()
{
    // define default Channel-level I/O differentiation
    this->define_channel_differentiation (option_default_channel_differentiation_workflow,
        option_default_channel_differentiation_operation_type,
        option_default_channel_differentiation_operation_context);

    Logging::log_debug ("Core default constructor.");
}

// Core parameterized constructor.
Core::Core (const int& channels,
    const bool& create_default_channels,
    const bool& create_default_objects)
{
    // define default Channel-level I/O differentiation
    this->define_channel_differentiation (option_default_channel_differentiation_workflow,
        option_default_channel_differentiation_operation_type,
        option_default_channel_differentiation_operation_context);

    // create default channel and enforcement objects (w/o HousekeepingRules input)
    if (create_default_channels) {
        // create a default set of Channels
        for (int i = 1; i <= channels; i++) {
            // create Channel with the workflow-id, operation_type, and operation_context I/O
            // classifiers set to $i and PAIO_GENERIC_CHANNEL::no_op
            ChannelDifferentiationTuple diff_tuple { static_cast<uint32_t> (i),
                static_cast<uint32_t> (PAIO_GENERAL::no_op),
                static_cast<uint32_t> (PAIO_GENERAL::no_op) };

            // create Channel
            PStatus status = this->create_channel (i, diff_tuple);

            // create default EnforcementObjects
            if (create_default_objects) {
                if (status.is_ok ()) {
                    // create default object-differentiation-pair w/ operation_type and
                    // operation_context I/O classifiers set to PAIO_GENERAL::no_op
                    ObjectDifferentiationPair diff_pair { static_cast<uint32_t> (
                                                              PAIO_GENERAL::no_op),
                        static_cast<uint32_t> (PAIO_GENERAL::no_op) };

                    // create default enforcement-object of NOOP type
                    status = this->create_enforcement_object (i, // channel-id
                        0, // object-id
                        diff_pair,
                        paio::options::EnforcementObjectType::NOOP, // object type
                        {}); // configurations

                    if (!status.is_ok ()) {
                        Logging::log_error ("Core: error while creating EnforcementObject.");
                    }
                } else {
                    Logging::log_error (
                        "Core: error while creating channel; EnforcementObject not created.");
                }
            }
        }
    }

    Logging::log_debug ("Core parameterized constructor.");
}

// Core default destructor.
Core::~Core ()
{
    Logging::log_debug_explicit ("Destroy PAIO Core ...");
};

// define_channel_differentiation call. Define how Channel selection is made.
void Core::define_channel_differentiation (const bool& workflow,
    const bool& operation_type,
    const bool& operation_context)
{
    // set I/O classifiers for Channel differentiation
    this->m_channel_diff_builder->set_classifiers (workflow, operation_type, operation_context);
    // bind function builder with new classifiers
    this->m_channel_diff_builder->bind_builder ();

    // log message
    std::string message { "Channel differentiation (" };
    message.append (workflow ? "true" : "false").append (", ");
    message.append (operation_type ? "true" : "false").append (", ");
    message.append (operation_context ? "true" : "false").append (")\n");
    Logging::log_debug (message);
}

// define_enforcement_object_differentiation_with_channel_token call. Define how EnforcementObject
// selection is made.
PStatus Core::define_enforcement_object_differentiation_with_channel_token (
    const diff_token_t& channel_token,
    const bool& operation_type,
    const bool& operation_context)
{
    // select channel to enforce request
    ChannelDefault* channel_ptr = this->select_channel (channel_token);

    // enforce I/O mechanism over request
    if (channel_ptr != nullptr) {
        channel_ptr->define_object_differentiation (operation_type, operation_context);
        return PStatus::OK ();
    } else {
        Logging::log_error ("Error while defining EnforcementObject differentiation: "
                            "Channel pointer ("
            + std::to_string (channel_token) + " is null.");
        return PStatus::Error ();
    }
}

// define_enforcement_object_differentiation call. Define how EnforcementObject selection is made.
PStatus Core::define_enforcement_object_differentiation (const long& channel_id,
    const bool& operation_type,
    const bool& operation_context)
{
    // get Channel's differentiation token
    diff_token_t channel_token;
    {
        std::lock_guard<std::mutex> guard (this->m_linkers_lock);
        channel_token = this->get_channel_diff_token (channel_id);
    }

    // verify if channel_token is valid
    if (channel_token == static_cast<diff_token_t> (-1)) {
        return PStatus::Error ();
    }

    // invoke private method for defining enforcement object differentiation
    return this->define_enforcement_object_differentiation_with_channel_token (channel_token,
        operation_type,
        operation_context);
}

// create_channel call. Create new Channel in the data plane stage.
PStatus Core::create_channel (const long& channel_id,
    const ChannelDifferentiationTuple& channel_differentiation_tuple)
{
    PStatus status = PStatus::Error ();

    // generate differentiation token for Channel
    diff_token_t channel_token;
    this->m_channel_diff_builder->build_differentiation_token (channel_differentiation_tuple,
        channel_token);

    // verify if Channel exists
    if (!this->does_channel_token_exist (channel_token)
        && !this->does_channel_id_exist (channel_id)) {
        // create new channel and emplace it in Core's m_channel_default container
        auto return_pair = this->m_channels.emplace (channel_token,
            std::make_unique<ChannelDefault> (channel_id,
                (option_default_channel_mode == ChannelMode::fast_path),
                option_default_channel_statistic_collection,
                option_default_object_statistic_collection));

        // verify if channel creation was successful
        if (return_pair.second) {
            status = PStatus::OK ();

            // create new entry in the channel-id to diff-token mapper
            this->create_new_channel_linker (channel_id, channel_token);

            // define default EnforcementObject differentiation
            if (this->m_define_default_object_differentiation.load ()) {
                return_pair.first->second->define_object_differentiation (
                    option_default_enforcement_object_differentiation_operation_type,
                    option_default_enforcement_object_differentiation_operation_context);
            }

        } else {
            Logging::log_error (
                "Error while creating channel; channel-id to diff-token link not registered.");
        }

    } else {
        Logging::log_error ("Error while creating Channel; Channel <'" + std::to_string (channel_id)
            + "', '" + std::to_string (channel_token) + "'> already exists.");
    }

    // if channel already exists, return StatusCode::error
    return status;
}

// create_new_channel_linker call. Create a new pair of channel-id and channel-token.
void Core::create_new_channel_linker (const long& channel_id, const diff_token_t& channel_token)
{
    {
        std::lock_guard<std::mutex> guard (this->m_linkers_lock);
        this->m_channel_id_to_token_linkers.emplace_back (
            std::make_pair (channel_id, channel_token));
    }
    // logging message
    std::string message { "Core: create_new_channel_linker (" };
    message.append (std::to_string (channel_id)).append (", ");
    message.append (std::to_string (channel_token)).append (")\n");
    Logging::log_debug (message);
}

// get_channel_diff_token call. Get the differentiation token of a given Channel.
diff_token_t Core::get_channel_diff_token (const long& channel_id) const
{
    // initialize an iterator to traverse the vector
    // since the function is marked as const, it will use a const_iterator
    auto iterator = this->m_channel_id_to_token_linkers.begin ();
    for (; iterator != this->m_channel_id_to_token_linkers.end (); iterator++) {
        // if found, return channel's differentiation token
        if (iterator->first == channel_id) {
            return iterator->second;
        }
    }

    // if not found, return -1
    return static_cast<diff_token_t> (-1);
}

// create_enforcement_object call. Create new EnforcementObject in the data plane stage.
PStatus Core::create_enforcement_object (const long& channel_id,
    const long& enforcement_object_id,
    const ObjectDifferentiationPair& differentiation_pair,
    const EnforcementObjectType& object_type,
    const std::vector<long>& configurations)
{
    PStatus status = PStatus::Error ();

    // get channel's differentiation token
    diff_token_t channel_token;
    {
        std::lock_guard<std::mutex> guard (this->m_linkers_lock);
        channel_token = this->get_channel_diff_token (channel_id);
    }

    // verify if channel_token is valid
    if (channel_token == static_cast<diff_token_t> (-1)) {
        Logging::log_error ("Channel ('" + std::to_string (channel_id) + "') does not exist.");
        return PStatus::Error ();
    }

    // select the channel (get pointer to channel) to create the object
    ChannelDefault* ptr = this->select_channel (channel_token);

    // verify if channel exists
    if (ptr != nullptr) {
        // create enforcement object
        status = ptr->create_enforcement_object (enforcement_object_id,
            differentiation_pair,
            object_type,
            configurations);

        if (!status.is_ok ()) {
            Logging::log_error ("Error while creating enforcement object.");
        }
    } else {
        Logging::log_error ("Error while creating enforcement object request: "
                            "channel pointer is null.");
    }

    return status;
}

// select_channel call. Verify and select a given channel of the data plane stage.
ChannelDefault* Core::select_channel (const diff_token_t& channel_token) const
{
    ChannelDefault* channel_ptr = nullptr;

    // search for the channel with 'channel_token'
    auto iterator = this->m_channels.find (channel_token);

    // validate if pointer is valid and return a const pointer to the channel
    if (iterator != this->m_channels.end ()) {
        channel_ptr = iterator->second.get ();
    }

    return channel_ptr;
}

// does_channel_exist call. Verifies if a given channel exists in the m_channels container.
bool Core::does_channel_token_exist (const diff_token_t& channel_token) const
{
    // search for the channel with a given 'channel_token'
    auto iterator = this->m_channels.find (channel_token);

    // validate if channel already exists
    if (iterator != this->m_channels.end ()) {
        return true;
    }

    return false;
}

// does_channel_id_exist call. Validate if a given channel exists in the
// m_channel_id_to_token_linkers container.
bool Core::does_channel_id_exist (const long& channel_id)
{
    std::lock_guard<std::mutex> guard (this->m_linkers_lock);

    // find if Channel with identifier 'channel_id' exists in m_channel_id_to_token_linkers
    auto it = std::find_if (this->m_channel_id_to_token_linkers.begin (),
        this->m_channel_id_to_token_linkers.end (),
        [channel_id] (
            const std::pair<long, diff_token_t>& element) { return element.first == channel_id; });

    // return 'true' if Channel exists; 'false' otherwise
    return (it != this->m_channel_id_to_token_linkers.end ());
}

// enforce_request call. Enforce specific storage (I/O) mechanism over the I/O request.
void Core::enforce_request (const Context& request_context,
    const void* buffer,
    const size_t& buffer_size,
    Result& result)
{
    // generate channel's differentiation token (to select the channel that enforces the request)
    diff_token_t channel_token;
    this->m_channel_diff_builder->build_differentiation_token (request_context.get_workflow_id (),
        request_context.get_operation_type (),
        request_context.get_operation_context (),
        channel_token);

    // select channel to enforce request
    ChannelDefault* channel_ptr = this->select_channel (channel_token);

    // enforce I/O mechanism over request
    if (channel_ptr != nullptr) {
        channel_ptr->channel_enforce (request_context, buffer, buffer_size, result);
    } else {
        Logging::log_error ("Error while enforcing request: channel pointer is null.");
    }
}

// list_channels call. List channels in the data plane stage with their string representation.
void Core::list_channels (std::vector<std::string>& channels)
{
    std::lock_guard<std::mutex> guard (this->m_channels_lock);

    // traverse all channels currently installed in the data plane stage
    for (const auto& channel : this->m_channels) {
        std::string message;
        message.append (std::to_string (channel.first)).append ("; ");
        message.append (channel.second->to_string ()).append ("\n");

        // emplace channel's string-based representation in the container
        channels.emplace_back (message);
    }
}

// insert_housekeeping_rule call. Insert an HousekeepingRule in the HousekeepingTable.
PStatus Core::insert_housekeeping_rule (const uint64_t& rule_id,
    const HousekeepingOperation& operation,
    const long& channel_id,
    const long& enforcement_object_id,
    const std::vector<long>& properties)
{
    // insert HousekeepingRule in table (parameterized version)
    return this->m_housekeeping_table.insert_housekeeping_rule (rule_id,
        operation,
        channel_id,
        enforcement_object_id,
        properties);
}

// insert_housekeeping_rule call. Insert an HousekeepingRule in the HousekeepingTable.
PStatus Core::insert_housekeeping_rule (const HousekeepingRule& rule)
{
    // insert HousekeepingRule in table (copy constructor version)
    return this->m_housekeeping_table.insert_housekeeping_rule (rule);
}

// execute_housekeeping_rule call. Execute a specific HousekeepingRule.
PStatus Core::execute_housekeeping_rule (const uint64_t& rule_id)
{
    PStatus status;
    // create temporary HousekeepingRule object
    HousekeepingRule rule {};
    // get an HousekeepingRule copy (with rule_id) from the HousekeepingTable
    status = this->m_housekeeping_table.select_housekeeping_rule (rule_id, rule);

    // verify if the HousekeepingRule selection was successful
    if (status.is_ok ()) {
        // verify if the rule was already employed/enforced
        if (!rule.get_enforced ()) {
            // verify and employ the respective housekeeping operation
            switch (rule.get_housekeeping_operation_type ()) {
                // create Channel
                case HousekeepingOperation::create_channel: {
                    // create ChannelDifferentiationTuple with a given workflow-identifier,
                    // namely rule.get_property_at_index (1); an operation_type, namely
                    // rule.get_property_at_index (2); and an operation_context, namely
                    // rule.get_property_at_index (3)
                    ChannelDifferentiationTuple diff_tuple { static_cast<uint32_t> (
                                                                 rule.get_property_at_index (1)),
                        static_cast<uint32_t> (rule.get_property_at_index (2)),
                        static_cast<uint32_t> (rule.get_property_at_index (3)) };

                    {
                        std::unique_lock<std::mutex> lock (this->m_channels_lock);
                        // <channel_id, {workflow_id, operation_type, operation_context}>
                        status = this->create_channel (rule.get_channel_id (), diff_tuple);
                    }
                    break;
                }

                // create EnforcementObject
                case HousekeepingOperation::create_object: {
                    // get EnforcementObject properties from generic properties' vector
                    std::vector<long> enforcement_object_configurations {};
                    // copy EnforcementObject properties from the HousekeepingRule object to an
                    // empty container. This function will get the all properties from the
                    // ContextType (begin_index: 4) to the final configuration to be set in the
                    // EnforcementObject (end_index: rule.get_properties_size () - 1).
                    int res = rule.get_properties_at_range (4,
                        rule.get_properties_size () - 1,
                        enforcement_object_configurations);

                    if (res == -1) {
                        // log message
                        Logging::log_error ("Error while executing HousekeepingRule of "
                                            "create_object type (index out-of-bounds)");
                    }

                    // create EnforcementObject differentiation pair w/ a given operation_type,
                    // namely rule.get_property_at_index (1), and an operation_context, namely
                    // rule.get_property_at_index (2).
                    ObjectDifferentiationPair diff_pair { static_cast<uint32_t> (
                                                              rule.get_property_at_index (1)),
                        static_cast<uint32_t> (rule.get_property_at_index (2)) };

                    {
                        std::unique_lock<std::mutex> lock (this->m_channels_lock);
                        // <channel_id, object_id, {operation_type, operation_context}, object_type,
                        // initial-configurations>
                        status = this->create_enforcement_object (rule.get_channel_id (),
                            rule.get_enforcement_object_id (),
                            diff_pair,
                            static_cast<EnforcementObjectType> (rule.get_property_at_index (3)),
                            enforcement_object_configurations);
                    }
                    break;
                }

                case HousekeepingOperation::configure:
                case HousekeepingOperation::remove:
                default:
                    throw std::logic_error ("HousekeepingOperation type not supported");
            }
        } else {
            status = PStatus::Enforced ();
        }
    }

    // mark housekeeping rules as enforced
    if (status.is_ok ()) {
        this->m_housekeeping_table.mark_housekeeping_rule_as_enforced (rule_id);
    }

    return status;
}

// execute_housekeeping_rules call. Execute all staged (pending) housekeeping rules.
PStatus Core::execute_housekeeping_rules ()
{
    PStatus status = PStatus::Error ();
    // get HousekeepingTable iterator
    auto table_iterator = this->m_housekeeping_table.get_housekeeping_table_begin_iterator ();

    for (; table_iterator != this->m_housekeeping_table.get_housekeeping_table_end_iterator ();
         table_iterator++) {
        // verify if housekeeping rule was already enforced
        if (!table_iterator->second.get_enforced ()) {
            // employ housekeeping rule
            status = this->execute_housekeeping_rule (table_iterator->first);
            Logging::log_debug ("PStatus: " + status.to_string ());

            if (status.is_error ()) {
                return PStatus::Error ();
            }
        }
    }

    return status;
}

// list_housekeeping_table_rules call. Return a string with all HousekeepingRules in the table.
std::string Core::list_housekeeping_table_rules ()
{
    return this->m_housekeeping_table.to_string ();
}

// employ_enforcement_rule call. Employ an EnforcementRule over a given EnforcementObject.
PStatus Core::employ_enforcement_rule (const long& channel_id,
    const long& enforcement_object_id,
    const int& enforcement_rule_type,
    const std::vector<long>& configurations)
{
    PStatus status = PStatus::Error ();

    // get channel's differentiation token
    diff_token_t channel_token;
    {
        std::lock_guard<std::mutex> guard (this->m_linkers_lock);
        channel_token = this->get_channel_diff_token (channel_id);
    }

    // validate if channel-token is valid
    if (channel_token == static_cast<diff_token_t> (-1)) {
        Logging::log_error ("employ_enforcement_rule::Channel (" + std::to_string (channel_id)
            + ") does not exist");

        return status;
    }

    // select the channel to create the object
    ChannelDefault* channel_ptr = this->select_channel (channel_token);

    // verify if channel pointer is valid (i.e., if channel exists)
    if (channel_ptr != nullptr) {
        // configure enforcement object of Channel's channel_token
        status = channel_ptr->configure_enforcement_object (enforcement_object_id,
            enforcement_rule_type, // configuration type
            configurations); // list of new configurations
    }

    return status;
}

// collect_enforcement_object_statistics call. Collect statistics of a specific EnforcementObject.
PStatus Core::collect_enforcement_object_statistics (const long& channel_id,
    const long& enforcement_object_id,
    ObjectStatisticsRaw& object_stats_raw)
{
    PStatus status = PStatus::Error ();

    // get channel's differentiation token
    diff_token_t channel_token;
    {
        std::lock_guard<std::mutex> guard (this->m_linkers_lock);
        channel_token = this->get_channel_diff_token (channel_id);
    }

    // validate if channel-token is valid
    if (channel_token == static_cast<diff_token_t> (-1)) {
        Logging::log_error ("collect_enforcement_object_statistics::Channel ("
            + std::to_string (channel_id) + ") does not exist");

        return status;
    }

    // select the channel to create the object
    ChannelDefault* channel_ptr = this->select_channel (channel_token);

    // verify if channel pointer is valid (i.e., if channel exists)
    if (channel_ptr != nullptr) {
        // update ObjectStatisticsRaw's channel-id and enforcement-object-id
        object_stats_raw.m_channel_id = channel_id;
        object_stats_raw.m_enforcement_object_id = enforcement_object_id;

        // collect enforcement object statistics from Channel
        status = channel_ptr->collect_object_statistics (enforcement_object_id, object_stats_raw);
    } else {
        Logging::log_error ("Core::collect_enforcement_object_statistics: Channel ("
            + std::to_string (channel_token) + ") does not exist.");
    }

    return status;
}

// collect_channel_statistics call. Collect general statistics of a specific channel.
PStatus Core::collect_channel_statistics (const long& channel_id, ChannelStatsRaw& channel_stats)
{
    PStatus status = PStatus::Error ();

    // get channel's differentiation token
    diff_token_t channel_token;
    {
        std::lock_guard<std::mutex> guard (this->m_linkers_lock);
        channel_token = this->get_channel_diff_token (channel_id);
    }

    // validate if channel-token is valid
    if (channel_token == static_cast<diff_token_t> (-1)) {
        Logging::log_error ("collect_channel_statistics::Channel (" + std::to_string (channel_id)
            + ") does not exist");

        return status;
    }

    // select the channel to create the object
    ChannelDefault* channel_ptr = this->select_channel (channel_token);

    // verify if channel pointer is valid (i.e., if channel exists)
    if (channel_ptr != nullptr) {
        // update ChannelStatsRaw's channel-id
        channel_stats.m_channel_id = channel_id;
        // collect general statistics from Channel
        status = channel_ptr->collect_general_statistics (channel_stats);
    } else {
        Logging::log_error ("Core::collect_general_statistics: Channel ("
            + std::to_string (channel_token) + ") does not exist.");
    }

    return status;
}

// collect_channel_statistics_detailed call. Collect detailed statistics of a specific channel.
PStatus Core::collect_channel_statistics_detailed (const long& channel_id,
    std::vector<double>& detailed_stat_entries)
{
    PStatus status = PStatus::Error ();

    // get channel's differentiation token
    diff_token_t channel_token;
    {
        std::lock_guard<std::mutex> guard (this->m_linkers_lock);
        channel_token = this->get_channel_diff_token (channel_id);
    }

    // validate if channel-token is valid
    if (channel_token == static_cast<diff_token_t> (-1)) {
        Logging::log_error ("collect_channel_statistics_detailed::Channel ("
            + std::to_string (channel_id) + ") does not exist");

        return status;
    }

    // select the channel to create the object
    ChannelDefault* channel_ptr = this->select_channel (channel_token);

    // verify if channel pointer is valid (i.e., if channel exists)
    if (channel_ptr != nullptr) {
        // collect detailed statistics from Channel
        status = channel_ptr->collect_detailed_statistics (detailed_stat_entries);
    } else {
        Logging::log_error ("Core::collect_channel_statistics_detailed: Channel ("
            + std::to_string (channel_token) + ") does not exist.");
    }

    return status;
}

// get_total_channels call. Get total amount of channels operating in the data plane stage.
int Core::get_total_channels ()
{
    std::lock_guard<std::mutex> guard (this->m_linkers_lock);
    return static_cast<int> (this->m_channel_id_to_token_linkers.size ());
}

// get_channels_identifiers call. Copy Channels' identifiers from m_channel_id_tol_token_linkers to
// container passed as reference.
void Core::get_channels_identifiers (std::vector<long>& channels_ids)
{
    std::lock_guard<std::mutex> guard (this->m_linkers_lock);
    for (const auto& entry : this->m_channel_id_to_token_linkers) {
        channels_ids.push_back (entry.first);
    }
}

// set_default_object_differentiation call. Set new value for m_default_object_differentiation.
void Core::set_default_object_differentiation (const bool& value)
{
    this->m_define_default_object_differentiation.store (value);
}

} // namespace paio::core
