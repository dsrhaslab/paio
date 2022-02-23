/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/differentiation/murmurhash_token_builder.hpp>

namespace paio::differentiation {

// MurmurHashTokenBuilder default constructor.
MurmurHashTokenBuilder::MurmurHashTokenBuilder ()
{
    Logging::log_debug ("MurmurHashTokenBuilder default constructor.");
    this->bind_function ();
}

// MurmurHashTokenBuilder parameterized constructor.
MurmurHashTokenBuilder::MurmurHashTokenBuilder (const HashingScheme& scheme) :
    m_hashing_scheme { scheme }
{
    Logging::log_debug ("MurmurHashTokenBuilder parameterized constructor.");
    this->bind_function ();
}

// WrapperMurmurHash3_x86_32 call. Wrapper function around the MurmurHash3_x86_32 method.
void MurmurHashTokenBuilder::WrapperMurmurHash3_x86_32 (const void* message_token,
    const int& message_size,
    void* token)
{
    MurmurHash3::MurmurHash3_x86_32 (message_token, message_size, token);
}

// WrapperMurmurHash3_x86_128 call. Wrapper function around the MurmurHash3_x86_128 method.
void MurmurHashTokenBuilder::WrapperMurmurHash3_x86_128 (const void* message_token,
    const int& message_size,
    void* token)
{
    MurmurHash3::MurmurHash3_x86_128 (message_token, message_size, token);
}

// WrapperMurmurHash3_x64_128 call. Wrapper function around the MurmurHash3_x64_128 method.
void MurmurHashTokenBuilder::WrapperMurmurHash3_x64_128 (const void* message_token,
    const int& message_size,
    void* token)
{
    MurmurHash3::MurmurHash3_x64_128 (message_token, message_size, token);
}

// bind_function call. Binds the hashing function to use in m_murmurhash_function.
void MurmurHashTokenBuilder::bind_function ()
{
    Logging::log_debug ("MurmurHashTokenBuilder: binding Murmurhash function.");

    switch (this->m_hashing_scheme) {
        case HashingScheme::MurmurHash_x86_32:
            // binding of MurmurHash3_x86_32 hashing method
            this->m_murmurhash_function
                = [this] (const void* message_token, int size, void* token) {
                      this->WrapperMurmurHash3_x86_32 (message_token, size, token);
                  };
            break;

        case HashingScheme::MurmurHash_x86_128:
            // binding of MurmurHash3_x86_128 hashing method
            this->m_murmurhash_function
                = [this] (const void* message_token, int size, void* token) {
                      this->WrapperMurmurHash3_x86_128 (message_token, size, token);
                  };
            break;

        case HashingScheme::MurmurHash_x64_128:
            // binding of MurmurHash3_x64_128 hashing method
            this->m_murmurhash_function
                = [this] (const void* message_token, int size, void* token) {
                      this->WrapperMurmurHash3_x64_128 (message_token, size, token);
                  };
            break;

        default:
            Logging::log_error ("MurmurHashTokenBuilder: No binding performed.");
            break;
    }
}

// generate_differentiation_token call. Generate a differentiation token object.
void MurmurHashTokenBuilder::generate_differentiation_token (const void* message_token,
    const int& message_size,
    void* token)
{
    this->m_murmurhash_function (message_token, message_size, token);
}
} // namespace paio::differentiation
