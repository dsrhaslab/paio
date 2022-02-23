/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_ENFORCEMENT_OBJECT_HPP
#define PAIO_ENFORCEMENT_OBJECT_HPP

#include <iostream>
#include <paio/core/interface_definitions.hpp>
#include <paio/enforcement/result.hpp>
#include <paio/enforcement/ticket.hpp>
#include <paio/utils/status.hpp>
#include <vector>

namespace paio::enforcement {

/**
 * EnforcementObject abstract class.
 * An enforcement object is a self-contained, single-purposed mechanism that contains custom I/O
 * logic to apply over requests. Examples of such mechanisms can range from performance control and
 * resource management such as token-buckets, I/O schedulers, and caches; data transformations as
 * compression and encryption; to data management (e.g., data prefetching, placement).
 * This abstraction provides to system designers the necessary flexibility and extensibility to
 * develop new I/O mechanisms tailored for enforcing specific storage policies over requests.
 * It exposes three main calls that allow building enforcement objects:
 *  - obj_configure provides the tuning knobs to update the enforcement object's internals with a
 *  new state. It allows the control plane to dynamically programme and adapt the enforcement object
 *  to respond to workload variations and new policies.
 *  - obj_enforce implements the actual I/O logic to apply over requests. It returns a result object
 *  that contains the updated request after applying its logic. It also receives the ticket object
 *  that can be used to employ different actions over the request.
 *   - obj_collect_statistics allows the control plane to collect I/O statistics of the enforcement
 *   object.
 *  TODO:
 *   - change structs to specific/dedicated objects in obj_collect_statistics method;
 *   - create a dedicated object for specifying each configuration or set of configurations in
 *   obj_configure method;
 */
class EnforcementObject {

public:
    /**
     * EnforcementObject default constructor.
     */
    EnforcementObject () = default;

    /**
     * EnforcementObject default destructor.
     */
    virtual ~EnforcementObject () = default;

    /**
     * get_enforcement_object_id: get the EnforcementObject identifier.
     * @return  Returns a copy of the object identifier.
     */
    [[nodiscard]] virtual long get_enforcement_object_id () const = 0;

    /**
     * obj_enforce: implements the actual I/O logic to apply over requests.
     * @param ticket Represents a metadata-like object that contains a set of elements that
     * characterize the request, including the request context, size, and buffer.
     * @param result Reference to a Result object to return the result of the enforcement mechanism
     * as well as the updated request content after applying its logic.
     */
    virtual void obj_enforce (const Ticket& ticket, Result& result) = 0;

    /**
     * obj_configure: configure internal properties of the enforcement object.
     * @param config Configuration property to be set/updated.
     * @param configuration_values Vector of configuration values to be updated.
     * @return PStatus state, being PStatus::OK if the configuration was successfully made, and
     * PStatus::ERROR otherwise.
     */
    virtual PStatus obj_configure (int configuration, const std::vector<long>& configuration_values)
        = 0;

    /**
     * obj_collect_statistics: collect and submit statistics from the enforcement object to
     * the SDS control plane (controller).
     * @return PStatus state, being PStatus::OK if statistics were effectively collected, and
     * PStatus::Error otherwise.
     */
    virtual PStatus obj_collect_statistics (ObjectStatisticsRaw& statistics_raw) = 0;

    /**
     * to_string: generate a string-based format of the contents of the enforcement object.
     * @return String of the Enforcement object settings.
     */
    virtual std::string to_string () = 0;
};
} // namespace paio::enforcement

#endif // PAIO_ENFORCEMENT_OBJECT_HPP
