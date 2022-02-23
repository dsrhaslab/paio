/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_KVS_INSTANCE_HPP
#define PAIO_KVS_INSTANCE_HPP

#include <paio/interface/instance_interface.hpp>

using namespace paio::statistics;

namespace paio {

/**
 * LsmKvsLayer class.
 * The LsmKvsLayer class implements the InstanceInterface methods, and is used for communication
 * between an I/O layer, in this case, an Log-structured merge-tree based key-value store, and a
 * PAIO data plane stage. This interface provides the means to establish the connection between
 * workflows and the PAIO internal enforcement mechanisms.
 * Contrarily to PaioInstance, it exposes the build_context_object method, that allow building
 * Context objects (which contain the necessary metadata/classifiers to differentiate and classify
 * a request, and several (SDS-enabled) LSM-based KVS calls.
 * Specifically, LSM-based KVS methods are provided with and without the Context object. The vanilla
 * version, i.e., without the Context object, it provides the same interface (which promotes
 * transparency) since the Context object is created inside the method. However, it does not allow
 * context propagation. The extended version on the other hand, i.e., with Context object, does not
 * follow the same interface (loss of transparency) as the Context object needs to be created
 * outside the LsmKvsLayer, but allows context propagation.
 * For an Application to use PAIO with the LSM key-value store backend, it only needs to replace the
 * original POSIX for the corresponding SDS-enabled one (considering respective vanilla and extended
 * versions).
 * TODO:
 *  - Implement the LSM KVS methods, with and without Context object.
 */
class LsmKvsLayer : public InstanceInterface {

private:
    bool m_has_io_transformation { option_default_has_io_transformation };
    std::mutex m_lock;

    /**
     * enforce: Method for enforcing I/O requests at the data plane stage.
     * This method receives the Context object (which specifies all I/O classifiers) and a Result
     * object (which will contain the result after the enforcement). Further, this method assumes
     * that a performance-oriented mechanism will be applied, and thus, the request's content (i.e.,
     * buffer) and size are set with nullptr and 0, respectively.
     * @param context Context object containing all necessary metadata/classifiers to enforce the
     * I/O request.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     */
    void enforce (const Context& context, Result& result) override;

    /**
     * enforce_request: Method for enforcing I/O requests at the data plane stage.
     * This method receives the Context object (which specifies all I/O classifiers), the request's
     * content and size, and a Result object (which will contain the result after the enforcement).
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

public:
    /**
     * LsmKvsLayer default constructor.
     * Initializes an InstanceInterface with default parameters.
     */
    LsmKvsLayer ();

    /**
     * LsmKvsLayer (explicit) parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object.
     * @param stage_ptr Shared pointer to a PaioStage object.
     */
    explicit LsmKvsLayer (std::shared_ptr<PaioStage> stage_ptr);

    /**
     * LsmKvsLayer parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object
     * and the default workflow identifier. The default operation type and context I/O classifiers
     * are initialized with default classifiers, namely KVS::no_op.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     * @param default_workflow_id Defines the workflow identifier of the LsmKvsLayer.
     */
    LsmKvsLayer (std::shared_ptr<PaioStage> stage_ptr, const long& default_workflow_id);

    /**
     * LsmKvsLayer parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object,
     * the default workflow identifier, and the default operation type and context I/O classifiers.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     * @param default_workflow_id Defines the workflow identifier of the LsmKvsLayer.
     * @param default_operation_type Defines the operation type of the LsmKvsLayer.
     * @param default_operation_context Defines the operation context of the LsmKvsLayer.
     */
    LsmKvsLayer (std::shared_ptr<PaioStage> stage_ptr,
        const long& default_workflow_id,
        const int& default_operation_type,
        const int& default_operation_context);

    /**
     * LsmKvsLayer default destructor.
     */
    ~LsmKvsLayer () override;

    /**
     * set_default_workflow_id: Set new value in the workflow identifier I/O classifier.
     * This method is thread-safe.
     * @param workflow_id New value to be set in m_default_workflow_id.
     */
    void set_default_workflow_id (const long& workflow_id) override;

    /**
     * set_default_operation_type: Set new value in the default operation type I/O classifier.
     * This method is thread-safe.
     * @param operation_type New value to be set in m_default_operation_type.
     */
    void set_default_operation_type (const int& operation_type) override;

    /**
     * set_default_operation_context: Set new value in the default operation context I/O classifier.
     * This method is thread-safe.
     * @param operation_context New value to be set in m_default_operation_context.
     */
    void set_default_operation_context (const int& operation_context) override;

    /**
     * set_default_secondary_workflow_identifier: Set new value in the workflow secondary I/O
     * classifier.
     * This method is thread-safe.
     * @param workflow_secondary_id New value to be set in m_default_secondary_workflow_identifier.
     */
    void set_default_secondary_workflow_identifier (
        const std::string& workflow_secondary_id) override;

    /**
     * set_io_transformation: Define if I/O transformations will be used to handle the I/O requests
     * accordingly. For example, considering an enforcement object that performs encryption: in
     * write-based requests, the transformation should be done before submitting the request to the
     * file system (encrypt (content) -> put (key, content')); in read-based requests, the operation
     * should be done first and then apply the respective transformation
     * (get (key) -> decrypt (content)).
     * This method is thread-safe.
     * @param value New value to be set in the m_has_io_transformation.
     */
    void set_io_transformation (const bool& value);

    /**
     * build_context_object: Method for building Context objects for the differentiation and
     * classification of application requests at the PAIO data plane stage.
     * The resulting context object will apply the default I/O classifiers, namely
     * m_default_workflow_id, m_default_operation_type, and m_default_operation_context.
     * @return Returns the respective Context object (following a RVO mechanism).
     */
    Context build_context_object () override;

    /**
     * build_context_object: Method for building Context objects for the differentiation and
     * classification of application requests at the Paio data plane stage.
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
     * put:
     * @param key
     * @param value
     * @return
     */
    PStatus put (const void* key, const void* value);

    /**
     * get:
     * @param key
     * @param result
     * @return
     */
    PStatus get (const void* key, Result& result);

    /**
     * delete_:
     * @param key
     * @return
     */
    PStatus delete_ (const void* key);

    /**
     * to_string: Generate a string with the LsmKvsLayer interface values.
     * @return Returns the LsmKvsLayer's values in string-based format.
     */
    std::string to_string () override;
};

} // namespace paio

#endif // PAIO_KVS_INSTANCE_HPP
