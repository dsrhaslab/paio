/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/core/stage_info.hpp>
#include <string>

namespace paio::core {

enum class StageInfoConstructorsTest { empty = 0, single = 1, copy = 2 };

/**
 * StageInfoTest class.
 * This class tests the usage and functionality of the StageInfo object.
 * Currently, it tests the StageInfo constructors, the setting of environment variables, the
 * serialization, and the setting of StageInfo's description.
 */
class StageInfoTest {

private:
    FILE* m_fd { stdout };

    /**
     * set_env: Set environment variable with name 'env_name' and value 'env_value'.
     * @param env_name Name of the environment variable.
     * @param env_value Value to be set in the environment variable.
     * @return Returns true if the environment variable was successfully set; false otherwise.
     */
    bool set_env (const std::string& env_name, const std::string& env_value)
    {
        auto return_value = ::setenv (env_name.c_str (), env_value.c_str (), 1);
        return (return_value == 0);
    }

    /**
     * unset_env: Remove environment variable with name 'env_name'.
     * @param env_name Name of the environment variable to remove.
     * @return Returns true if the environment variable was successfully unset; false otherwise.
     */
    bool unset_env (const std::string& env_name)
    {
        auto return_value = ::unsetenv (env_name.c_str ());
        return (return_value == 0);
    }

public:
    /**
     * StageInfoTest default constructor.
     */
    StageInfoTest () = default;

    /**
     * StageInfoTest parameterized constructor.
     * @param fd Pointer to file through where results should be written to.
     */
    explicit StageInfoTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * StageInfoTest default destructor.
     */
    ~StageInfoTest () = default;

    /**
     * test_constructors: test the constructors of the StageInfo class.
     * @param constructor_type Defines the type of constructor to be used.
     * @param name Name (m_name) of the data plane stage. Used in the explicit and parameterized
     * constructors.
     * @param description Description (m_description) of the data plane stage. Used in the
     * parameterized constructor.
     */
    void test_constructors (const StageInfoConstructorsTest& constructor_type,
        const std::string& name)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Test StageInfo constructors ");

        switch (constructor_type) {
            { // explicit constructor (name)
                case StageInfoConstructorsTest::single:
                    std::fprintf (this->m_fd, "(explicit)\n");
                    std::fprintf (this->m_fd, "----------------------------\n");

                    StageInfo info { name };
                    std::fprintf (this->m_fd, "%s\n", info.to_string ().c_str ());
                    break;
            }
            { // copy constructor
                case StageInfoConstructorsTest::copy:
                    std::fprintf (this->m_fd, "(copy)\n");
                    std::fprintf (this->m_fd, "----------------------------\n");

                    // create a StageInfo object and set a description
                    StageInfo info_original { name };
                    info_original.set_description ("This is a test description.");

                    // create a copy of the StageInfo object
                    StageInfo info_copy { info_original };
                    std::fprintf (this->m_fd,
                        "Original::%s\n",
                        info_original.to_string ().c_str ());
                    std::fprintf (this->m_fd, "Copy::%s\n", info_copy.to_string ().c_str ());
                    break;
            }
            {
                case StageInfoConstructorsTest::empty:
                default:
                    std::fprintf (this->m_fd, "(default)\n");
                    std::fprintf (this->m_fd, "----------------------------\n");

                    StageInfo info {};
                    std::fprintf (this->m_fd, "%s\n", info.to_string ().c_str ());
                    break;
            }
        }

        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * test_set_environment: test if the environment variable is being set and available at the
     * StageInfo object. This method first creates a StageInfo object without registering any
     * environment variable, to demonstrate it is empty. Then, it sets the environment variable and
     * creates another StageInfo object, that now contains the environment variable value.
     * @param env_name Name of the environment variable to be set.
     * @param env_value Value to be set in the environment variable.
     */
    void test_set_environment (const std::string& env_name, const std::string& env_value)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd,
            "Test StageInfo set environment-variable (%s, %s)\n",
            env_name.c_str (),
            env_value.c_str ());
        std::fprintf (this->m_fd, "----------------------------\n");

        // create StageInfo without environment variable
        StageInfo info_env_less {};
        std::fprintf (this->m_fd, "%s\n", info_env_less.to_string ().c_str ());

        // set environment variable
        auto return_value = this->set_env (env_name, env_value);
        // create StageInfo after setting environment variable
        StageInfo info_env {};
        std::fprintf (this->m_fd, "%s\n", info_env.to_string ().c_str ());

        // unset environment variable
        if (return_value) {
            this->unset_env (env_name);
        }

        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * test_serialize: test the serialization of StageInfo object.
     * @param info Reference to the StageInfo object to be serialized.
     */
    void test_serialize (StageInfo& info)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Test StageInfo serialization\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        StageInfoRaw h_obj {};
        info.serialize (h_obj);

        std::string message { "StageInfoRaw {\n" };
        message.append ("   name\t\t: ").append (h_obj.m_stage_name).append (" (");
        message.append (std::to_string (sizeof (h_obj.m_stage_name))).append (")\n");

        message.append ("   env\t\t: ").append (h_obj.m_stage_env).append (" (");
        message.append (std::to_string (sizeof (h_obj.m_stage_env))).append (")\n");

        message.append ("   pid\t\t: ").append (std::to_string (h_obj.m_pid)).append (" (");
        message.append (std::to_string (sizeof (h_obj.m_pid))).append (")\n");

        message.append ("   ppid\t\t: ").append (std::to_string (h_obj.m_ppid)).append (" (");
        message.append (std::to_string (sizeof (h_obj.m_ppid))).append (")\n");

        message.append ("   hostname\t: ").append (h_obj.m_stage_hostname).append (" (");
        message.append (std::to_string (sizeof (h_obj.m_stage_hostname))).append (")\n");

        message.append ("   login_name\t: ").append (h_obj.m_stage_login_name).append (" (");
        message.append (std::to_string (sizeof (h_obj.m_stage_login_name))).append (")\n}\n");

        std::fprintf (this->m_fd, "%s", message.c_str ());
        std::fprintf (this->m_fd, "----------------------------\n\n");
    }

    /**
     * test_set_description: set a new description value in a given StageInfo object.
     * @param info Reference to the StageInfo object to be used.
     * @param new_description New description value to be set.
     */
    void test_set_description (StageInfo& info, const std::string& new_description)
    {
        // printer header
        std::fprintf (this->m_fd, "----------------------------\n");
        std::fprintf (this->m_fd, "Test StageInfo set-description\n");
        std::fprintf (this->m_fd, "----------------------------\n");

        std::fprintf (this->m_fd, "%s\n", info.to_string ().c_str ());

        info.set_description (new_description);
        std::fprintf (this->m_fd, "%s\n", info.to_string ().c_str ());
        std::fprintf (this->m_fd, "----------------------------\n\n");
    }
};
} // namespace paio::core

using namespace paio::core;

int main (int argc, char** argv)
{
    // check argv for the file to be placed the result
    FILE* fd = stdout;

    // open file to write the logging results
    if (argc > 1) {
        fd = ::fopen (argv[1], "w");

        if (fd == nullptr) {
            fd = stdout;
        }
    }

    StageInfoTest test { fd };
    std::string name { "testing-class" };
    std::string description { "This data plane stage respects to a testing class." };

    // test constructors
    test.test_constructors (StageInfoConstructorsTest::empty, "");
    test.test_constructors (StageInfoConstructorsTest::single, name);
    test.test_constructors (StageInfoConstructorsTest::copy, name);

    // test get/set environment variable
    std::string stage_name = paio::options::option_environment_variable_name ();
    std::string stage_name_value = "paio-stage-info-test";
    test.test_set_environment (stage_name, stage_name_value);
    std::string stage_env = paio::options::option_environment_variable_env ();
    std::string stage_env_value = "tmp";
    test.test_set_environment (stage_env, stage_env_value);

    // test StageInfo serialization
    StageInfo stage_info { name };
    test.test_serialize (stage_info);

    // test StageInfo set description
    test.test_set_description (stage_info, description);

    return 0;
}
