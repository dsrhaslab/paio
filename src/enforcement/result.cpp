/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/result.hpp>

namespace paio::enforcement {

// Result default constructor.
Result::Result () = default;

// Result parameterized constructor.
Result::Result (const uint64_t& ticket_id) : m_ticket_id { ticket_id }
{ }

// Result parameterized constructor without content.
Result::Result (const uint64_t& ticket_id, const ResultStatus& status) :
    m_ticket_id { ticket_id },
    m_result_status { status }
{ }

// Result parameterized constructor with content.
Result::Result (const uint64_t& ticket_id,
    const ResultStatus& status,
    const bool& has_content,
    const std::size_t& content_size,
    unsigned char* buffer) :
    m_ticket_id { ticket_id },
    m_result_status { status },
    m_has_content { has_content },
    m_content_size { content_size }
{
    this->m_content = new unsigned char[content_size];
    std::memcpy (this->m_content, buffer, content_size);

    // if (Logging::is_debug_enabled()) {
    //     std::stringstream stream;
    //     stream << "Result parameterized constructor (" << this << ").\n";
    //     Logging::log_debug(stream.str());
    //  }
}

// Result copy constructor.
Result::Result (const Result& result) :
    m_ticket_id { result.m_ticket_id },
    m_result_status { result.m_result_status },
    m_has_content { result.m_has_content },
    m_content_size { 0 },
    m_content { nullptr }
{
    if (result.m_has_content) {
        this->m_content_size = result.m_content_size;
        this->m_content = new unsigned char[result.m_content_size];
        std::memcpy (this->m_content, result.m_content, result.m_content_size);
    }
}

// Result move constructor.
Result::Result (Result&& result) noexcept :
    m_ticket_id { result.m_ticket_id },
    m_result_status { result.m_result_status },
    m_has_content { result.m_has_content }
{
    if (result.m_has_content) {
        this->m_content_size = result.m_content_size;
        this->m_content = std::move (result.m_content);

        result.m_ticket_id = 0;
        result.m_result_status = ResultStatus::none;
        result.m_has_content = false;
        result.m_content_size = 0;
        result.m_content = nullptr;
    }
}

// Result assignment operator.
Result& Result::operator= (const Result& result)
{
    if (this != &result) {
        this->m_ticket_id = result.m_ticket_id;
        this->m_result_status = result.m_result_status;
        this->m_has_content = result.m_has_content;

        if (result.m_has_content) {
            delete[] this->m_content;
            this->m_content_size = result.m_content_size;
            this->m_content = new unsigned char[result.m_content_size];
            std::memcpy (this->m_content, result.m_content, result.m_content_size);
        } else {
            this->m_content_size = 0;
            this->m_content = nullptr;
        }
    }

    return *this;
}

// Result move assignment operator.
Result& Result::operator= (Result&& result) noexcept
{
    if (this != &result) {
        this->m_ticket_id = result.m_ticket_id;
        this->m_result_status = result.m_result_status;
        this->m_has_content = result.m_has_content;

        if (result.m_has_content) {
            delete[] this->m_content;
            this->m_content_size = result.m_content_size;
            this->m_content = std::move (result.m_content);
        } else {
            this->m_content_size = 0;
            this->m_content = nullptr;
        }

        result.m_ticket_id = 0;
        result.m_result_status = ResultStatus::none;
        result.m_has_content = false;
        result.m_content_size = 0;
        result.m_content = nullptr;
    }

    return *this;
}

// Result default destructor.
Result::~Result ()
{
    delete[] this->m_content;
};

// get_ticket_id call. Return a copy of the Result's ticket identifier.
uint64_t Result::get_ticket_id () const
{
    return this->m_ticket_id;
}

// get_result_status call. Return a copy of the Result's status.
ResultStatus Result::get_result_status () const
{
    return this->m_result_status;
}

// get_has_content call. Return a copy of m_has_content that determines if it has content or not.
bool Result::get_has_content () const
{
    return this->m_has_content;
}

// get_content_size call. Return a copy of the result's content size.
std::size_t Result::get_content_size () const
{
    return this->m_content_size;
}

// get_content call. Return a pointer to the Result's content.
unsigned char* Result::get_content () const
{
    return this->m_content;
}

// set_ticket_id call. Update Result's m_ticket_id value.
void Result::set_ticket_id (const uint64_t& ticket_id)
{
    this->m_ticket_id = ticket_id;
}

// set_result_status call. Update Result's m_result_status value.
void Result::set_result_status (const ResultStatus& status)
{
    this->m_result_status = status;
}

// set_has_content call. Update Result's n_has_content value.
void Result::set_has_content (const bool& has_content)
{
    this->m_has_content = has_content;
}

// set_content_size call. Update Result's m_content_size value.
void Result::set_content_size (const std::size_t& size)
{
    if (this->m_has_content) {
        this->m_content_size = size;
    } else {
        Logging::log_error ("Result: cannot set content size.");
    }
}

// set_content call. Update Result's m_content buffer.
void Result::set_content (const std::size_t& size, const unsigned char* buffer)
{
    // if has_content is true, perform a memory copy of the content's buffer
    if (this->m_has_content) {
        this->m_content = new unsigned char[size];
        std::memcpy (this->m_content, buffer, size);
    } else {
        Logging::log_error ("Result: cannot set content.");
    }
}

// to_string call. Return a string value of Result.
std::string Result::to_string () const
{
    std::stringstream stream;
    stream << m_ticket_id << ", ";
    stream << static_cast<int> (m_result_status) << ", ";
    stream << (m_has_content ? "true" : "false") << ", ";
    stream << m_content_size;

    if (m_has_content && m_content != nullptr) {
        stream << ", " << (reinterpret_cast<char*> (m_content));
    }

    return stream.str ();
}

} // namespace paio::enforcement