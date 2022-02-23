/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_ENFORCEMENT_OBJECT_DIFFERENTIATION_PAIR_HPP
#define PAIO_ENFORCEMENT_OBJECT_DIFFERENTIATION_PAIR_HPP

#include <cstdint>
#include <cstdio>
#include <paio/utils/logging.hpp>
#include <string>

using namespace paio::utils;

namespace paio::differentiation {

/**
 * ObjectDifferentiationPair class.
 * This class provides the unified object that contains the I/O classifiers to be considered in the
 * classification and differentiation of requests in enforcement objects.
 * Currently, an ObjectDifferentiationPair consists of two variables, which correspond to the
 * respective I/O classifiers:
 *  - m_operation_type: operation type I/O classifier to be considered in the differentiation;
 *  - m_operation_context: operation context I/O classifier to be considered in the differentiation.
 */
class ObjectDifferentiationPair {

private:
    uint32_t m_operation_type { 0 };
    uint32_t m_operation_context { 0 };

public:
    /**
     * ObjectDifferentiationPair default constructor.
     */
    ObjectDifferentiationPair () = default;

    /**
     * ObjectDifferentiationPair parameterized constructor.
     * @param operation_type Defines the operation type classifier to perform the differentiation.
     * @param operation_context Defines the operation context classifier to perform the
     * differentiation.
     */
    ObjectDifferentiationPair (const uint32_t& operation_type, const uint32_t& operation_context) :
        m_operation_type { operation_type },
        m_operation_context { operation_context }
    {
        Logging::log_debug ("ObjectDifferentiationPair parameterized constructor");
    }

    /**
     * ObjectDifferentiationPair default destructor.
     */
    ~ObjectDifferentiationPair () = default;

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
     * to_string: Convert ObjectDifferentiationPair class elements in string-based format.
     * @return Returns a string with the ObjectDifferentiationPair elements.
     */
    [[nodiscard]] std::string to_string () const
    {
        std::stringstream stream;
        stream << "ObjectDifferentiationPair {";
        stream << this->m_operation_type << "; ";
        stream << this->m_operation_context << "}";

        return stream.str ();
    }
};
} // namespace paio::differentiation
#endif // PAIO_ENFORCEMENT_OBJECT_DIFFERENTIATION_PAIR_HPP
