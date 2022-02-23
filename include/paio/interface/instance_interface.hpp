/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_INSTANCE_INTERFACE_HPP
#define PAIO_INSTANCE_INTERFACE_HPP

#include <paio/core/context.hpp>
#include <paio/stage/paio_stage.hpp>

namespace paio {

/**
 * InstanceInterface class.
 * The InstanceInterface class is used to achieve communication between an I/O layer (e.g.,
 * key-value store, file system, object store) and a PAIO data plane stage. This interface provides
 * the means to establish the connection between workflows and the PAIO internal enforcement
 * mechanisms (e.g., rate-limiting, data placement, compression, encryption).
 * Apart from the traditional constructors, destructors, getters, and setters, this class exposes
 * two main methods:
 *  - build_context_object: that allow building Context objects (which contain the necessary
 * metadata/classifiers to differentiate and classify a request);
 *  - enforce_request: that allows enforcing specific storage mechanisms over (application) I/O
 *  requests.
 * Furthermore, it provides the following instance variables:
 *  - m_paio_stage: Shared pointer to the PaioStage object;
 *  - m_default_workflow_id: defines the workflow identifier of the InstanceInterface;
 *  - m_default_secondary_workflow_identifier: defines the name/secondary identifier of the
 *  InstanceInterface;
 *  - m_default_operation_type: defines the default operation type of the InstanceInterface;
 *  - m_default_operation_context: defines the default operation context of the InstanceInterface.
 * For an Application to interact with PAIO, it can either use PaioInstance, that provides RAW
 * access to build_context_object and enforce_request methods, or by building a custom layer and
 * simply replace the original method to a PAIO-enabled one. This is the case for the PosixLayer
 * and KvsLayer classes.
 * TODO:
 *  - add tests for all interface classes (1. for testing (class type) for the main/general methods,
 *  and 2. for testing (instance type) for the specific methods).
 */
class InstanceInterface {

protected:
    std::shared_ptr<PaioStage> m_paio_stage { nullptr };
    long m_default_workflow_id { -1 };
    std::string m_default_secondary_workflow_identifier { "workflow" };
    int m_default_operation_type { static_cast<int> (PAIO_GENERAL::no_op) };
    int m_default_operation_context { static_cast<int> (PAIO_GENERAL::no_op) };

    /**
     * set_default_workflow_id: Set new value in the workflow identifier parameter.
     * This method is not thread-safe.
     * @param workflow_id New value to be set in m_default_workflow_id.
     */
    virtual void set_default_workflow_id (const long& workflow_id)
    {
        this->m_default_workflow_id = workflow_id;
    }

    /**
     * set_default_operation_type: Set new value in the default operation type parameter.
     * This method is not thread-safe.
     * @param operation_type New value to be set in m_default_operation_type.
     */
    virtual void set_default_operation_type (const int& operation_type)
    {
        this->m_default_operation_type = operation_type;
    }

    /**
     * set_default_operation_context: Set new value in the default operation context parameter.
     * This method is not thread-safe.
     * @param operation_context New value to be set in m_default_operation_context.
     */
    virtual void set_default_operation_context (const int& operation_context)
    {
        this->m_default_operation_context = operation_context;
    }

    /**
     * set_default_secondary_workflow_identifier: Set new value in the workflow secondary identifier
     * parameter.
     * This method is not thread-safe.
     * @param workflow_secondary_id New value to be set in m_default_secondary_workflow_identifier.
     */
    virtual void set_default_secondary_workflow_identifier (
        const std::string& workflow_secondary_id)
    {
        this->m_default_secondary_workflow_identifier = workflow_secondary_id;
    }

    /**
     * get_default_workflow_id: Get the value of the workflow identifier parameter.
     * This method is not thread-safe.
     * @return Returns a copy of the m_default_workflow_id parameter.
     */
    [[nodiscard]] long get_default_workflow_id () const
    {
        return this->m_default_workflow_id;
    }

    /**
     * get_default_secondary_workflow_identifier: Get the value of the secondary workflow identifier
     * parameter.
     * This method is not thread-safe.
     * @return Returns a copy of the m_default_secondary_workflow_identifier parameter.
     */
    [[nodiscard]] std::string get_default_secondary_workflow_identifier () const
    {
        return this->m_default_secondary_workflow_identifier;
    }

    /**
     * get_default_operation_type: Get the value of the operation type parameter.
     * This method is not thread-safe.
     * @return Returns a copy of the m_default_operation_type parameter.
     */
    [[nodiscard]] int get_default_operation_type () const
    {
        return this->m_default_operation_type;
    }

    /**
     * get_default_operation_context: Get the value of the operation context parameter.
     * This method is not thread-safe.
     * @return Returns a copy of the m_default_operation_context parameter.
     */
    [[nodiscard]] int get_default_operation_context () const
    {
        return this->m_default_operation_context;
    }

public:
    /**
     * InstanceInterface default constructor.
     */
    InstanceInterface () = default;

    /**
     * InstanceInterface (explicit) parameterized constructor.
     * This constructor only initializes the m_paio_stage parameter (shared pointer to a PaioStage),
     * leaving the remainder as default.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     */
    explicit InstanceInterface (std::shared_ptr<PaioStage> stage_ptr) :
        m_paio_stage { std::move (stage_ptr) }
    {
        Logging::log_debug ("InstanceInterface (explicit) constructor.");
    };

    /**
     * InstanceInterface parameterized constructor.
     * This constructor initializes the m_paio_stage parameter (shared pointer to a PaioStage),
     * as well as the default operation classifiers, such as the default workflow identifier (which
     * can be given by the thread-id, tenant, etc), the operation type, and the operation context.
     * It also enables the use of default operation type and operation context.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     * @param workflow_id Defines the workflow identifier of the InstanceInterface.
     * @param operation_type Defines the operation type of the InstanceInterface.
     * @param operation_context Defines the operation context of the InstanceInterface.
     */
    InstanceInterface (std::shared_ptr<PaioStage> stage_ptr,
        const long& workflow_id,
        const int& operation_type,
        const int& operation_context) :
        m_paio_stage { std::move (stage_ptr) },
        m_default_workflow_id { workflow_id },
        m_default_operation_type { operation_type },
        m_default_operation_context { operation_context }
    {
        Logging::log_debug ("InstanceInterface parameterized constructor.");
    }

    /**
     * InstanceInterface default destructor.
     */
    virtual ~InstanceInterface () = default;

    /**
     * build_context_object: Virtual method for building Context objects for the differentiation
     * and classification of application requests at the Paio data plane stage.
     * @return Returns the respective Context object. This operation follows the Return Value
     * Optimization (RVO).
     */
    virtual Context build_context_object () = 0;

    /**
     * build_context_object: Virtual method for building Context objects for the differentiation and
     * classification of application requests at the Paio data plane stage.
     * @param workflow_id Defines the workflow identifier used to submit the request (from the
     * application to the data plane stage).
     * @param operation_type Defines the type of the submitted operation (from the application to
     * the data plane stage).
     * @param operation_context Defines the context of the submitted operation (from the application
     * to the data plane stage).
     * @param operation_size Defines the size of the operation to be enforced.
     * @param total_operations Defines the total number of operations that will be enforced.
     * @return Returns the respective Context object. This operation follows the Return Value
     * Optimization (RVO).
     */
    virtual Context build_context_object (const long& workflow_id,
        const int& operation_type,
        const int& operation_context,
        const uint64_t& operation_size,
        const int& total_operations)
        = 0;

    /**
     * enforce: Virtual method for enforcing I/O requests at the data plane stage.
     * This method receives the Context object (which specifies all I/O classifiers) and a Result
     * object (which will contain the result after the enforcement). Further, this method assumes
     * that a performance-oriented mechanism will be applied, and thus, the request's content
     * (i.e., buffer) and size are set with nullptr and 0, respectively.
     * @param context Context object containing all necessary metadata/classifiers to enforce the
     * I/O request.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     */
    virtual void enforce (const Context& context, Result& result)
    {
        bool enforced = false;

        while (!enforced) {
            // submit Context object and request's content to the data plane stage
            PStatus status = this->m_paio_stage->enforce_request (context, nullptr, 0, result);

            if (status.is_enforced ()) {
                enforced = true;
            } else {
                std::fprintf (stderr, "InstanceInterface: Sleeping 'til its ready ...\n");
                std::this_thread::sleep_for (milliseconds (1000));
            }
        }
    }

    /**
     * enforce: Virtual method for enforcing I/O requests at the data plane stage.
     * This method receives the Context object (which specifies all I/O classifiers), the request's
     * content and size, and a Result object (which will contain the result after the enforcement).
     * @param context Context object containing all necessary metadata/classifiers to enforce the
     * I/O request.
     * @param buffer Content to be enforced.
     * @param size Size of the passed data content.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     */
    virtual void
    enforce (const Context& context, const void* buffer, const size_t& size, Result& result)
    {
        // marshal/unmarshal buffer content before/after being enforced
        bool enforced = false;

        while (!enforced) {
            // submit Context object and request's content to the data plane stage
            PStatus status = this->m_paio_stage->enforce_request (context, buffer, size, result);

            if (status.is_enforced ()) {
                enforced = true;
            } else {
                std::fprintf (stderr, "InstanceInterface: Sleeping 'til its ready ...\n");
                std::this_thread::sleep_for (milliseconds (1000));
            }
        }
    }

    /**
     * to_string: Generate a string with PAIO InstanceInterface values.
     * @return Returns the InstanceInterface's values in string-based format.
     */
    virtual std::string to_string ()
    {
        std::string message;
        message.append (std::to_string (this->m_default_workflow_id)).append (", ");
        message.append (this->m_default_secondary_workflow_identifier).append (", ");
        message.append (std::to_string (this->m_default_operation_type)).append (", ");
        message.append (std::to_string (this->m_default_operation_context));
        return message;
    }
};

} // namespace paio

#endif // PAIO_INSTANCE_INTERFACE_TMP_HPP
