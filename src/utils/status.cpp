/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/utils/status.hpp>

namespace paio::utils {

// PStatus parameterized constructor.
PStatus::PStatus (PStatus::StatusCode code) : m_state { code }
{ }

// OK call. Create a new PStatus object with StatusCode::ok.
PStatus PStatus::OK ()
{
    return PStatus (StatusCode::ok);
}

// NotFound call. Create a new PStatus object with StatusCode::notfound.
PStatus PStatus::NotFound ()
{
    return PStatus (StatusCode::notfound);
}

// NotSupported call. Create a new PStatus object with StatusCode::notsupported.
PStatus PStatus::NotSupported ()
{
    return PStatus (StatusCode::notsupported);
}

// Enforced call. Create a new PStatus object with StatusCode::enforced.
PStatus PStatus::Enforced ()
{
    return PStatus (StatusCode::enforced);
}

// Error call. Create a new PStatus object with StatusCode::error.
PStatus PStatus::Error ()
{
    return PStatus (StatusCode::error);
}

// is_ok call. Verifies if the PStatus object has StatusCode::ok.
bool PStatus::is_ok () const
{
    return (this->m_state == StatusCode::ok);
}

// is_not_found call. Verifies if the PStatus object has StatusCode::notfound.
bool PStatus::is_not_found () const
{
    return (this->m_state == StatusCode::notfound);
}

// is_not_supported call. Verifies if the PStatus object has StatusCode::notsupported.
bool PStatus::is_not_supported () const
{
    return (this->m_state == StatusCode::notsupported);
}

// is_enforced call. Verifies if the PStatus object has StatusCode::enforced.
bool PStatus::is_enforced () const
{
    return (this->m_state == StatusCode::enforced);
}

// is_error call. Verifies if the PStatus object has StatusCode::error.
bool PStatus::is_error () const
{
    return (this->m_state == StatusCode::error);
}

// to_string call. Present the PStatus' StatusCode in string format.
std::string PStatus::to_string ()
{
    std::string state_string;

    switch (this->m_state) {
        case StatusCode::ok:
            state_string = "OK";
            break;

        case StatusCode::notfound:
            state_string = "NotFound";
            break;

        case StatusCode::notsupported:
            state_string = "NotSupported";
            break;

        case StatusCode::enforced:
            state_string = "Enforced";
            break;

        case StatusCode::error:
            state_string = "Error";
            break;

        case StatusCode::nostatus:
        default:
            state_string = "Unknown Status";
            break;
    }

    return state_string;
}

} // namespace paio::utils
