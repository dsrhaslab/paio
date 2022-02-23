/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/utils/rules_parser.hpp>
#include <stdexcept>

namespace paio::utils {

// RulesParser default constructor.
RulesParser::RulesParser ()
{
    Logging::log_debug ("RulesParser default constructor.");
}

// RulesParser parameterized constructor.
RulesParser::RulesParser (RuleType type, const fs::path& path) :
    m_rules_type { type },
    m_staged_rules {}
{
    Logging::log_debug ("RulesParser parameterized constructor.");
    this->read_rules_from_file (path);
}

// RulesParser default destructor.
RulesParser::~RulesParser ()
{
    Logging::log_debug ("RulesParser default destructor.");
}

/**
 * hash: Compute an hash based on a string value. The methods uses an offset (hardcoded to 0), and
 * a salt/tweak (hardcoded to 10242048).
 * @param s String-based value to be computed.
 * @param off Offset value.
 * @param salt Slat to generate pseudo-random hash values.
 * @return Returns an unsigned int value that corresponds to the original string.
 */
constexpr unsigned int hash (const char* s, int off = 0, int salt = 10242048)
{
    return !s[off] ? salt : (hash (s, off + 1) * 33) ^ s[off];
}

/**
 * operator ""_: String operator that converts a string to the corresponding hash value.
 * @param string_value String to be computed.
 * @return Returns an unsigned int value that corresponds to the original string.
 */
constexpr inline unsigned int operator""_ (char const* string_value, size_t)
{
    return hash (string_value);
}

// getRulesType call. Get the type of the rules in the file.
RuleType RulesParser::get_rule_type () const
{
    return this->m_rules_type;
}

// parse_rule call. Parses a line and stores its contents in a container.
void RulesParser::parse_rule (const std::string& rule, std::vector<std::string>& tokens)
{
    size_t start;
    size_t end = 0;

    while ((start = rule.find_first_not_of (' ', end)) != std::string::npos) {
        end = rule.find (' ', start);
        tokens.push_back (rule.substr (start, end - start));
    }
}

// read_rules_from_file call. Read the rules from file @param path.
int RulesParser::read_rules_from_file (const fs::path& path)
{
    std::string line;
    std::ifstream input_stream;

    int total_rules = 0;
    // open file stream
    input_stream.open (path.string ());

    // verify if stream is open
    if (input_stream.is_open ()) {
        // read line and store in line_t
        while (std::getline (input_stream, line)) {
            // send to parser and store in the respective structure
            std::vector<std::string> tokens {};
            this->parse_rule (line, tokens);

            // store parsed tokens
            this->m_staged_rules.push_back (tokens);
            total_rules++;
        }

        // close file stream
        input_stream.close ();
    } else {
        Logging::log_error ("RulesParser: cannot open file " + path.string () + ".");
    }

    return total_rules;
}

// convertHousekeepingOperation call. Convert a string to a HousekeepingOperation.
HousekeepingOperation RulesParser::convert_housekeeping_operation (
    const std::string& operation) const
{
    switch (paio::utils::hash (operation.data ())) {
        case "create_channel"_:
            return HousekeepingOperation::create_channel;
        case "create_object"_:
            return HousekeepingOperation::create_object;
        default:
            return HousekeepingOperation::no_op;
    }
}

// convert_object_type call. Convert a string to the respective EnforcementObjectType.
EnforcementObjectType RulesParser::convert_object_type (const std::string& object_type) const
{
    return (object_type == "drl") ? EnforcementObjectType::DRL : EnforcementObjectType::NOOP;
}

// convertEnforcementOperation call. Convert a string to an enforcement operation.
int RulesParser::convert_enforcement_operation (const EnforcementObjectType& object_type,
    const std::string& operation) const
{
    switch (object_type) {
        case EnforcementObjectType::DRL:
            switch (paio::utils::hash (operation.data ())) {
                case "init"_:
                    return 1;
                case "rate"_:
                    return 2;
                case "refill"_:
                    return 4;
                default:
                    return 0;
            }

        case EnforcementObjectType::NOOP:
            return 0;

        default:
            throw std::logic_error { "RulesParser: unexpected object type" };
    }
}

// convert_context_type_definition call. Convert string-based ContextType value to long.
long RulesParser::convert_context_type_definition (const std::string& context_type) const
{
    switch (paio::utils::hash (context_type.data ())) {
        case "general"_:
            return static_cast<long> (ContextType::PAIO_GENERAL);
        case "posix"_:
            return static_cast<long> (ContextType::POSIX);
        case "posix_meta"_:
            return static_cast<long> (ContextType::POSIX_META);
        case "lsm_kvs_simple"_:
            return static_cast<long> (ContextType::LSM_KVS_SIMPLE);
        case "lsm_kvs_detailed"_:
            return static_cast<long> (ContextType::LSM_KVS_DETAILED);
        case "kvs"_:
            return static_cast<long> (ContextType::KVS);
        default:
            return -1;
    }
}

// convert_differentiation_definitions call. Convert I/O differentiation and classification
// definitions from string to long.
long RulesParser::convert_differentiation_definitions (const std::string& context_type,
    const std::string& definition) const
{
    switch (paio::utils::hash (context_type.data ())) {
        case "general"_:
            return this->convert_paio_general_definitions (definition);
        case "posix"_:
            return this->convert_posix_definitions (definition);
        case "posix_meta"_:
            return this->convert_posix_meta_definitions (definition);
        case "lsm_kvs_simple"_:
            return this->convert_posix_lsm_simple_definitions (definition);
        case "lsm_kvs_detailed"_:
            return this->convert_posix_lsm_detailed_definitions (definition);
        case "kvs"_:
            return this->convert_kvs_definitions (definition);
        default:
            return -1;
    }
}

// convert_paio_general_definitions call. Convert PAIO_GENERAL differentiation definitions from
// string to long.
long RulesParser::convert_paio_general_definitions (const std::string& general_definitions) const
{
    switch (paio::utils::hash (general_definitions.data ())) {
        case "foreground"_:
            return static_cast<long> (PAIO_GENERAL::foreground);
        case "background"_:
            return static_cast<long> (PAIO_GENERAL::background);
        case "high_priority"_:
            return static_cast<long> (PAIO_GENERAL::high_priority);
        case "low_priority"_:
            return static_cast<long> (PAIO_GENERAL::low_priority);
        default:
            return static_cast<long> (PAIO_GENERAL::no_op);
    }
}

// convert_posix_lsm_simple_definitions call. Convert POSIX_KVS_LSM_SIMPLE differentiation
// definitions from string to long.
long RulesParser::convert_posix_lsm_simple_definitions (
    const std::string& posix_lsm_definitions) const
{
    switch (paio::utils::hash (posix_lsm_definitions.data ())) {
        case "bg_flush"_:
            return static_cast<long> (LSM_KVS_SIMPLE::bg_flush);
        case "bg_compaction_high_priority"_:
            return static_cast<long> (LSM_KVS_SIMPLE::bg_compaction_high_priority);
        case "bg_compaction_low_priority"_:
            return static_cast<long> (LSM_KVS_SIMPLE::bg_compaction_low_priority);
        case "foreground"_:
            return static_cast<long> (LSM_KVS_SIMPLE::foreground);
        case "background"_:
            return static_cast<long> (LSM_KVS_SIMPLE::background);
        default:
            return static_cast<long> (LSM_KVS_SIMPLE::no_op);
    }
}

// convert_posix_lsm_detailed_definitions call. Convert POSIX_KVS_LSM_DETAILED differentiation
// definitions from string to long.
long RulesParser::convert_posix_lsm_detailed_definitions (
    const std::string& posix_lsm_definitions) const
{
    switch (paio::utils::hash (posix_lsm_definitions.data ())) {
        case "bg_flush"_:
            return static_cast<long> (LSM_KVS_DETAILED::bg_flush);
        case "bg_compaction"_:
            return static_cast<long> (LSM_KVS_DETAILED::bg_compaction);
        case "bg_compaction_L0_L0"_:
            return static_cast<long> (LSM_KVS_DETAILED::bg_compaction_L0_L0);
        case "bg_compaction_L0_L1"_:
            return static_cast<long> (LSM_KVS_DETAILED::bg_compaction_L0_L1);
        case "bg_compaction_L1_L2"_:
            return static_cast<long> (LSM_KVS_DETAILED::bg_compaction_L1_L2);
        case "bg_compaction_L2_L3"_:
            return static_cast<long> (LSM_KVS_DETAILED::bg_compaction_L2_L3);
        case "bg_compaction_LN"_:
            return static_cast<long> (LSM_KVS_DETAILED::bg_compaction_LN);
        case "foreground"_:
            return static_cast<long> (LSM_KVS_DETAILED::foreground);
        default:
            return static_cast<long> (LSM_KVS_DETAILED::no_op);
    }
}

// convert_posix_definitions call. Convert POSIX differentiation definitions from string to long.
long RulesParser::convert_posix_definitions (const std::string& posix_definitions) const
{
    switch (paio::utils::hash (posix_definitions.data ())) {
        case "read"_:
            return static_cast<long> (POSIX::read);
        case "write"_:
            return static_cast<long> (POSIX::write);
        case "pread"_:
            return static_cast<long> (POSIX::pread);
        case "pwrite"_:
            return static_cast<long> (POSIX::pwrite);
        case "pread64"_:
            return static_cast<long> (POSIX::pread64);
        case "pwrite64"_:
            return static_cast<long> (POSIX::pwrite64);
        case "fread"_:
            return static_cast<long> (POSIX::fread);
        case "fwrite"_:
            return static_cast<long> (POSIX::fwrite);
        case "open"_:
            return static_cast<long> (POSIX::open);
        case "open64"_:
            return static_cast<long> (POSIX::open64);
        case "creat"_:
            return static_cast<long> (POSIX::creat);
        case "creat64"_:
            return static_cast<long> (POSIX::creat64);
        case "openat"_:
            return static_cast<long> (POSIX::openat);
        case "close"_:
            return static_cast<long> (POSIX::close);
        case "fsync"_:
            return static_cast<long> (POSIX::fsync);
        case "fdatasync"_:
            return static_cast<long> (POSIX::fdatasync);
        case "sync"_:
            return static_cast<long> (POSIX::sync);
        case "syncfs"_:
            return static_cast<long> (POSIX::syncfs);
        case "truncate"_:
            return static_cast<long> (POSIX::truncate);
        case "ftruncate"_:
            return static_cast<long> (POSIX::ftruncate);
        case "truncate64"_:
            return static_cast<long> (POSIX::truncate64);
        case "ftruncate64"_:
            return static_cast<long> (POSIX::ftruncate64);
        case "xstat"_:
            return static_cast<long> (POSIX::xstat);
        case "lxstat"_:
            return static_cast<long> (POSIX::lxstat);
        case "fxstat"_:
            return static_cast<long> (POSIX::fxstat);
        case "xstat64"_:
            return static_cast<long> (POSIX::xstat64);
        case "lxstat64"_:
            return static_cast<long> (POSIX::lxstat64);
        case "fxstat64"_:
            return static_cast<long> (POSIX::fxstat64);
        case "fxstatat"_:
            return static_cast<long> (POSIX::fxstatat);
        case "fxstatat64"_:
            return static_cast<long> (POSIX::fxstatat64);
        case "statfs"_:
            return static_cast<long> (POSIX::statfs);
        case "fstatfs"_:
            return static_cast<long> (POSIX::fstatfs);
        case "statfs64"_:
            return static_cast<long> (POSIX::statfs64);
        case "fstatfs64"_:
            return static_cast<long> (POSIX::fstatfs64);
        case "link"_:
            return static_cast<long> (POSIX::link);
        case "linkat"_:
            return static_cast<long> (POSIX::linkat);
        case "unlink"_:
            return static_cast<long> (POSIX::unlink);
        case "unlinkat"_:
            return static_cast<long> (POSIX::unlinkat);
        case "rename"_:
            return static_cast<long> (POSIX::rename);
        case "renameat"_:
            return static_cast<long> (POSIX::renameat);
        case "symlink"_:
            return static_cast<long> (POSIX::symlink);
        case "symlinkat"_:
            return static_cast<long> (POSIX::symlinkat);
        case "readlink"_:
            return static_cast<long> (POSIX::readlink);
        case "readlinkat"_:
            return static_cast<long> (POSIX::readlinkat);
        case "fopen"_:
            return static_cast<long> (POSIX::fopen);
        case "fopen64"_:
            return static_cast<long> (POSIX::fopen64);
        case "freopen"_:
            return static_cast<long> (POSIX::freopen);
        case "freopen64"_:
            return static_cast<long> (POSIX::freopen64);
        case "fclose"_:
            return static_cast<long> (POSIX::fclose);
        case "fflush"_:
            return static_cast<long> (POSIX::fflush);
        case "access"_:
            return static_cast<long> (POSIX::access);
        case "faccessat"_:
            return static_cast<long> (POSIX::faccessat);
        case "lseek"_:
            return static_cast<long> (POSIX::lseek);
        case "lseek64"_:
            return static_cast<long> (POSIX::lseek64);
        case "fseek"_:
            return static_cast<long> (POSIX::fseek);
        case "fseek64"_:
            return static_cast<long> (POSIX::fseek64);
        case "ftell"_:
            return static_cast<long> (POSIX::ftell);
        case "fseeko"_:
            return static_cast<long> (POSIX::fseeko);
        case "fseeko64"_:
            return static_cast<long> (POSIX::fseeko64);
        case "ftello"_:
            return static_cast<long> (POSIX::ftello);
        case "ftello64"_:
            return static_cast<long> (POSIX::ftello64);
        case "mkdir"_:
            return static_cast<long> (POSIX::mkdir);
        case "mkdirat"_:
            return static_cast<long> (POSIX::mkdirat);
        case "rmdir"_:
            return static_cast<long> (POSIX::rmdir);
        case "opendir"_:
            return static_cast<long> (POSIX::opendir);
        case "readdir"_:
            return static_cast<long> (POSIX::readdir);
        case "readdir64"_:
            return static_cast<long> (POSIX::readdir64);
        case "fdopendir"_:
            return static_cast<long> (POSIX::fdopendir);
        case "closedir"_:
            return static_cast<long> (POSIX::closedir);
        case "dirfd"_:
            return static_cast<long> (POSIX::dirfd);
        case "getxattr"_:
            return static_cast<long> (POSIX::getxattr);
        case "lgetxattr"_:
            return static_cast<long> (POSIX::lgetxattr);
        case "fgetxattr"_:
            return static_cast<long> (POSIX::fgetxattr);
        case "setxattr"_:
            return static_cast<long> (POSIX::setxattr);
        case "lsetxattr"_:
            return static_cast<long> (POSIX::lsetxattr);
        case "fsetxattr"_:
            return static_cast<long> (POSIX::fsetxattr);
        case "removexattr"_:
            return static_cast<long> (POSIX::removexattr);
        case "lremovexattr"_:
            return static_cast<long> (POSIX::lremovexattr);
        case "fremovexattr"_:
            return static_cast<long> (POSIX::fremovexattr);
        case "listxattr"_:
            return static_cast<long> (POSIX::listxattr);
        case "llistxattr"_:
            return static_cast<long> (POSIX::llistxattr);
        case "flistxattr"_:
            return static_cast<long> (POSIX::flistxattr);
        case "chmod"_:
            return static_cast<long> (POSIX::chmod);
        case "fchmod"_:
            return static_cast<long> (POSIX::fchmod);
        case "fchmodat"_:
            return static_cast<long> (POSIX::fchmodat);
        case "chown"_:
            return static_cast<long> (POSIX::chown);
        case "fchown"_:
            return static_cast<long> (POSIX::fchown);
        case "fchownat"_:
            return static_cast<long> (POSIX::fchownat);
        case "lchown"_:
            return static_cast<long> (POSIX::lchown);
        default:
            return static_cast<long> (POSIX::no_op);
    }
}

// convert_posix_meta_definitions call. Convert POSIX_META differentiation definitions from string
// to long.
long RulesParser::convert_posix_meta_definitions (const std::string& posix_meta_definitions) const
{
    switch (paio::utils::hash (posix_meta_definitions.data ())) {
        case "foreground"_:
            return static_cast<long> (POSIX_META::foreground);
        case "background"_:
            return static_cast<long> (POSIX_META::background);
        case "high_priority"_:
            return static_cast<long> (POSIX_META::high_priority);
        case "med_priority"_:
            return static_cast<long> (POSIX_META::med_priority);
        case "low_priority"_:
            return static_cast<long> (POSIX_META::low_priority);
        case "data_op"_:
            return static_cast<long> (POSIX_META::data_op);
        case "meta_op"_:
            return static_cast<long> (POSIX_META::meta_op);
        case "dir_op"_:
            return static_cast<long> (POSIX_META::dir_op);
        case "ext_attr_op"_:
            return static_cast<long> (POSIX_META::ext_attr_op);
        case "file_mod_op"_:
            return static_cast<long> (POSIX_META::file_mod_op);
        default:
            return static_cast<long> (POSIX::no_op);
    }
}

// convert_kvs_definitions call. Convert KVS differentiation definitions from string to long.
long RulesParser::convert_kvs_definitions (const std::string& kvs_definitions) const
{
    switch (paio::utils::hash (kvs_definitions.data ())) {
        case "put"_:
            return static_cast<long> (KVS::put);
        case "get"_:
            return static_cast<long> (KVS::get);
        case "new_iterator"_:
            return static_cast<long> (KVS::new_iterator);
        case "delete"_:
            return static_cast<long> (KVS::delete_);
        case "write"_:
            return static_cast<long> (KVS::write);
        case "get_snapshot"_:
            return static_cast<long> (KVS::get_snapshot);
        case "get_property"_:
            return static_cast<long> (KVS::get_property);
        case "get_approximate_size"_:
            return static_cast<long> (KVS::get_approximate_size);
        case "compact_range"_:
            return static_cast<long> (KVS::compact_range);
        default:
            return static_cast<long> (KVS::no_op);
    }
}

// fixme: update the differentiation and object properties vector (way too hardcoded)
// get_create_channel_rules call. Convert string-based rules of create_channel type to the
// respective HousekeepingRule.
int RulesParser::get_create_channel_rules (std::vector<HousekeepingRule>& hsk_rules,
    int total_rules)
{
    int rules_passed = 0;
    if (total_rules == -1) {
        total_rules = std::numeric_limits<int>::max ();
    }

    for (auto& staged_rule : this->m_staged_rules) {
        // convert staged rule element to HousekeepingOperation type
        HousekeepingOperation operation = this->convert_housekeeping_operation (staged_rule[1]);

        // validate if current operation is of type 'create_channel'
        if (operation == HousekeepingOperation::create_channel) {
            // verify if total of elements in staged rule are complete
            if (static_cast<int> (staged_rule.size ())
                < this->m_create_channel_rules_min_elements) {
                Logging::log_error ("Error while reading staged rule and creating "
                                    "HousekeepingRule object (missing elements)");
            } else {
                // emplace the enum ContextType of which the operation definitions respect to, and
                // the respective classifiers, namely workflow-id (staged_rule[4]), operation type
                // (staged_rule[5]), and operation context (staged_rule[6])
                std::vector<long> channel_differentiation_properties {};
                // fixme: this is way too hardcoded
                // push_back ContextType class
                channel_differentiation_properties.push_back (
                    this->convert_context_type_definition (staged_rule[3]));
                // push_back workflow-id definition
                channel_differentiation_properties.push_back (std::stol (staged_rule[4]));
                // push_back operation type definition
                channel_differentiation_properties.push_back (
                    this->convert_differentiation_definitions (staged_rule[3], staged_rule[5]));
                // push_back operation context definition
                channel_differentiation_properties.push_back (
                    this->convert_differentiation_definitions (staged_rule[3], staged_rule[6]));

                // create HousekeepingRule for creating new channel
                HousekeepingRule channel_rule { std::stoull (staged_rule[0]),
                    HousekeepingOperation::create_channel,
                    std::stol (staged_rule[2]),
                    -1,
                    channel_differentiation_properties };

                hsk_rules.push_back (channel_rule);
                rules_passed++;

                if (rules_passed == total_rules) {
                    break;
                }
            }
        }
    }

    return rules_passed;
}

// fixme: update the differentiation and object properties vector (way too hardcoded)
// get_create_object_rules call. Convert string-based rules of create_object type to the respective
// HousekeepingRule.
int RulesParser::get_create_object_rules (std::vector<HousekeepingRule>& hsk_rules, int total_rules)
{
    int rules_passed = 0;
    if (total_rules == -1) {
        total_rules = std::numeric_limits<int>::max ();
    }

    for (auto& staged_rule : this->m_staged_rules) {
        // convert staged rule element to HousekeepingOperation type
        HousekeepingOperation operation = this->convert_housekeeping_operation (staged_rule[1]);

        // validate if current operation is of type 'create_object'
        if (operation == HousekeepingOperation::create_object) {
            // verify if total of elements in staged rule are complete
            if (static_cast<int> (staged_rule.size ()) < this->m_create_object_rules_min_elements) {
                Logging::log_error ("Error while reading staged rule and creating "
                                    "HousekeepingRule object (missing elements)");
            } else {
                // differentiation and enforcement object properties
                std::vector<long> dif_and_obj_properties {};

                // fixme: this is way too hardcoded
                // insert differentiation properties of the EnforcementObject
                // push_back ContextType class
                dif_and_obj_properties.push_back (
                    this->convert_context_type_definition (staged_rule[4]));
                // push_back operation type definition
                dif_and_obj_properties.push_back (
                    this->convert_differentiation_definitions (staged_rule[4], staged_rule[5]));
                // push_back operation context definition
                dif_and_obj_properties.push_back (
                    this->convert_differentiation_definitions (staged_rule[4], staged_rule[6]));

                // insert initialization properties of the EnforcementObject
                // push_back EnforcementObjectType
                dif_and_obj_properties.push_back (
                    static_cast<long> (this->convert_object_type (staged_rule[7])));
                // push_back first (initialization) property
                dif_and_obj_properties.push_back (std::stol (staged_rule[8]));
                // push_back second (initialization) property
                dif_and_obj_properties.push_back (std::stol (staged_rule[9]));

                // create HousekeepingRule for creating new object
                HousekeepingRule object_rule { std::stoull (staged_rule[0]),
                    HousekeepingOperation::create_object,
                    std::stol (staged_rule[2]),
                    std::stol (staged_rule[3]),
                    dif_and_obj_properties };

                hsk_rules.push_back (object_rule);
                rules_passed++;

                if (rules_passed == total_rules) {
                    break;
                }
            }
        }
    }

    return rules_passed;
}

// get_enforcement_rules call. Convert stage rules of type ENF to the respective RAW structure.
int RulesParser::get_enforcement_rules (std::vector<EnforcementRule>& enf_rules, int total_rules)
{
    int rules_passed = 0;

    if (total_rules == -1) {
        total_rules = std::numeric_limits<int>::max ();
    }

    for (auto& staged_rule : this->m_staged_rules) {
        // convert string to enforcement operation type
        int operation_type
            = this->convert_enforcement_operation (this->convert_object_type (staged_rule[3]),
                staged_rule[4]);

        // create configurations vector
        std::vector<long> configurations {};
        // fixme: this is way too hardcoded
        switch (operation_type) {
            case 1: // drl - init
                configurations.push_back (std::stol (staged_rule[5])); // initial refill period
                configurations.push_back (std::stol (staged_rule[6])); // initial rate
                break;

            case 2: // drl - rate/refill
                configurations.push_back (std::stol (staged_rule[5])); // new rate/refill period
                break;

            default:
                // do nothing; leave empty vector
                break;
        }

        // create enforcement rule
        EnforcementRule rule { std::stoull (staged_rule[0]),
            std::stol (staged_rule[1]),
            std::stol (staged_rule[2]),
            operation_type,
            configurations };

        enf_rules.push_back (rule);
        rules_passed++;

        if (rules_passed == total_rules) {
            break;
        }
    }

    return rules_passed;
}

// erase_rules call. Remove all rules from the m_staged_rules container.
int RulesParser::erase_rules ()
{
    int removed_rules = 0;
    for (auto iterator = this->m_staged_rules.begin (); iterator != this->m_staged_rules.end ();) {
        this->m_staged_rules.erase (iterator);
        removed_rules++;
    }

    // return total of rules removed
    return removed_rules;
}

// to_string call. Write to stdout all staged rules in the m_staged_rules container.
void RulesParser::print_rules (FILE* fd) const
{
    for (const auto& m_staged_rule : this->m_staged_rules) {
        for (const auto& parameter : m_staged_rule) {
            std::fprintf (fd, "%s ", parameter.c_str ());
        }
        std::fprintf (fd, "\n");
    }
}

} // namespace paio::utils
