/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_RESULT_HPP
#define PAIO_RESULT_HPP

#include <cstring>
#include <paio/utils/logging.hpp>
#include <sstream>
#include <string>

using namespace paio::utils;

namespace paio::enforcement {

enum class ResultStatus { success = 0, error = 1, none = 2 };

/**
 * Result class.
 * This class is used for storing the result of a request after being enforced by an enforcement
 * object. The result object is created by the original workflow at the Instance interface (e.g.,
 * PosixLayer, PaioKvsBackend,...), is passed as a reference throughout the data plane path,
 * is updated by the enforcement object, and returned again to the Instance interface.
 * It comprises 5 parameters:
 *  - m_ticket_id: defines the ticket identifier that the results respects to. This is useful to
 *  ensure the order of requests/results when processed in the background.
 *  - m_result_status: defines the result status of the I/O request after being enforced. It
 *  presents three status: ResultStatus::success, ResultStatus::error, and ResultStatus::none.
 *  - m_has_content: defines if the Result object has content or not. This is useful for preventing
 *  unnecessary buffer allocations and checking.
 *  - m_content_size: defines the size of the buffer to allocate the data generated from the
 *  enforcement of the I/O request in the enforcement object.
 *  - m_content: is a buffer that contains the data resulted from the enforcement of the I/O request
 *  in the enforcement object. Depending on the enforcement mechanisms applied, the m_content buffer
 *  can contain the original or transformed request data (e.g., encryption, compression), metadata
 *  (e.g., file descriptors and paths -- data placement), or both (e.g., deduplication, containing
 *  both deduplicated data and the index).
 *  TODO:
 *   - create dedicated testing class (tests/paio_result_test.cpp)
 */
class Result {
private:
    uint64_t m_ticket_id { 0 }; // Result object respects to a specific ticket
    ResultStatus m_result_status { ResultStatus::none };
    bool m_has_content { false };
    std::size_t m_content_size { 0 };
    unsigned char* m_content { nullptr };

public:
    /**
     * Result default constructor.
     */
    Result ();

    /**
     * Result parameterized constructor.
     * Only initializes the ticket identifier.
     * @param ticket_id Ticket identifier.
     */
    explicit Result (const uint64_t& ticket_id);

    /**
     * Result parameterized constructor.
     * Initializes the Result object without any content of the request.
     * @param ticket_id Ticket identifier.
     * @param status Result status.
     */
    Result (const uint64_t& ticket_id, const ResultStatus& status);

    /**
     * Result parameterized constructor.
     * Initializes all instance variables, including the request content.
     * @param ticket_id Ticket identifier.
     * @param status Result status.
     * @param has_content Boolean that defines if Result will include content or not.
     * @param content_size Size of the data buffer that is included in the Result object.
     * @param buffer Buffer with the result of the enforcement mechanism, which can contain data
     * and/or metadata of the request, or other type of content that the system designer might find
     * helpful.
     */
    Result (const uint64_t& ticket_id,
        const ResultStatus& status,
        const bool& has_content,
        const std::size_t& content_size,
        unsigned char* buffer);

    /**
     * Result copy constructor.
     * Rule of five.
     * @param result Result object to be copied.
     */
    Result (const Result& result);

    /**
     * Result move constructor.
     * Rule of five.
     * @param result Result object to be moved.
     */
    Result (Result&& result) noexcept;

    /**
     * Result assignment operator.
     * Rule of five.
     * @param result Result object to be assigned.
     * @return Pointer to this Result object.
     */
    Result& operator= (const Result& result);

    /**
     * Result move assignment operator.
     * Rule of five.
     * @param result Result object to be assigned.
     * @return Pointer to this Result object.
     */
    Result& operator= (Result&& result) noexcept;

    /**
     * Result destructor.
     * Rule of five.
     */
    ~Result ();

    /**
     * get_ticket_id: Get Result's ticket identifier.
     * @return Return a copy of the m_ticket_id parameter.
     */
    [[nodiscard]] uint64_t get_ticket_id () const;

    /**
     * get_result_status: Get Result's status.
     * @return Return a copy of the m_result_status parameter.
     */
    [[nodiscard]] ResultStatus get_result_status () const;

    /**
     * get_has_content: Get Result's has content value.
     * @return Return a copy of the m_has_content parameter.
     */
    [[nodiscard]] bool get_has_content () const;

    /**
     * get_content_size: Get Result's content size.
     * @return Return a copy of the m_content_size parameter.
     */
    [[nodiscard]] std::size_t get_content_size () const;

    /**
     * get_content: Get a pointer of the Result's content.
     * @return Return an unsigned char pointer to Result's m_content.
     */
    [[nodiscard]] unsigned char* get_content () const;

    /**
     * set_ticket_id: Update the Result's ticket identifier.
     * @param ticket_id New ticket identifier value.
     */
    void set_ticket_id (const uint64_t& ticket_id);

    /**
     * set_result_status: Update the Result's status.
     * @param status New status value.
     */
    void set_result_status (const ResultStatus& status);

    /**
     * set_has_content: Update the Result's has_content value.
     * @param has_content New value for the has_content parameter.
     */
    void set_has_content (const bool& has_content);

    /**
     * set_content_size: Update the Result's content size.
     * @param size Size of the Result's content.
     */
    void set_content_size (const std::size_t& size);

    /**
     * set_content: Update the Result's content.
     * @param size Size of the buffer to be allocated.
     * @param buffer Buffer with the data to be included in Result's content.
     */
    void set_content (const std::size_t& size, const unsigned char* buffer);

    /**
     * to_string: Generate a string with the Result instance values.
     * @return Returns the Result instance values in string format.
     */
    [[nodiscard]] std::string to_string () const;
};
} // namespace paio::enforcement

#endif // PAIO_RESULT_HPP
