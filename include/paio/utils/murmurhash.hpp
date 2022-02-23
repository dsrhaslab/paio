/**
 *  MurmurHash3 was written by Austin Appleby, and is placed in the public domain.
 *  The author hereby disclaims copyright to this source code.
 **/

#ifndef PAIO_MURMURHASH_HPP
#define PAIO_MURMURHASH_HPP

#include <cstdint>

namespace paio::utils {

/**
 * MurmurHash3 class.
 * This class allows to generate hashes using the MurmurHash hashing scheme, which was cloned from
 * the awesome SMHasher library (https://github.com/aappleby/smhasher/).
 * MurmurHash3 is the successor to MurmurHash2. It comes in 3 variants:
 * - MurmurHash3_x86_32: a 32-bit version that targets low latency for hash table use;
 * - MurmurHash3_x86_128: a 128-bit version for generating unique identifiers for large blocks of
 * data, one each for x86 platforms;
 * - MurmurHash3_x64_128: a 128-bit version for generating unique identifiers for large blocks of
 * data, one each for x64 platforms.
 * For more information, please the MurmurHash3 wiki page of the awesome SMHasher library
 * (https://github.com/aappleby/smhasher/wiki/MurmurHash3).
 * TODO:
 *  - this should extend a main class, for example, HashingScheme class.
 */
class MurmurHash3 {

private:
    static const uint32_t m_seed = 42;

public:
    /**
     * MurmurHash_x86_32: Converts a given message into a 32-bit hash.
     * Best used for low latency scenarios.
     * @param message Message to be hashed.
     * @param size Message size.
     * @param token Pointer to an object that stores the resulting hash.
     */
    static void MurmurHash3_x86_32 (const void* message, int size, void* token);

    /**
     * MurmurHash3_x86_128: Converts a given message into a 128-bit hash.
     * Suited for x86 platforms.
     * @param message Message to be hashed.
     * @param size Message size.
     * @param token Pointer to an object that stores the resulting hash.
     */
    static void MurmurHash3_x86_128 (const void* message, int size, void* token);

    /**
     * MurmurHash3_x64_128: Converts a given message into a 128-bit hash.
     * Suited for x64 platforms.
     * @param message Message to be hashed.
     * @param size Message size.
     * @param token Pointer to an object that stores the resulting hash.
     */
    static void MurmurHash3_x64_128 (const void* message, int size, void* token);
};
} // namespace paio::utils

#endif // PAIO_MURMURHASH_HPP
