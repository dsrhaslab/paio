/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_TICKET_HPP
#define PAIO_TICKET_HPP

#include <cstdint>
#include <cstring>
#include <paio/core/context_propagation_definitions.hpp>
#include <paio/options/options.hpp>
#include <sstream>

using namespace paio::core;

namespace paio::enforcement {

/**
 * Ticket class.
 * The Ticket class characterizes an I/O request of a given workflow, comprising its average cost,
 * the number of operations, and the I/O content of the respective request.
 * All tickets are created with an identifier (m_ticket_id), the number of operations of the given
 * request (m_operations), the I/O payload (m_payload) i.e., average cost of the request based on
 * the total of operations and the size of the buffer, and the operation context (m_context), which
 * specifies the context of the operation and can be used to select which operation to perform (for
 * instance, if its an encryption object, for write-based request we want to encrypt, while for
 * read-based request we want to decrypt).
 * A ticket can be created differently depending on the type of the request and its granularity.
 * For content-less based request (e.g., rate limiting), the Ticket does not initialize
 * m_buffer_size and m_buffer parameters. When we want to apply a specific enforcement object of the
 * request's content, we pass the size of the buffer and the content to be handled (e.g., request's
 * buffer, metadata).
 * TODO:
 *  - [feature] consider a m_previous_operation_time parameter, that adjusts and
 *  fine-tunes the cost of each I/O operation throughout time, based on the time of that the
 *  previous I/O operation has taken to execute. For instance, if the previous operation was taken
 *  more time than the expected, the current operation will be set to consume less tokens so it can
 *  normalize/amortize the overall cost of I/O requests;
 *  - create dedicated testing class (tests/paio_ticket_test.cpp)
 */
class Ticket {

private:
    uint64_t m_ticket_id { 0 };
    int m_operations { 0 };
    long m_payload { 1 };
    int m_operation_type { static_cast<int> (PAIO_GENERAL::no_op) };
    int m_operation_context { static_cast<int> (PAIO_GENERAL::no_op) };
    std::size_t m_buffer_size { 0 };
    unsigned char* m_buffer { nullptr };

public:
    /**
     * Ticket default constructor.
     */
    Ticket ();

    /**
     * Ticket parameterized constructor.
     * This constructor does not initialize m_buffer_size and m_buffer parameters.
     * @param ticket_id Ticket's identifier.
     * @param total_operations Total operations of the I/O request. Each I/O request can be
     * made of several operations (e.g., batch operators, iterators, ...).
     * @param payload I/O cost of the submitted request.
     * @param operation_context I/O context of the current operation.
     */
    Ticket (const uint64_t& ticket_id,
        const int& total_operations,
        const long& payload,
        const int& operation_type,
        const int& operation_context);

    /**
     * Ticket parameterized constructor with request content (buffer).
     * This constructor initializes both m_buffer_size and m_buffer parameters.
     * @param ticket_id Ticket's identifier.
     * @param total_operations Total operations of the I/O request. Each I/O request can be
     * made of several operations (e.g., batch operators, iterators, ...).
     * @param payload I/O cost of the submitted request.
     * @param operation_context I/O context of the current operation.
     * @param size Size of the buffer to be passed to the Ticket.
     * @param data Buffer (i.e, data content) of the request.
     */
    Ticket (const uint64_t& ticket_id,
        const int& total_operations,
        const long& payload,
        const int& operation_type,
        const int& operation_context,
        const std::size_t& size,
        const unsigned char* data);

    /**
     * Ticket copy constructor.
     * Rule of three (user-defined copy constructor).
     * @param ticket Ticket object to be copied.
     */
    Ticket (const Ticket& ticket);

    /**
     * Ticket assignment operator.
     * Rule of three (user-defined assignment operator).
     */
    Ticket& operator= (const Ticket& ticket);

    /**
     * Ticket destructor.
     * Rule of three (user-defined destructor).
     */
    ~Ticket ();

    /**
     * get_ticket_id: Get Ticket's identifier.
     * @return Returns a copy of m_ticket_id.
     */
    [[nodiscard]] uint64_t get_ticket_id () const;

    /**
     * get_operations: Get Ticket's total of operations of the I/O request.
     * @return Returns a copy of m_operations.
     */
    [[nodiscard]] int get_total_operations () const;

    /**
     * get_payload: Get Ticket's I/O payload i.e., cost in "tokens" or any other
     * unit of that specific operation.
     * @return Returns a copy of m_payload.
     */
    [[nodiscard]] long get_payload () const;

    /**
     * get_operation_type: Get Ticket's operation type of the I/O request.
     * @return Returns a copy of m_operation_type.
     */
    [[nodiscard]] int get_operation_type () const;

    /**
     * get_operation_context: Get Ticket's operation context of the I/O request.
     * @return Returns a copy of m_operation_context.
     */
    [[nodiscard]] int get_operation_context () const;

    /**
     * get_buffer_size: Get Ticket's buffer size.
     * @return Returns a copy of m_size.
     */
    [[nodiscard]] std::size_t get_buffer_size () const;

    /**
     * get_buffer: Get the Ticket's content pointer.
     * @return Returns a pointer to the beginning of m_buffer.
     */
    [[nodiscard]] unsigned char* get_buffer () const;

    /**
     * to_string: Return a string object containing the Ticket identifiers.
     * @return Returns a parsed string depicting the contents of the Ticket object.
     */
    [[nodiscard]] std::string to_string () const;
};
} // namespace paio::enforcement

#endif // PAIO_TICKET_HPP
