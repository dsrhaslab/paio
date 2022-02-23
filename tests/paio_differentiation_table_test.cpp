/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/rules/differentiation_rule.hpp>
#include <paio/rules/differentiation_table.hpp>
#include <paio/utils/status.hpp>

namespace paio::rules {

/**
 * TODO:
 *  - add tests get_differentiation_table_begin_iterator and get_differentiation_table_end_iterator
 *  (DifferentiationTable);
 */
class DifferentiationRuleTableTest {

private:
    FILE* m_fd { stdout };

public:
    /**
     * DifferentiationRuleTableTest default constructor.
     */
    DifferentiationRuleTableTest () = default;

    /**
     * DifferentiationRuleTableTest parameterized constructor.
     * @param fd Pointer to file to where results should be written to.
     */
    explicit DifferentiationRuleTableTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * DifferentiationRuleTableTest default destructor.
     */
    ~DifferentiationRuleTableTest () = default;

    /**
     * test_insert_differentiation_rule: Insert DifferentiationRules in the table, using the
     * parameterized constructor.
     * @param table Pointer to a DifferentiationTable object.
     * @param total_rules Total of rules to be generated and inserted in the table.
     * @param random_rule_id Boolean that defines if the rule identifier should be randomly
     * generated.
     * @param classifiers_range Range of I/O classifiers (workflow-id, operation type, operation
     * context) to use
     * @param log Boolean that defines if the differentiation table content should be printed to the
     * log file.
     */
    void test_insert_differentiation_rule (DifferentiationTable* table,
        const int& total_rules,
        const bool& random_rule_id,
        const int& classifiers_range,
        const bool& log)
    {
        std::fprintf (this->m_fd, "Test insert DifferentiationRule in DifferentiationTable ...\n");

        for (int i = 1; i <= total_rules; i++) {
            // create initial configurations
            auto rule_id = static_cast<uint64_t> (i);
            DifferentiationRuleType rule_type { DifferentiationRuleType::none };
            long channel_id = static_cast<long> (i);
            long enforcement_object_id = -1;
            auto workflow_id = static_cast<uint32_t> (random () % classifiers_range);
            auto operation_type = static_cast<uint32_t> (random () % classifiers_range);
            auto operation_context = static_cast<uint32_t> (random () % classifiers_range);

            if (random () % 2) {
                // channel differentiation type
                rule_type = DifferentiationRuleType::channel_differentiation;
            } else {
                rule_type = DifferentiationRuleType::enforcement_object_differentiation;
                enforcement_object_id = static_cast<long> (i);
            }

            // generate random rule id
            if (random_rule_id) {
                rule_id = random () % total_rules;
            }
            // insert differentiation rule in table
            table->insert_differentiation_rule (rule_id,
                rule_type,
                channel_id,
                enforcement_object_id,
                workflow_id,
                operation_type,
                operation_context);
        }

        if (log) {
            std::fprintf (this->m_fd, "%s\n", table->to_string ().c_str ());
            std::fflush (this->m_fd);
        }
    }

    /**
     * test_insert_differentiation_rule_copy: Insert DifferentiationRules in the table, using the
     * copy constructor.
     * @param table Pointer to a DifferentiationTable object.
     * @param total_rules Total of rules to be generated and inserted in the table.
     * @param random_rule_id Boolean that defines if the rule identifier should be randomly
     * generated.
     * @param classifiers_range Range of I/O classifiers (workflow-id, operation type, operation
     * context) to use
     * @param log Boolean that defines if the differentiation table content should be printed to the
     * log file.
     */
    void test_insert_differentiation_rule_copy (DifferentiationTable* table,
        const int& total_rules,
        const bool& random_rule_id,
        const int& classifiers_range,
        const bool& log)
    {
        std::fprintf (this->m_fd,
            "Test insert DifferentiationRule (copy) in DifferentiationTable ...\n");

        for (int i = 1; i <= total_rules; i++) {
            // create initial configurations
            auto rule_id = static_cast<uint64_t> (i);
            DifferentiationRuleType rule_type { DifferentiationRuleType::none };
            long channel_id = static_cast<long> (i);
            long enforcement_object_id = -1;
            auto workflow_id = static_cast<uint32_t> (random () % classifiers_range);
            auto operation_type = static_cast<uint32_t> (random () % classifiers_range);
            auto operation_context = static_cast<uint32_t> (random () % classifiers_range);

            if (random () % 2) {
                // channel differentiation type
                rule_type = DifferentiationRuleType::channel_differentiation;
            } else {
                rule_type = DifferentiationRuleType::enforcement_object_differentiation;
                enforcement_object_id = static_cast<long> (i);
            }

            // generate random rule id
            if (random_rule_id) {
                rule_id = random () % total_rules;
            }

            // create DifferentiationRule object
            DifferentiationRule rule { rule_id,
                rule_type,
                channel_id,
                enforcement_object_id,
                workflow_id,
                operation_type,
                operation_context };

            std::fprintf (this->m_fd, "Original: %s\n", rule.to_string ().c_str ());

            // insert differentiation rule in table
            table->insert_differentiation_rule (rule);

            // validate if rule was successfully created
            DifferentiationRule temporary_rule;
            table->select_differentiation_rule (rule_id, temporary_rule);
            std::fprintf (this->m_fd, "Copy: %s\n\n", temporary_rule.to_string ().c_str ());
        }

        if (log) {
            std::fprintf (this->m_fd, "%s\n", table->to_string ().c_str ());
            std::fflush (this->m_fd);
        }
    }

    /**
     * test_select_differentiation_rule: Select/pick differentiation rule from table.
     * @param table Pointer to a DifferentiationTable object.
     * @param iterations Number of iterations to perform the select_differentiation_rule operation.
     * @param log Boolean that defines if the differentiation table content should be printed to the
     * log file.
     */
    void test_select_differentiation_rule (DifferentiationTable* table,
        const int& iterations,
        const bool& log)
    {
        std::fprintf (this->m_fd, "Test select differentiation rule\n");

        // get size of the differentiation rules table
        int table_size = table->get_differentiation_table_size ();

        for (int i = 0; i < iterations; i++) {
            // generate random rule identifier
            auto rule_id = static_cast<uint64_t> (random () % table_size);
            DifferentiationRule rule;

            // select differentiation rule
            PStatus status = table->select_differentiation_rule (rule_id, rule);

            if (log) {
                if (status.is_ok ()) {
                    std::fprintf (this->m_fd, "Iteration %d : %s\n", i, rule.to_string ().c_str ());
                }
            }
        }
    }

    /**
     * test_remove_differentiation_rule: Remove DifferentiationRule from the table.
     * @param table Pointer to a DifferentiationTable object.
     * @param iterations Number of iterations to perform the remove_differentiation_rule operation.
     * @param log Boolean that defines if the differentiation table content should be printed to the
     * log file.
     */
    void test_remove_differentiation_rule (DifferentiationTable* table,
        const int& iterations,
        const bool& log)
    {
        std::fprintf (this->m_fd, "Test remove differentiation rule\n");

        // get size of the differentiation table
        int table_size = table->get_differentiation_table_size ();
        int total_rules_removed = 0;

        // remove differentiation rules
        for (int i = 0; i < iterations; i++) {
            uint64_t rule_id = random () % table_size;

            // remove differentiation rule and verify its return value
            if (table->remove_differentiation_rule (rule_id).is_ok ()) {
                total_rules_removed++;
            }
        }

        if (log) {
            std::fprintf (this->m_fd, "%s\n", table->to_string ().c_str ());
        }

        std::fprintf (this->m_fd,
            "Result: %d == %d\n",
            total_rules_removed,
            (table_size - table->get_differentiation_table_size ()));
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

    DifferentiationRuleTableTest test { fd };
    DifferentiationTable table {};

    // insert differentiation rules
    test.test_insert_differentiation_rule (&table, 100, true, 10, log);
    // test.test_insert_differentiation_rule_copy (&table, 100, true, 10, log);

    test.test_select_differentiation_rule (&table, 100, log);
    test.test_remove_differentiation_rule (&table, 100, log);

    // close file descriptor
    if (fd != stdout && fd != nullptr) {
        std::fclose (fd);
    }
}
