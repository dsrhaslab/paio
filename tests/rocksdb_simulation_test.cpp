/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <cstdio>
#include <paio/interface/posix_layer.hpp>
#include <paio/stage/paio_stage.hpp>

/**
 * Notes - options header:
 *  - option_create_default_channels = false;
 *  - option_default_channel_differentiation_workflow = true;
 *  - option_default_channel_differentiation_operation_type = false;
 *  - option_default_channel_differentiation_operation_context = false;
 *  - option_default_enforcement_object_differentiation_operation_type = true;
 *  - option_default_enforcement_object_differentiation_operation_context = true;
 *  - option_default_channel_statistic_collection = true;
 *  - option_default_statistic_metric = StatisticMetric::throughput;
 *  - option_default_statistic_classifier = ClassifierType::operation_context;
 *  - option_default_context_type = ContextType::LSM_KVS_SIMPLE.
 *  TODO:
 *   - complete remainder code to simulate the RocksDB use case in PAIO;
 *   - spawn 8 threads for foreground work
 *   - spawn 1 thread for flushing
 *   - spawn 7 threads for compactions, where each of these can submit different compaction types
 *   (use a random generator, provide more probability for high priority compactions)
 */
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

    int channels = -1;
    bool default_object_creation = false;
    std::string stage_name_value = "RocksDB";

    // create PAIO stage
    std::shared_ptr<paio::PaioStage> stage_ptr { std::make_shared<paio::PaioStage> (channels,
        default_object_creation,
        stage_name_value,
        std::string { "../files/tests/rocksdb_housekeeping_rules" },
        std::string {},
        std::string {},
        true) };

    // print stage information
    std::fprintf (fd, "%s\n", stage_ptr->stage_info_to_string ().c_str ());

    return 0;
}
