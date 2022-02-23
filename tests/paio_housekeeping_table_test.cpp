/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/rules/housekeeping_table.hpp>

namespace paio::rules {

/**
 * TODO:
 *  - add tests get_property_at_index and get_property_at_range (HousekeepingRule);
 */
class HousekeepingRuleTableTest {

private:
    FILE* m_fd { stdout };

public:
    /**
     * HousekeepingRuleTableTest default constructor.
     */
    HousekeepingRuleTableTest () = default;

    /**
     * HousekeepingRuleTableTest parameterized constructor.
     * @param fd Pointer to file to where results should be written to.
     */
    explicit HousekeepingRuleTableTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * HousekeepingRuleTableTest default destructor.
     */
    ~HousekeepingRuleTableTest () = default;

    /**
     * test_insert_housekeeping_rule: Insert HousekeepingRules in the table, using the
     * parameterized constructor.
     * @param table Pointer to an HousekeepingTable object.
     * @param total_rules Number of rules to be created/inserted.
     * @param random_rule_id Boolean that defines if the rule identifier should be randomly
     * generated or not.
     * @param log Boolean that defines if housekeeping table content should be printed to stdout.
     */
    void test_insert_housekeeping_rule (HousekeepingTable* table,
        const int& total_rules,
        const bool& random_rule_id,
        const bool& log)
    {
        std::fprintf (this->m_fd, "Test insert HousekeepingRule in HousekeepingTable ...\n");

        for (int i = 1; i <= total_rules; i++) {
            // create initial configurations
            auto rule_id = static_cast<uint64_t> (i);
            HousekeepingOperation operation_type { HousekeepingOperation::no_op };
            long channel_id = static_cast<long> (i);
            long enforcement_object_id = -1;
            std::vector<long> properties {};

            // generate random operation type
            int operation = static_cast<int> (random ());
            if (operation % 2) {
                operation_type = HousekeepingOperation::create_channel;
            } else {
                operation_type = HousekeepingOperation::create_object;
                enforcement_object_id = random () % 10; // small set of enforcement objects
                properties.push_back ((random () % 3) + 1); // random configuration setting
            }

            // generate a random rule id
            if (random_rule_id) {
                rule_id = random () % total_rules;
            }

            // insert housekeeping rule in table
            table->insert_housekeeping_rule (rule_id,
                operation_type,
                channel_id,
                enforcement_object_id,
                properties);
        }

        if (log) {
            std::fprintf (this->m_fd, "%s\n", table->to_string ().c_str ());
            std::fflush (this->m_fd);
        }
    }

    /**
     * test_insert_housekeeping_rule: Insert HousekeepingRules in the table, using the copy
     * constructor.
     * @param table Pointer to an HousekeepingTable object.
     * @param total_rules Number of rules to be created/inserted.
     */
    void test_insert_housekeeping_rule_copy (HousekeepingTable* table, const int& total_rules)
    {
        for (int i = 1; i <= total_rules; i++) {
            // create initial configurations
            auto rule_id = static_cast<uint64_t> (i);
            HousekeepingOperation operation_type { HousekeepingOperation::no_op };
            long channel_id = static_cast<long> (i);
            long enforcement_object_id = -1;
            std::vector<long> properties {};

            // generate random operation type
            int operation = static_cast<int> (random ());
            if (operation % 2) {
                operation_type = HousekeepingOperation::create_channel;
            } else {
                operation_type = HousekeepingOperation::create_object;
                enforcement_object_id = random () % 10; // small set of enforcement objects
                properties.push_back ((random () % 3) + 1); // random configuration setting
            }

            // create HousekeepingRule object
            HousekeepingRule rule { rule_id,
                operation_type,
                channel_id,
                enforcement_object_id,
                properties };

            std::fprintf (this->m_fd, "%s\n", rule.to_string ().c_str ());

            // insert housekeeping rule using copy constructor
            table->insert_housekeeping_rule (rule);

            // validate if rule was successfully created
            HousekeepingRule temporary {};
            table->select_housekeeping_rule (rule_id, temporary);
            std::fprintf (this->m_fd, "%s\n\n", temporary.to_string ().c_str ());
        }
    }

    /**
     * test_select_housekeeping_rule: Select/pick housekeeping rule from the table.
     * @param table Pointer to an HousekeepingTable object.
     * @param iterations Number of iterations to perform the select_housekeeping_rule operation.
     * @param log Boolean that defines if housekeeping table content should be printed to stdout.
     */
    void
    test_select_housekeeping_rule (HousekeepingTable* table, const int& iterations, const bool& log)
    {
        std::fprintf (this->m_fd, "Test select housekeeping rule\n");

        // get size of the housekeeping rules table
        int table_size = table->get_housekeeping_table_size ();

        for (int i = 0; i < iterations; i++) {
            // generate random rule identifier
            uint64_t rule_id = random () % table_size;
            HousekeepingRule rule {};

            // select housekeeping rule
            PStatus status = table->select_housekeeping_rule (rule_id, rule);

            if (log) {
                if (status.is_ok ()) {
                    std::fprintf (this->m_fd, "Iteration %d : %s\n", i, rule.to_string ().c_str ());
                    std::fflush (this->m_fd);
                }
            }
        }
    }

    /**
     * test_employ_housekeeping_rule: Set HousekeepingRule to enforced.
     * @param table Pointer to an HousekeepingTable object.
     * @param iterations Number of iterations to perform the employ_housekeeping_rule operation.
     * @param log Boolean that defines if housekeeping table content should be printed to stdout.
     */
    void
    test_employ_housekeeping_rule (HousekeepingTable* table, const int& iterations, const bool& log)
    {
        std::fprintf (this->m_fd, "Test employ housekeeping rule\n");

        // get size of the housekeeping rules table
        int table_size = table->get_housekeeping_table_size ();

        int total_rules_employed = 0;

        // employ housekeeping rule
        for (int i = 0; i < iterations; i++) {
            uint64_t rule_id = random () % table_size;

            // mar housekeeping rule as enforced and verify the return status
            if ((table->mark_housekeeping_rule_as_enforced (rule_id)).is_ok ()) {
                total_rules_employed++;
            }

            if (log) {
                // select housekeeping rule
                HousekeepingRule rule {};
                table->select_housekeeping_rule (rule_id, rule);
                std::fprintf (this->m_fd, "Iteration %d : %s\n", i, rule.to_string ().c_str ());
            }
        }

        std::fprintf (this->m_fd,
            "Result: %d == %d\n",
            total_rules_employed,
            (table->get_housekeeping_table_size () - table->get_total_of_rules_left_to_employ ()));
    }

    /**
     * test_remove_housekeeping_rule: Remove HousekeepingRule from the table.
     * @param table Pointer to an HousekeepingTable object.
     * @param iterations Number of iterations to perform the remove_housekeeping_rule operation.
     * @param log Boolean that defines if housekeeping table content should be printed to stdout.
     */
    void
    test_remove_housekeeping_rule (HousekeepingTable* table, const int& iterations, const bool& log)
    {
        std::fprintf (this->m_fd, "Test remove housekeeping rule\n");

        // get size of the housekeeping rules table
        int table_size = table->get_housekeeping_table_size ();
        int total_rules_removed = 0;

        // remove housekeeping rule
        for (int i = 0; i < iterations; i++) {
            uint64_t rule_id = random () % table_size;

            // remove housekeeping rule and verify the return status
            if ((table->remove_housekeeping_rule (rule_id)).is_ok ()) {
                total_rules_removed++;
            }
        }

        if (log) {
            std::fprintf (this->m_fd, "%s\n", table->to_string ().c_str ());
        }

        std::fprintf (this->m_fd,
            "Result: %d == %d\n",
            total_rules_removed,
            (table_size - table->get_housekeeping_table_size ()));
    }
};

} // namespace paio::rules

using namespace paio::rules;

int main (int argc, char** argv)
{
    // check argv for the file to be placed the result
    FILE* fd = stdout;
    bool log = true;

    // open file to write the logging results
    if (argc > 1) {
        fd = ::fopen (argv[1], "w");

        if (fd == nullptr) {
            fd = stdout;
        }
    }

    HousekeepingRuleTableTest test { fd };
    HousekeepingTable table {};

    // insert housekeeping rules
    test.test_insert_housekeeping_rule (&table, 100, false, log);
    // test.test_insert_housekeeping_rule_copy (&table, 10);

    test.test_select_housekeeping_rule (&table, 100, log);
    test.test_employ_housekeeping_rule (&table, 10, log);
    test.test_remove_housekeeping_rule (&table, 50, log);

    // close file descriptor
    if (fd != stdout && fd != nullptr) {
        std::fclose (fd);
    }
}
