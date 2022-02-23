/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CHANNEL_DIFFERENTIATION_BUILDER_HPP
#define PAIO_CHANNEL_DIFFERENTIATION_BUILDER_HPP

#include <paio/differentiation/channel_differentiation_tuple.hpp>

namespace paio::differentiation {

/**
 * ChannelDifferentiationBuilder abstract class.
 * This class is used for achieving I/O differentiation at the channel level.
 * For each incoming request, PAIO selects the channel that must service it. To do so, it invokes a
 * build_differentiation_token call that verifies the context's classifiers and generates a
 * differentiation token, that will map the request to the respective channel to be enforced.
 * A system designer can implement custom channel differentiation classes, by using the
 * ChannelDifferentiationBuilder.
 * Currently, PAIO supports differentiation by hashing all (valid) classifiers into a single
 * differentiation token, as presented in the ChannelHashingDifferentiation class.
 */
class ChannelDifferentiationBuilder {

public:
    /**
     * ChannelDifferentiationBuilder default constructor.
     */
    ChannelDifferentiationBuilder () = default;

    /**
     * ChannelDifferentiationBuilder default destructor.
     */
    virtual ~ChannelDifferentiationBuilder () = default;

    /**
     * bind_builder: Binds the differentiation function to use in m_func_build_token, which is used
     * to generate the differentiation tokens.
     * It should be used whenever the classifiers are changed.
     */
    virtual void bind_builder () = 0;

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
    virtual void set_classifiers (bool workflow, bool operation_type, bool operation_context) = 0;

    /**
     * build_differentiation_token: generate a differentiation token based on the classifiers set
     * in this builder. It invokes m_func_build_token to calculate the resulting differentiation
     * token, and is bind to the respective classifier function.
     * @param workflow_id Workflow identifier value to be classified.
     * @param operation_type Operation type value to be classified.
     * @param operation_context Operation context value to be classified.
     * @param hash_value Address to store the result of differentiation token generated with
     * m_func_build_token.
     */
    virtual void build_differentiation_token (const uint32_t& workflow_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value) const = 0;

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
    virtual void build_differentiation_token (
        const ChannelDifferentiationTuple& differentiation_tuple,
        uint32_t& hash_value) const = 0;
};
} // namespace paio::differentiation

#endif // PAIO_CHANNEL_DIFFERENTIATION_BUILDER_HPP
