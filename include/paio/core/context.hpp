/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CONTEXT_HPP
#define PAIO_CONTEXT_HPP

#include <paio/core/context_propagation_definitions.hpp>
#include <sstream>

namespace paio::core {

/**
 * Context class.
 * A context represents a metadata-like object that contains a set of elements that characterize a
 * request. These elements, or classifiers, include the workflow-id, request type, request size,
 * and the request context, which defines the context of a request.
 * For each request, PAIO generates the corresponding context object that is used for classifying
 * and differentiating the request, so the correct enforcement mechanism can be employed.
 * It contains 5 classifiers:
 *  - m_flow_id: I/O workflow identifier (e.g., tenant, thread-id);
 *  - m_operation_type: type of the submitted operation (e.g., read, open, put, get);
 *  - m_operation_context: context of the submitted operation (e.g., foreground or background tasks,
 *  flush or compaction, ...);
 *  - m_operation_size: size of the submitted operation;
 *  - m_total_operations: number of operations submitted, in case of batch processing (e.g., scans,
 *  iterators, ...).
 * TODO:
 *  - add support for m_previous_exec_time classifier;
 */
class Context {

private:
    long m_workflow_id { -1 };
    int m_operation_type { static_cast<int> (PAIO_GENERAL::no_op) };
    int m_operation_context { static_cast<int> (PAIO_GENERAL::no_op) };
    uint64_t m_operation_size { 0 };
    int m_total_operations { 0 };
    // uint64_t m_previous_exec_time { 0 };

public:
    /**
     * Context default constructor.
     */
    Context () = default;

    /**
     * Context parameterized constructor.
     */
    Context (const long& workflow_id,
        const int& operation_type,
        const int& operation_context,
        const uint64_t& operation_size,
        const int& total_operations) :
        m_workflow_id { workflow_id },
        m_operation_type { operation_type },
        m_operation_context { operation_context },
        m_operation_size { operation_size },
        m_total_operations { total_operations }
    { }

    /**
     * Context default destructor.
     */
    ~Context () = default;

    /**
     * get_workflow_id: Get Context's I/O workflow identifier.
     * @return Return a copy of m_flow_id classifier.
     */
    [[nodiscard]] long get_workflow_id () const
    {
        return this->m_workflow_id;
    }

    /**
     * get_operation_type: Get Context's operation type.
     * @return Return a copy of m_operation_type classifier.
     */
    [[nodiscard]] int get_operation_type () const
    {
        return this->m_operation_type;
    }

    /**
     * get_operation_context: Get Context's operation context.
     * @return Return a copy of the m_operation_context classifier.
     */
    [[nodiscard]] int get_operation_context () const
    {
        return this->m_operation_context;
    }

    /**
     * get_operation_size: Get Context's operation size.
     * @return Return a copy of m_operation_size classifier.
     */
    [[nodiscard]] uint64_t get_operation_size () const
    {
        return this->m_operation_size;
    }

    /**
     * get_total_operations: Get Context's total operations.
     * @return Return a copy of m_total_operations classifier.
     */
    [[nodiscard]] int get_total_operations () const
    {
        return this->m_total_operations;
    }

    /**
     * to_string: Return a string object containing the Context identifiers.
     * @return Returns a parsed string depicting the contents of the Context object.
     */
    [[nodiscard]] std::string to_string () const
    {
        std::stringstream stream;
        stream << "Context {";
        stream << this->m_workflow_id << ", ";
        stream << this->m_operation_type << ", ";
        stream << this->m_operation_context << ", ";
        stream << this->m_operation_size << ", ";
        stream << this->m_total_operations << "}";

        return stream.str ();
    };
};
} // namespace paio::core

#endif // PAIO_CONTEXT_HPP
