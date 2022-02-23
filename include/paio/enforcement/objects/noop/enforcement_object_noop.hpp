/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_ENFORCEMENT_OBJECT_NOOP_HPP
#define PAIO_ENFORCEMENT_OBJECT_NOOP_HPP

#include <paio/enforcement/objects/enforcement_object.hpp>
#include <paio/utils/logging.hpp>

namespace paio::enforcement {

/**
 * NoopObject class. Inherited from EnforcementObject class.
 * The Noop enforcement object is a bypass storage service that does not perform any operation or
 * service over I/O flows.
 * On enforce, if the Ticket object contains the data/metadata, Noop will copy it to the Result
 * object, which will be forwarded to the next I/O layer.
 * The Noop object is NOT thread-safe.
 *  TODO:
 *   - change structs to specific/dedicated objects in obj_collect_statistics method;
 *   - create a dedicated object for specifying each configuration or set of configurations in
 *   obj_configure method;
 */
class NoopObject : public EnforcementObject {

private:
    long m_object_id { 0 };
    std::string m_tag_name {};
    uint32_t m_counter { 0 };
    std::mutex m_lock;
    const bool m_shared { false };

    /**
     * increment_counter: Increments the counter object (m_counter) considering if the object is
     * shared or not. In case of shared, the counter is incremented while protected by a mutex.
     */
    void increment_counter ();

public:
    /**
     * NoopObject default constructor.
     */
    NoopObject ();

    /**
     * NoopObject parameterized constructor.
     * @param object_id EnforcementObject identifier.
     */
    explicit NoopObject (const long& object_id);

    /**
     * NoopObject parameterized constructor.
     * @param tag_name Name of the noop object.
     */
    NoopObject (const long& object_id, const std::string& tag_name, const bool& is_shared);

    /**
     * NoopObject default destructor.
     */
    ~NoopObject () override;

    /**
     * get_enforcement_object_id: Get the EnforcementObject's identifier.
     * @return Returns a copy of the m_object_id value.
     */
    [[nodiscard]] long get_enforcement_object_id () const override;

    /**
     * obj_enforce: apply the Noop enforcement mechanisms over requests.
     * In this case, it bypasses any enforcement properties for the request.
     * @param ticket Represents a metadata-like object that contains a set of elements that
     * characterize the request, including the request context, size, and buffer.
     * @param result Reference to a Result object that stores the result of Noop enforcement
     * mechanism, including the request content if enabled.
     */
    void obj_enforce (const Ticket& ticket, Result& result) override;

    /**
     * obj_configure: configure internal properties of the Noop enforcement object.
     * In this case, since no operation is performed in this enforcement object, it immediately
     * returns PStatus::OK.
     * @param config Configuration property to be set/updated.
     * @param configuration_values Vector of configuration values to be updated.
     * @return PStatus state, being PStatus::OK if the configuration was successfully made, and
     * PStatus::ERROR otherwise.
     */
    PStatus obj_configure (int config, const std::vector<long>& configuration_values) override;

    /**
     * obj_collect_statistics: collect and submit statistics from the Noop enforcement object to
     * the SDS control plane (controller).
     * @return PStatus state, being PStatus::OK if statistics were effectively collected, and
     * PStatus::Error otherwise.
     */
    PStatus obj_collect_statistics (ObjectStatisticsRaw& statistics_raw) override;

    /**
     * to_string: generate a string-based format of the contents of the Noop enforcement object.
     * @return String of the Noop settings.
     */
    std::string to_string () override;
};
} // namespace paio::enforcement

#endif // PAIO_ENFORCEMENT_OBJECT_NOOP_HPP
