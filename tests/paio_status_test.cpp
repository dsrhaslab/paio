/**
 *  Written by Ricardo Macedo.
 *  Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/utils/status.hpp>
#include <string>

namespace paio::utils {

class PStatusTest {

private:
    FILE* m_fd { stdout };

public:
    /**
     * PStatusTest default constructor.
     */
    PStatusTest () = default;

    /**
     * PStatusTest parameterized constructor.
     * @param fd Pointer to file through where results should be written to.
     */
    explicit PStatusTest (FILE* fd) : m_fd { fd }
    { }

    /**
     * PStatus default destructor.
     */
    ~PStatusTest () = default;

    /**
     * test_constructors: test different PStatus constructors options.´
     * @param status Reference to a PStatus object.
     * @param code String that corresponds to the status to be used.
     * @param print Boolean that defines if the result should written to the log file.
     */
    void test_constructors (PStatus& status, const std::string& code, bool print)
    {
        if (code == "ok") {
            status = PStatus::OK ();
        } else if (code == "notfound") {
            status = PStatus::NotFound ();
        } else if (code == "notsupported") {
            status = PStatus::NotSupported ();
        } else if (code == "error") {
            status = PStatus::Error ();
        } else {
            status = PStatus {};
        }

        if (print) {
            fprintf (this->m_fd, "PStatus: %s\n", status.to_string ().c_str ());
        }
    }

    /**
     * test_conditions: validate the status code of a given PStatus object.
     * @param status PStatus object to be validated.
     */
    void test_conditions (const PStatus& status)
    {
        fprintf (this->m_fd, "isOK(): %d\n", status.is_ok ());
        fprintf (this->m_fd, "isNotFound(): %d\n", status.is_not_found ());
        fprintf (this->m_fd, "isNotSupported(): %d\n", status.is_not_supported ());
        fprintf (this->m_fd, "isError(): %d\n\n", status.is_error ());
    }
};
} // namespace paio::utils

using namespace paio::utils;

int main (int argc, char** argv)
{
    // check argv for the file to be placed the result
    FILE* fd = stdout;
    bool log = true;

    // open file to write the logging results
    if (argc > 1) {
        fd = std::fopen (argv[1], "w");

        if (fd == nullptr) {
            fd = stdout;
        }
    }

    PStatusTest test { fd };
    PStatus status {};

    // test OK constructor
    test.test_constructors (status, "ok", log);
    test.test_conditions (status);

    // test NotFound constructor
    test.test_constructors (status, "notfound", log);
    test.test_conditions (status);

    // test NotSupported constructor
    test.test_constructors (status, "notsupported", log);
    test.test_conditions (status);

    // test Error constructor
    test.test_constructors (status, "error", log);
    test.test_conditions (status);

    // test unknown status
    test.test_constructors (status, "other", log);
    test.test_conditions (status);

    // close file
    if (fd != stdout) {
        std::fclose (fd);
    }
}
