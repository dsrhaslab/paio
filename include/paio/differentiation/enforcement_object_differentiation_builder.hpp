/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_ENFORCEMENT_OBJECT_DIFFERENTIATION_BUILDER_HPP
#define PAIO_ENFORCEMENT_OBJECT_DIFFERENTIATION_BUILDER_HPP

#include <paio/differentiation/enforcement_object_differentiation_pair.hpp>

namespace paio::differentiation {

/**
 * ObjectDifferentiationBuilder abstract class.
 * This class is used for achieving I/O differentiation at the enforcement object level.
 * Since each channel may contain several enforcement objects, for each incoming request, PAIO
 * selects the enforcement object that must service it. To do so, it invokes a
 * build_differentiation_token call that verifies the context's classifiers and generates a
 * differentiation token, that will map the request to the respective enforcement object to be
 * enforced. A system designer can implement custom enforcement object differentiation classes, by
 * using the ObjectDifferentiationBuilder.
 * Currently, PAIO supports differentiation by hashing all (valid) classifiers into a single
 * differentiation token, as presented in the ObjectHashingDifferentiation class.
 */
class ObjectDifferentiationBuilder {

public:
    /**
     * ObjectDifferentiationBuilder default constructor.
     */
    ObjectDifferentiationBuilder () = default;

    /**
     * ObjectDifferentiationBuilder default destructor.
     */
    virtual ~ObjectDifferentiationBuilder () = default;

    /**
     * bind_builder: Binds the differentiation function to use in m_func_build_token, which is used
     * to generate the differentiation tokens.
     * It should be used whenever the classifiers are changed.
     */
    virtual void bind_builder () = 0;

    /**
     * set_classifiers: Set the classifiers to consider for performing the differentiation of
     * requests.
     * @param operation_type Boolean that defines if the operation type classifier should be
     * considered for differentiating requests.
     * @param operation_context Boolean that defines if the operation context classifier should be
     * considered for differentiating requests.
     */
    virtual void set_classifiers (bool operation_type, bool operation_context) = 0;

    /**
     * build_differentiation_token: generate a differentiation token based on the classifiers set
     * in this builder. It invokes m_func_build_token to calculate the resulting differentiation
     * token, and is bind to the respective classifier function.
     * @param operation_type Operation type value to be classified.
     * @param operation_context Operation context value to be classified.
     * @param hash_value Address to store the result of differentiation token generated with
     * m_func_build_token.
     */
    virtual void build_differentiation_token (const uint32_t& operation_type,
        const uint32_t& operation_context,
        uint32_t& hash_value) const = 0;

    /**
     * build_differentiation_token: generate a differentiation token based on the classifiers set
     * in this builder, specified through ObjectDifferentiationPair. It invokes m_func_build_token
     * to calculate the resulting differentiation token, and is bind to the respective classifier
     * function.
     * @param differentiation_pair Differentiation pair to be used to generate the token.
     * @param hash_value Address to store the result of differentiation token generated with
     * m_func_build_token.
     */
    virtual void build_differentiation_token (const ObjectDifferentiationPair& differentiation_pair,
        uint32_t& hash_value) const = 0;
};

} // namespace paio::differentiation

#endif // PAIO_ENFORCEMENT_OBJECT_DIFFERENTIATION_BUILDER_HPP
