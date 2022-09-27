/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_PAIO_INSTANCE_HPP
#define PAIO_PAIO_INSTANCE_HPP

#include <paio/interface/instance_interface.hpp>

namespace paio {

/**
 * PaioInstance class.
 * The PaioInstance class implements the InstanceInterface methods, and is used for communication
 * between an I/O layer and a PAIO data plane stage. This interfaces provide the means to establish
 * the connection between workflows and the PAIO internal enforcement mechanisms.
 * It exposes two main methods:
 *  - build_context_object: that allow building Context objects (which contain the necessary
 * metadata/classifiers to differentiate and classify a request);
 *  - enforce_request: that allows enforcing specific storage mechanisms over (application) I/O
 *  requests.
 * For an Application to use PAIO, without considering any storage backend (e.g., POSIX file system,
 * key-value store, user-level block device), it should use this interface. First, it should build a
 * Context object (with build_context_object), and then invoke enforce_request to apply the
 * respective PAIO storage mechanisms over the Application's requests.
 * TODO:
 *  - fix enforce methods in the case of !Status::Enforced;
 *  - create testing class.
 */
class PaioInstance : public InstanceInterface {

private:
    std::mutex m_lock;

public:
    /**
     * PaioInstance default constructor.
     * Initializes an InstanceInterface with default parameters.
     */
    PaioInstance ();

    /**
     * PaioInstance (explicit) parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     */
    explicit PaioInstance (std::shared_ptr<PaioStage> stage_ptr);

    /**
     * PaioInstance parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object
     * and the default workflow identifier. The default operation type and context are initialized
     * with default classifiers, namely PAIO_GENERAL::no_op.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     * @param default_workflow_id Defines the workflow identifier of the PaioInstance.
     */
    PaioInstance (std::shared_ptr<PaioStage> stage_ptr, const long& default_workflow_id);

    /**
     * PaioInstance (fully) parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object,
     * the default workflow identifier, and the default operation type and context.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     * @param default_workflow_id Defines the workflow identifier of the PaioInstance.
     * @param default_operation_type Defines the operation type of the PaioInstance.
     * @param default_operation_context Defines the operation context of the PaioInstance.
     */
    PaioInstance (std::shared_ptr<PaioStage> stage_ptr,
        const long& default_workflow_id,
        const int& default_operation_type,
        const int& default_operation_context);

    /**
     * PaioInstance default destructor.
     */
    ~PaioInstance () override;

    /**
     * set_default_workflow_id: Set new value in the workflow identifier parameter.
     * This method is thread-safe.
     * @param workflow_id New value to be set in m_default_workflow_id.
     */
    void set_default_workflow_id (const long& workflow_id) override;

    /**
     * set_default_operation_type: Set new value in the default operation type parameter.
     * This method is thread-safe.
     * @param operation_type New value to be set in m_default_operation_type.
     */
    void set_default_operation_type (const int& operation_type) override;

    /**
     * set_default_operation_context: Set new value in the default operation context parameter.
     * This method is thread-safe.
     * @param operation_context New value to be set in m_default_operation_context.
     */
    void set_default_operation_context (const int& operation_context) override;

    /**
     * set_default_secondary_workflow_identifier: Set new value in the workflow secondary identifier
     * parameter.
     * This method is thread-safe.
     * @param workflow_secondary_id New value to be set in m_default_secondary_workflow_identifier.
     */
    void set_default_secondary_workflow_identifier (
        const std::string& workflow_secondary_id) override;

    /**
     * get_default_workflow_id: Get the value of the workflow identifier parameter.
     * This method is thread-safe.
     * @return Returns a copy of the m_default_workflow_id parameter.
     */
    [[nodiscard]] long get_default_workflow_id ();

    /**
     * get_default_secondary_workflow_identifier: Get the value of the secondary workflow identifier
     * parameter.
     * This method is thread-safe.
     * @return Returns a copy of the m_default_secondary_workflow_identifier parameter.
     */
    [[nodiscard]] std::string get_default_secondary_workflow_identifier ();

    /**
     * get_default_operation_type: Get the value of the operation type parameter.
     * This method is thread-safe.
     * @return Returns a copy of the m_default_operation_type parameter.
     */
    [[nodiscard]] int get_default_operation_type ();

    /**
     * get_default_operation_context: Get the value of the operation context parameter.
     * This method is thread-safe.
     * @return Returns a copy of the m_default_operation_context parameter.
     */
    [[nodiscard]] int get_default_operation_context ();

    /**
     * build_context_object: Method for building Context objects for the differentiation and
     * classification of application requests at the Paio data plane stage.
     * This method is thread-safe.
     * @return Returns the respective Context object (following an RVO mechanism).
     */
    Context build_context_object () override;

    /**
     * build_context_object: Method for building Context objects for the differentiation and
     * classification of application requests at the Paio data plane stage.
     * This method is lock-free.
     * @param workflow_id Defines the workflow identifier used to submit the request (from the
     * application to the data plane stage).
     * @param operation_type Defines the type of the submitted operation (from the application to
     * the data plane stage).
     * @param operation_context Defines the context of the submitted operation (from the application
     * to the data plane stage).
     * @param operation_size Defines the size of the operation to be enforced.
     * @param total_operations Defines the total number of operations that will be enforced.
     * @return Returns the respective Context object (following an RVO mechanism).
     */
    Context build_context_object (const long& workflow_id,
        const int& operation_type,
        const int& operation_context,
        const uint64_t& operation_size,
        const int& total_operations) override;

    /**
     * enforce: Method for enforcing I/O requests at the data plane stage.
     * This method receives the Context object (which specifies all I/O classifiers) and a Result
     * object (which will contain the result after the enforcement). Further, this method assumes
     * that a performance-oriented mechanism will be applied, and thus, the request's content (i.e.,
     * buffer) and size are set with nullptr and 0, respectively.
     * This method is lock-free.
     * @param context Context object containing all necessary metadata/classifiers to enforce the
     * I/O request.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     */
    void enforce (const Context& context, Result& result) override;

    /**
     * enforce: Method for enforcing I/O requests at the data plane stage.
     * This method receives the Context object (which specifies all I/O classifiers), the request's
     * content and size, and a Result object (which will contain the result after the enforcement).
     * This method is lock-free.
     * @param context Context object containing all necessary metadata/classifiers to enforce the
     * I/O request.
     * @param buffer Content to be enforced.
     * @param size Size of the passed data content.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     */
    void enforce (const Context& context,
        const void* buffer,
        const size_t& size,
        Result& result) override;

    /**
     * to_string: Generate a string with the PaioInstance interface values.
     * @return Returns the PaioInstance's values in string-based format.
     */
    std::string to_string () override;
};
} // namespace paio

#endif // PAIO_PAIO_INSTANCE_HPP
