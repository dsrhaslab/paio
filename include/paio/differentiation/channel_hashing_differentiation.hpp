/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CHANNEL_HASHING_DIFFERENTIATION_HPP
#define PAIO_CHANNEL_HASHING_DIFFERENTIATION_HPP

#include <paio/core/context_propagation_definitions.hpp>
#include <paio/differentiation/channel_differentiation_builder.hpp>
#include <paio/differentiation/differentiation_token_builder.hpp>
#include <paio/differentiation/murmurhash_token_builder.hpp>
#include <paio/utils/logging.hpp>
#include <sstream>

using namespace paio::core;
using namespace paio::options;
using namespace paio::utils;

namespace paio::differentiation {

/**
 * ChannelHashingDifferentiation class.
 * This class is used for achieving I/O differentiation at the channel level.
 * More details on how this is achieved, check the ChannelDifferentiationBuilder class.
 * This class supports differentiation by hashing all (valid) classifiers into a single
 * differentiation token. It holds the following parameters:
 *  - m_use_workflow: boolean that defines if the workflow id classifier should be used (or not) to
 *  differentiate the I/O request;
 *  - m_use_operations_type: boolean that defines if the operation type classifier should be used
 *  (or not) to differentiate the I/O request;
 *  - m_use_operation_context: boolean that defines if the operation context classifier should be
 *  used (or not) to differentiate the I/O request;
 *  - m_token_builder: respects to the DifferentiationTokenBuilder object that generates the
 *  differentiation token of a given set of I/O classifiers;
 *  - m_lock: mutex used for concurrency control;
 *  - m_func_build_token: respects to the function that is used to compute the differentiation
 *  token of a given I/O request.
 * To classify and differentiate I/O requests, there are two phases that must be made. First, at
 * startup time, the user must set the classifiers that should be considered for the
 * differentiation process. Namely, if one wants to classify requests based on the workflow id,
 * operation type and operation context, one must set m_use_workflow, m_use_operation_type and
 * m_use_operation_context to true. If one wants to consider a single classifier (or none), it must
 * set the corresponding boolean value. This can be achieved using the set_classifiers method.
 * After setting the correct classifiers, one must bind the function pointer m_func_build_token with
 * the correct token builder. This is important since it will generate the differentiation token
 * based on the selected I/O classifiers. This is achieved using the bind_builder function, and
 * must be used whenever the I/O classifiers are changed.
 * In the second step, that happens at execution time, I/O requests should be classified and
 * differentiated. This is done using the build_differentiation_token, which receives all I/O
 * classifiers but only computes the differentiation token with those previously set.
 * TODO:
 *  - Currently, the DifferentiationTokenBuilder uses by default MurmurHash3, which hashes
 *  classifiers into a fixed-size token (32-bit hash); add support for other token builders.
 */
class ChannelHashingDifferentiation : public ChannelDifferentiationBuilder {

private:
    bool m_use_workflow { false };
    bool m_use_operation_type { false };
    bool m_use_operation_context { false };
    std::unique_ptr<DifferentiationTokenBuilder> m_token_builder {
        std::make_unique<MurmurHashTokenBuilder> (option_default_hashing_algorithm)
    };
    std::mutex m_lock;

    std::function<void (const uint32_t&, const uint32_t&, const uint32_t&, uint32_t&)>
        m_func_build_token;

    /**
     * build_workflow_type_context: Build differentiation token based on the workflow identifier,
     * operation type, and operation context.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_workflow_type_context (const uint32_t& workflow_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value)
    {
        std::string message_token = std::to_string (workflow_id);
        message_token.append ("|");
        message_token.append (std::to_string (operation_type));
        message_token.append ("|");
        message_token.append (std::to_string (operation_context));

        // generate differentiation token
        this->m_token_builder->generate_differentiation_token (message_token.c_str (),
            static_cast<int> (message_token.size ()),
            &hash_value);
    }

    /**
     * build_workflow_type: Build differentiation token based on the workflow identifier and
     * operation type.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_workflow_type (const uint32_t& workflow_id,
        const uint32_t& operation_type,
        [[maybe_unused]] const uint32_t& operation_context,
        uint32_t& hash_value)
    {
        std::string message_token = std::to_string (workflow_id);
        message_token.append ("|");
        message_token.append (std::to_string (operation_type));

        // generate differentiation token
        this->m_token_builder->generate_differentiation_token (message_token.c_str (),
            static_cast<int> (message_token.size ()),
            &hash_value);
    }

    /**
     * build_workflow_context: Build differentiation token based on the workflow identifier and
     * operation context.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_workflow_context (const uint32_t& workflow_id,
        [[maybe_unused]] const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value)
    {
        std::string message_token = std::to_string (workflow_id);
        message_token.append ("|");
        message_token.append (std::to_string (operation_context));

        // generate differentiation token
        this->m_token_builder->generate_differentiation_token (message_token.c_str (),
            static_cast<int> (message_token.size ()),
            &hash_value);
    }

    /**
     * build_workflow: Build differentiation token based on the workflow identifier.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_workflow (const uint32_t& workflow_id,
        [[maybe_unused]] const uint32_t& operation_type,
        [[maybe_unused]] const uint32_t& operation_context,
        uint32_t& hash_value)
    {
        std::string message_token = std::to_string (workflow_id);

        // generate differentiation token
        this->m_token_builder->generate_differentiation_token (message_token.c_str (),
            static_cast<int> (message_token.size ()),
            &hash_value);
    }

    /**
     * build_type_context: Build differentiation token based on the operation type and context.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_type_context ([[maybe_unused]] const uint32_t& workflow_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value)
    {
        std::string message_token = std::to_string (operation_type);
        message_token.append ("|");
        message_token.append (std::to_string (operation_context));

        // generate differentiation token
        this->m_token_builder->generate_differentiation_token (message_token.c_str (),
            static_cast<int> (message_token.size ()),
            &hash_value);
    }

    /**
     * build_type: Build differentiation token based on the operation type.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_type ([[maybe_unused]] const uint32_t& workflow_id,
        const uint32_t& operation_type,
        [[maybe_unused]] const uint32_t& operation_context,
        uint32_t& hash_value)
    {
        std::string message_token = std::to_string (operation_type);

        // generate differentiation token
        this->m_token_builder->generate_differentiation_token (message_token.c_str (),
            static_cast<int> (message_token.size ()),
            &hash_value);
    }

    /**
     * build_context: Build differentiation token based on the operation context.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_context ([[maybe_unused]] const uint32_t& workflow_id,
        [[maybe_unused]] const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value)
    {
        std::string message_token = std::to_string (operation_context);

        // generate differentiation token
        this->m_token_builder->generate_differentiation_token (message_token.c_str (),
            static_cast<int> (message_token.size ()),
            &hash_value);
    }

    /**
     * build_no_diff: Build differentiation token based no classifier.
     * @param workflow_id Workflow identifier classifier value.
     * @param operation_type Operation type classifier value.
     * @param operation_context Operation context classifier value.
     * @param hash_value Address to store the result of the hashing scheme.
     */
    void build_no_diff ([[maybe_unused]] const uint32_t& workflow_id,
        [[maybe_unused]] const uint32_t& operation_type,
        [[maybe_unused]] const uint32_t& operation_context,
        uint32_t& hash_value)
    {
        std::string message_token = "no_diff";

        // generate differentiation token
        this->m_token_builder->generate_differentiation_token (message_token.c_str (),
            static_cast<int> (message_token.size ()),
            &hash_value);
    }

    /**
     * generate_builder: Generate the differentiation builder function based on the I/O classifiers.
     */
    void generate_builder ()
    {
        std::stringstream stream;
        stream << "Generate (channel differentiation) builder (";
        stream << this->m_use_workflow << ",";
        stream << this->m_use_operation_type << ",";
        stream << this->m_use_operation_context << "): ";

        if (this->m_use_workflow && this->m_use_operation_type && this->m_use_operation_context) {
            this->m_func_build_token = [this] (const uint32_t& workflow_id,
                                           const uint32_t& operation_type,
                                           const uint32_t& operation_context,
                                           uint32_t& hash_value) {
                this->build_workflow_type_context (workflow_id,
                    operation_type,
                    operation_context,
                    hash_value);
            };

            stream << "&ChannelHashingDifferentiation::build_workflow_type_context";
        } else if (this->m_use_workflow && this->m_use_operation_type) {
            this->m_func_build_token = [this] (const uint32_t& workflow_id,
                                           const uint32_t& operation_type,
                                           const uint32_t& operation_context,
                                           uint32_t& hash_value) {
                this->build_workflow_type (workflow_id,
                    operation_type,
                    operation_context,
                    hash_value);
            };

            stream << "&ChannelHashingDifferentiation::build_workflow_type";
        } else if (this->m_use_workflow && this->m_use_operation_context) {
            this->m_func_build_token = [this] (const uint32_t& workflow_id,
                                           const uint32_t& operation_type,
                                           const uint32_t& operation_context,
                                           uint32_t& hash_value) {
                this->build_workflow_context (workflow_id,
                    operation_type,
                    operation_context,
                    hash_value);
            };

            stream << "&ChannelHashingDifferentiation::build_workflow_context";
        } else if (this->m_use_workflow) {
            this->m_func_build_token = [this] (const uint32_t& workflow_id,
                                           const uint32_t& operation_type,
                                           const uint32_t& operation_context,
                                           uint32_t& hash_value) {
                this->build_workflow (workflow_id, operation_type, operation_context, hash_value);
            };

            stream << "&ChannelHashingDifferentiation::build_workflow";
        } else if (this->m_use_operation_type && this->m_use_operation_context) {
            this->m_func_build_token = [this] (const uint32_t& workflow_id,
                                           const uint32_t& operation_type,
                                           const uint32_t& operation_context,
                                           uint32_t& hash_value) {
                this->build_type_context (workflow_id,
                    operation_type,
                    operation_context,
                    hash_value);
            };

            stream << "&ChannelHashingDifferentiation::build_type_context";
        } else if (this->m_use_operation_type) {
            this->m_func_build_token = [this] (const uint32_t& workflow_id,
                                           const uint32_t& operation_type,
                                           const uint32_t& operation_context,
                                           uint32_t& hash_value) {
                this->build_type (workflow_id, operation_type, operation_context, hash_value);
            };

            stream << "&ChannelHashingDifferentiation::build_type";
        } else if (this->m_use_operation_context) {
            this->m_func_build_token = [this] (const uint32_t& workflow_id,
                                           const uint32_t& operation_type,
                                           const uint32_t& operation_context,
                                           uint32_t& hash_value) {
                this->build_context (workflow_id, operation_type, operation_context, hash_value);
            };

            stream << "&ChannelHashingDifferentiation::build_context";
        } else {
            this->m_func_build_token = [this] (const uint32_t& workflow_id,
                                           const uint32_t& operation_type,
                                           const uint32_t& operation_context,
                                           uint32_t& hash_value) {
                this->build_no_diff (workflow_id, operation_type, operation_context, hash_value);
            };

            stream << "&ObjectHashingDifferentiation::build_no_diff";
        }

        // debug message
        Logging::log_debug (stream.str ());
    }

public:
    /**
     * ChannelHashingDifferentiation default constructor.
     * By default, it initializes the differentiation token builder to be used for the generation
     * of the differentiation tokens.
     */
    ChannelHashingDifferentiation () = default;

    /**
     * ChannelHashingDifferentiation default constructor.
     * By default, it initializes the differentiation token builder to be used for the generation
     * of the differentiation tokens.
     * @param scheme Hashing scheme to be used in the differentiation token generation.
     */
    explicit ChannelHashingDifferentiation (const HashingScheme& scheme) :
        m_token_builder { std::make_unique<MurmurHashTokenBuilder> (scheme) }
    { }

    /**
     * ChannelHashingDifferentiation parameterized constructor.
     * Initializes which classifiers to consider for performing the differentiation of requests.
     * It executes the generate_builder function that binds the differentiation function to use in
     * m_func_build_token, which is used to generate the differentiation tokens.
     * @param workflow_id Boolean that defines if the workflow identifier classifier should be
     * considered for differentiating requests.
     * @param operation_type Boolean that defines if the operation type classifier should be
     * considered for differentiating requests.
     * @param operation_context Boolean that defines if the operation context classifier should be
     * considered for differentiating requests.
     * @param scheme Hashing scheme to be used in the differentiation token generation.
     */
    ChannelHashingDifferentiation (bool workflow_id,
        bool operation_type,
        bool operation_context,
        const HashingScheme& scheme) :
        m_use_workflow { workflow_id },
        m_use_operation_type { operation_type },
        m_use_operation_context { operation_context },
        m_token_builder { std::make_unique<MurmurHashTokenBuilder> (scheme) }
    {
        this->generate_builder ();
    }

    /**
     * ChannelHashingDifferentiation default destructor.
     */
    ~ChannelHashingDifferentiation () override = default;

    /**
     * bind_builder: Binds the differentiation function to use in m_func_build_token, which is used
     * to generate the differentiation tokens.
     * It should be used whenever the classifiers are changed.
     */
    void bind_builder () override
    {
        std::lock_guard<std::mutex> guard (this->m_lock);
        this->generate_builder ();
    }

    /**
     * set_classifiers: Set the classifiers to consider for performing the differentiation of
     * requests.
     * @param workflow Boolean that defines if the workflow identifier classifier should be
     * considered for differentiating requests.
     * @param operation_type Boolean that defines if the operation type classifier should be
     * considered for differentiating requests.
     * @param operation_context Boolean that defines if the operation context classifier should be
     * considered for differentiating requests.
     */
    void set_classifiers (bool workflow, bool operation_type, bool operation_context) override
    {
        std::lock_guard<std::mutex> guard (this->m_lock);
        this->m_use_workflow = workflow;
        this->m_use_operation_type = operation_type;
        this->m_use_operation_context = operation_context;
    }

    /**
     * build_differentiation_token: Generate a differentiation token based on the classifiers set
     * in this builder. It invokes m_func_build_token to calculate the resulting differentiation
     * token, and is bind to the respective classifier function.
     * @param workflow_id Workflow identifier value to be classified.
     * @param operation_type Operation type value to be classified.
     * @param operation_context Operation context value to be classified.
     * @param hash_value Address to store the result of differentiation token generated with
     * m_func_build_token.
     */
    void build_differentiation_token (const uint32_t& workflow_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value) const override
    {
        this->m_func_build_token (workflow_id, operation_type, operation_context, hash_value);
    }

    /**
     * build_differentiation_token: generate a differentiation token based on the classifiers set
     * in this builder, specified through ChannelDifferentiationTuple. It invokes m_func_build_token
     * to calculate the resulting differentiation token, and is bind to the respective classifier
     * function.
     * @param differentiation_tuple ChannelDifferentiationTuple containing the classifiers to build
     * the differentiation token.
     * @param hash_value Address to store the result of differentiation token generated with
     * m_func_build_token.
     */
    void build_differentiation_token (const ChannelDifferentiationTuple& differentiation_tuple,
        uint32_t& hash_value) const override
    {
        this->m_func_build_token (differentiation_tuple.get_workflow_identifier (),
            differentiation_tuple.get_operation_type (),
            differentiation_tuple.get_operation_context (),
            hash_value);
    }
};
} // namespace paio::differentiation

#endif // PAIO_CHANNEL_HASHING_DIFFERENTIATION_HPP
