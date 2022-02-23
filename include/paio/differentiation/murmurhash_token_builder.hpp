/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_MURMURHASH_TOKEN_BUILDER_HPP
#define PAIO_MURMURHASH_TOKEN_BUILDER_HPP

#include <cstdint>
#include <paio/differentiation/differentiation_token_builder.hpp>
#include <paio/options/options.hpp>
#include <paio/utils/murmurhash.hpp>

using namespace paio::core;
using namespace paio::options;
using namespace paio::utils;

namespace paio::differentiation {

/**
 * MurmurHashTokenBuilder class.
 * This class implements the DifferentiationTokenBuilder interface for implementing a
 * MurmurHash-based differentiation builder. It generate hashes using the MurmurHash hashing scheme,
 * which was cloned from the awesome SMHasher library (https://github.com/aappleby/smhasher/).
 * MurmurHash3 is the successor to MurmurHash2. It comes in 3 variants:
 * - MurmurHash3_x86_32: a 32-bit version that targets low latency for hash table use;
 * - MurmurHash3_x86_128: a 128-bit version for generating unique identifiers for large blocks of
 * data, one each for x86 platforms;
 * - MurmurHash3_x64_128: a 128-bit version for generating unique identifiers for large blocks of
 * data, one each for x64 platforms.
 * For more information, please the MurmurHash3 wiki page of the awesome SMHasher library
 * (https://github.com/aappleby/smhasher/wiki/MurmurHash3).
 * MurmurHashTokenBuilder class provides two instance variables:
 * - m_hashing_scheme: defines the HashingScheme to be used in the MurmurHashTokenBuilder;
 * - m_murmurhash_function: function pointer to the MurmurHash3 scheme to be used.
 */
class MurmurHashTokenBuilder : public DifferentiationTokenBuilder {

private:
    HashingScheme m_hashing_scheme { HashingScheme::MurmurHash_x86_32 };
    std::function<void (const void*, int, void*)> m_murmurhash_function { nullptr };

    /**
     * WrapperMurmurHash3_x86_32: Wrapper function around the MurmurHash3_x86_32 method.
     * @param message Message to be used for the generation of the differentiation token.
     * @param size Size of the message.
     * @param token Pointer to an object that will store the differentiation token.
     */
    void WrapperMurmurHash3_x86_32 (const void* message, const int& size, void* token);

    /**
     * WrapperMurmurHash3_x86_128: Wrapper function around the MurmurHash3_x86_128 method.
     * @param message Message to be used for the generation of the differentiation token.
     * @param size Size of the message.
     * @param token Pointer to an object that will store the differentiation token.
     */
    void WrapperMurmurHash3_x86_128 (const void* message, const int& size, void* token);

    /**
     * WrapperMurmurHash3_x64_128: Wrapper function around the MurmurHash3_x64_128 method.
     * @param message Message to be used for the generation of the differentiation token.
     * @param size Size of the message.
     * @param token Pointer to an object that will store the differentiation token.
     */
    void WrapperMurmurHash3_x64_128 (const void* message, const int& size, void* token);

    /**
     * bind_function: Binds the hashing function to use in m_murmurhash_function, which is used
     * to generate the differentiation tokens.
     */
    void bind_function ();

public:
    /**
     * MurmurHashTokenBuilder default constructor.
     */
    MurmurHashTokenBuilder ();

    /**
     * MurmurHashTokenBuilder parameterized constructor.
     * @param scheme HashingScheme to be used.
     */
    explicit MurmurHashTokenBuilder (const HashingScheme& scheme);

    /**
     * MurmurHashTokenBuilder default destructor.
     */
    ~MurmurHashTokenBuilder () override = default;

    /**
     * generate_differentiation_token: given a message and its size, this call generates a
     * differentiation token object.
     * @param message Message to be used for the generation of the differentiation token.
     * @param size Size of the message.
     * @param token Pointer to an object that will store the differentiation token.
     */
    void generate_differentiation_token (const void* message_token,
        const int& message_size,
        void* token) override;
};

} // namespace paio::differentiation

#endif // PAIO_MURMURHASH_TOKEN_BUILDER_HPP
