/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_STATUS_HPP
#define PAIO_STATUS_HPP

#include <string>

namespace paio::utils {

/**
 * PStatus class.
 * This class is used to define and determine the status of a given operation. Currently, PStatus
 * supports five types of result status, namely:
 *  - StatusCode::ok: identifies that the function was executed with success;
 *  - StatusCode::notfound: identifies that a given object does not exist or was not found;
 *  - StatusCode::notsupported: identifies if a specific method is not currently supported;
 *  - StatusCode::error: identifies that the function was not successfully executed;
 *  - StatusCode::enforced: identifies if a given rule was enforced;
 *  - StatusCode::nostatus: initialization status of the PStatus object.
 */
class PStatus {
    enum class StatusCode {
        ok = 0,
        notfound = 1,
        notsupported = 2,
        error = 3,
        enforced = 4,
        nostatus = -1
    };

private:
    StatusCode m_state { StatusCode::nostatus };

public:
    /**
     * PStatus default constructor.
     */
    PStatus () = default;

    /**
     * PStatus parameterized constructor.
     * @param code Status code to be set.
     */
    explicit PStatus (StatusCode code);

    /**
     * PStatus default destructor.
     */
    ~PStatus () = default;

    /**
     * OK: Create a new PStatus object with StatusCode::ok.
     * @return Returns a PStatus object, initialized with StatusCode::ok.
     */
    static PStatus OK ();

    /**
     * NotFound: Create a new PStatus object with StatusCode::notfound.
     * @return Returns a PStatus object, initialized with StatusCode::notfound.
     */
    static PStatus NotFound ();

    /**
     * NotSupported: Create a new PStatus object with StatusCode::notsupported.
     * @return Returns a PStatus object, initialized with StatusCode::notsupported.
     */
    static PStatus NotSupported ();

    /**
     * NotFound: Create a new PStatus object with StatusCode::error.
     * @return Returns a PStatus object, initialized with StatusCode::error.
     */
    static PStatus Error ();

    /**
     * NotFound: Create a new PStatus object with StatusCode::enforced.
     * @return Returns a PStatus object, initialized with StatusCode::enforced.
     */
    static PStatus Enforced ();

    /**
     * is_ok: Verifies if the PStatus object has StatusCode::ok.
     * @return Returns true if StatusCode::ok; false otherwise.
     */
    [[nodiscard]] bool is_ok () const;

    /**
     * is_not_found: Verifies if the PStatus object has StatusCode::notfound.
     * @return Returns true if StatusCode::notfound; false otherwise.
     */
    [[nodiscard]] bool is_not_found () const;

    /**
     * is_not_supported: Verifies if the PStatus object has StatusCode::notsupported.
     * @return Returns true if StatusCode::notsupported; false otherwise.
     */
    [[nodiscard]] bool is_not_supported () const;

    /**
     * is_error: Verifies if the PStatus object has StatusCode::error.
     * @return Returns true if StatusCode::error; false otherwise.
     */
    [[nodiscard]] bool is_error () const;

    /**
     * is_enforced: Verifies if the PStatus object has StatusCode::enforced.
     * @return Returns true if StatusCode::enforced; false otherwise.
     */
    [[nodiscard]] bool is_enforced () const;

    /**
     * to_string: Present the StatusCode of the PStatus object in string format.
     * @return Returns a string-formatted StatusCode.
     */
    std::string to_string ();
};
} // namespace paio::utils

#endif // PAIO_STATUS_HPP
