/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CHANNELDIFFERENTIATIONTUPLE_HPP
#define PAIO_CHANNELDIFFERENTIATIONTUPLE_HPP

#include <cstdint>
#include <cstdio>
#include <paio/utils/logging.hpp>
#include <sstream>
#include <string>

using namespace paio::utils;

namespace paio::differentiation {

/**
 * ChannelDifferentiationTuple class.
 * This class provides the unified object that contains the I/O classifiers to be considered in the
 * classification and differentiation of requests in channels.
 * Currently, a ChannelDifferentiationTuple consists of three variables, which correspond to the
 * respective I/O classifiers:
 *  - m_workflow_identifier: workflow identifier I/O classifier to be considered in the
 *  differentiation;
 *  - m_operation_type: operation type I/O classifier to be considered in the differentiation;
 *  - m_operation_context: operation context I/O classifier to be considered in the differentiation.
 */
class ChannelDifferentiationTuple {

private:
    uint32_t m_workflow_identifier { 0 };
    uint32_t m_operation_type { 0 };
    uint32_t m_operation_context { 0 };

public:
    /**
     * ChannelDifferentiationTuple default constructor.
     */
    ChannelDifferentiationTuple () = default;

    /**
     * ChannelDifferentiationTuple parameterized constructor.
     * @param workflow_id Defines the workflow identifier classifier to perform the differentiation.
     * @param operation_type Defines the operation type classifier to perform the differentiation.
     * @param operation_context Defines the operation context classifier to perform the
     * differentiation.
     */
    ChannelDifferentiationTuple (const uint32_t& workflow_id,
        const uint32_t& operation_type,
        const uint32_t& operation_context) :
        m_workflow_identifier { workflow_id },
        m_operation_type { operation_type },
        m_operation_context { operation_context }
    {
        Logging::log_debug ("ChannelDifferentiationPair parameterized constructor.");
    }

    /**
     * ChannelDifferentiationTuple default destructor.
     */
    ~ChannelDifferentiationTuple () = default;

    /**
     * get_workflow_identifier: get the workflow identifier value.
     * @return Returns a copy of the m_workflow_identifier parameter.
     */
    [[nodiscard]] uint32_t get_workflow_identifier () const
    {
        return this->m_workflow_identifier;
    }

    /**
     * get_operation_type: get the operation type value.
     * @return Returns a copy of the m_operation_type parameter.
     */
    [[nodiscard]] uint32_t get_operation_type () const
    {
        return this->m_operation_type;
    }

    /**
     * get_operation_context: get the operation context value.
     * @return Returns a copy of the m_operation_context parameter.
     */
    [[nodiscard]] uint32_t get_operation_context () const
    {
        return this->m_operation_context;
    }

    /**
     * to_string: Convert ChannelDifferentiationTuple class elements in string-based format.
     * @return Returns a string with the ChannelDifferentiationTuple elements.
     */
    [[nodiscard]] std::string to_string () const
    {
        std::stringstream stream;
        stream << "ChannelDifferentiationTuple {";
        stream << this->m_workflow_identifier << "; ";
        stream << this->m_operation_type << "; ";
        stream << this->m_operation_context << "}";

        return stream.str ();
    }
};
} // namespace paio::differentiation

#endif // PAIO_CHANNELDIFFERENTIATIONTUPLE_HPP
