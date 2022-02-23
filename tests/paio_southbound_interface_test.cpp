/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2021 INESC TEC.
 **/

#include <paio/stage/paio_stage.hpp>

namespace paio::networking {

class SouthboundInterfaceTest {

private:
    FILE* m_fd { stdout };

public:
    /**
     * SouthboundInterfaceTest default constructor.
     */
    SouthboundInterfaceTest () = default;

    /**
     * SouthboundInterfaceTest parameterized constructor.
     * @param fd
     */
    explicit SouthboundInterfaceTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * SouthboundInterfaceTest default destructor.
     */
    ~SouthboundInterfaceTest () = default;

    /**
     * stage_handshake_test
     * @param stage
     */
    void stage_handshake_test (ConnectionManager* manager)
    {
        std::fprintf (this->m_fd, "----------------------------------------------\n");
        std::fprintf (this->m_fd, "StageHandshake test\n");
        std::fprintf (this->m_fd, "----------------------------------------------\n");

        // TODO: perform a single test combining the handshake and southbound interfaces
        // auto return_value = manager->m_southbound_handler.stage_handshake ();
    }

    /**
     * create_housekeeping_rule_channel
     * @param stage
     */
    void create_housekeeping_rule_channel (Core* core, ConnectionManager* manager)
    {
        std::fprintf (this->m_fd, "----------------------------------------------\n");
        std::fprintf (this->m_fd, "Create HousekeepingRule (channel) test\n");
        std::fprintf (this->m_fd, "----------------------------------------------\n");

        // create control operation w/ respective subtype (create channel)
        ControlOperation operation {};
        operation.m_operation_subtype
            = static_cast<int> (ControlPlaneOperationSubtype::hsk_create_channel);
        std::vector<long> channels_list {};

        std::fprintf (this->m_fd, "... before southbound-interface call ...\n");

        // list registered housekeeping rules in the stage
        std::fprintf (this->m_fd, "%s\n", core->list_housekeeping_table_rules ().c_str ());

        // list installed channels in the stage
        std::fprintf (this->m_fd, "Total channels: %d\n", core->get_total_channels ());
        core->get_channels_identifiers (channels_list);
        for (auto elem : channels_list) {
            std::fprintf (this->m_fd, "   Channel %ld\n", elem);
        }

        // create housekeeping operation; to test this, we created a simples
        // HousekeepingCreateChannelRaw struct to prevent reading from socket
        manager->m_southbound_connection_handler->create_housekeeping_rule (operation);

        std::fprintf (this->m_fd, "... after southbound-interface call ...\n");

        // list registered housekeeping rules in the stage
        std::fprintf (this->m_fd, "%s\n", core->list_housekeeping_table_rules ().c_str ());

        std::fprintf (this->m_fd, "Total channels: %d\n", core->get_total_channels ());
        channels_list.clear ();
        // get identifiers of all channels installed at the stage
        core->get_channels_identifiers (channels_list);
        for (auto elem : channels_list) {
            std::fprintf (this->m_fd, "   Channel %ld\n", elem);
        }
    }

    /**
     * create_housekeeping_rule_object
     * @param stage
     */
    void create_housekeeping_rule_object (Core* core, ConnectionManager* manager)
    {
        std::fprintf (this->m_fd, "-------------------------------------------------\n");
        std::fprintf (this->m_fd, "Create HousekeepingRule (enforcement-object) test\n");
        std::fprintf (this->m_fd, "-------------------------------------------------\n");

        // create control operation w/ respective subtype (create object)
        ControlOperation operation {};
        operation.m_operation_subtype
            = static_cast<int> (ControlPlaneOperationSubtype::hsk_create_object);
        std::vector<long> channels_list {};

        std::fprintf (this->m_fd, "... before southbound-interface call ...\n");

        // list registered housekeeping rules in the stage
        std::fprintf (this->m_fd, "%s\n", core->list_housekeeping_table_rules ().c_str ());

        // create housekeeping operation; to test this, we created a simple
        // HousekeepingCreateChannelRaw struct to prevent reading from socket
        manager->m_southbound_connection_handler->create_housekeeping_rule (operation);

        std::fprintf (this->m_fd, "... after southbound-interface call ...\n");
        // list registered housekeeping rules in the stage
        std::fprintf (this->m_fd, "%s\n", core->list_housekeeping_table_rules ().c_str ());
    }

    /**
     * create_enforcement_rule:
     * @param stage
     */
    void create_enforcement_rule (ConnectionManager* manager)
    {
        std::fprintf (this->m_fd, "----------------------------------------------\n");
        std::fprintf (this->m_fd, "Create EnforcementRule test\n");
        std::fprintf (this->m_fd, "----------------------------------------------\n");

        // create empty ControlOperation object
        ControlOperation operation {};

        // create enforcement rule operation; to test this, we created a simple EnforcementRuleRaw
        // struct to prevent reading from socket; this call is dependent the previous testing calls,
        // namely create_housekeeping_rule_channel and create_housekeeping_rule_object
        manager->m_southbound_connection_handler->create_enforcement_rule (operation);
    }

    /**
     * mark_data_plane_stage_ready:
     * @param stage
     */
    void mark_data_plane_stage_ready (PaioStage* stage, ConnectionManager* manager)
    {
        std::fprintf (this->m_fd, "----------------------------------------------\n");
        std::fprintf (this->m_fd, "Mark data plane stage ready test\n");
        std::fprintf (this->m_fd, "----------------------------------------------\n");

        // create empty ControlOperation object
        ControlOperation operation {};

        std::fprintf (this->m_fd,
            "Before call: Is data plane stage ready? %s\n",
            (stage->is_ready () ? "true" : "false"));

        // mark data plane stage ready operation; to test this, we created a simple StageReadyRaw
        // struct to prevent reading from the socket; the struct defines if the stage should be
        // marked or not.
        manager->m_southbound_connection_handler->mark_stage_as_ready (operation);

        std::fprintf (this->m_fd,
            "After call: Is data plane stage ready? %s\n",
            (stage->is_ready () ? "true" : "false"));
    }
};
} // namespace paio::networking

using namespace paio;

int main (int argc, char** argv)
{
    // check argv for the file to be placed the result
    FILE* fd = stdout;

    // open file to write the logging results
    if (argc > 1) {
        fd = ::fopen (argv[1], "w");

        if (fd == nullptr) {
            fd = stdout;
        }
    }

    int channels = 1;
    bool default_object_creation = true;
    std::string stage_name = "tensorflow-test-stage";

    std::shared_ptr<PaioStage> stage {
        std::make_shared<PaioStage> (channels, default_object_creation, stage_name)
    };

    std::cout << stage->stage_info_to_string () << "\n";

    SouthboundInterfaceTest test { fd };

    // perform stage handshake
    test.stage_handshake_test (stage->get_connection_manager ());

    // create HousekeepingRule of type 'create-channel'
    test.create_housekeeping_rule_channel (stage->get_core (), stage->get_connection_manager ());
    // create HousekeepingRule of type 'create-object'
    test.create_housekeeping_rule_object (stage->get_core (), stage->get_connection_manager ());

    // create EnforcementRule
    test.create_enforcement_rule (stage->get_connection_manager ());

    // mark data plane stage as ready
    test.mark_data_plane_stage_ready (stage.get (), stage->get_connection_manager ());

    return 0;
}
