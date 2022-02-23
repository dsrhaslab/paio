/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_TOKEN_BUILDER_HPP
#define PAIO_TOKEN_BUILDER_HPP

#include <paio/options/options.hpp>

namespace paio::differentiation {

/**
 * DifferentiationTokenBuilder abstract class.
 * A differentiation token builder is an object that allows the creation of differentiation tokens,
 * which are used for classifying and differentiating requests at both Channel and EnforcementObject
 * levels. The DifferentiationTokenBuilder class provides the means to create custom token builders.
 * Currently, PAIO implements a MurmurHash token builder.
 */
class DifferentiationTokenBuilder {

public:
    /**
     * DifferentiationTokenBuilder default constructor.
     */
    DifferentiationTokenBuilder () = default;

    /**
     * DifferentiationTokenBuilder default destructor.
     */
    virtual ~DifferentiationTokenBuilder () = default;

    /**
     * generate_differentiation_token: given a message and its size, this call generates a
     * differentiation token object.
     * @param message Message to be used for the generation of the differentiation token.
     * @param size Size of the message.
     * @param token Pointer to an object that will store the differentiation token.
     */
    virtual void generate_differentiation_token (const void* message, const int& size, void* token)
        = 0;
};

} // namespace paio::differentiation

#endif // PAIO_TOKEN_BUILDER_HPP
