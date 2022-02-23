/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/ticket.hpp>

namespace paio::enforcement {

// Ticket default constructor.
Ticket::Ticket () = default;

// Ticket parameterized constructor.
Ticket::Ticket (const uint64_t& ticket_id,
    const int& total_operations,
    const long& payload,
    const int& type,
    const int& context) :
    m_ticket_id { ticket_id },
    m_operations { total_operations },
    m_payload { payload },
    m_operation_type { type },
    m_operation_context { context }
{ }

// Ticket parameterized constructor with request content (buffer).
Ticket::Ticket (const uint64_t& ticket_id,
    const int& total_operations,
    const long& payload,
    const int& type,
    const int& context,
    const std::size_t& size,
    const unsigned char* data) :
    m_ticket_id { ticket_id },
    m_operations { total_operations },
    m_payload { payload },
    m_operation_type { type },
    m_operation_context { context },
    m_buffer_size { size },
    m_buffer { new unsigned char[size] }
{
    std::memcpy (this->m_buffer, data, size);
}

// Ticket copy constructor.
Ticket::Ticket (const Ticket& ticket) :
    m_ticket_id { ticket.m_ticket_id },
    m_operations { ticket.m_operations },
    m_payload { ticket.m_payload },
    m_operation_type { ticket.m_operation_type },
    m_operation_context { ticket.m_operation_context },
    m_buffer_size { ticket.m_buffer_size }
{
    m_buffer = new unsigned char[m_buffer_size];
    std::memcpy (this->m_buffer, ticket.m_buffer, ticket.m_buffer_size);
}

// Ticket assignment operator.
Ticket& Ticket::operator= (const Ticket& ticket)
{
    if (this != &ticket) {
        this->m_ticket_id = ticket.m_ticket_id;
        this->m_operations = ticket.m_operations;
        this->m_payload = ticket.m_payload;
        this->m_operation_type = ticket.m_operation_type;
        this->m_operation_context = ticket.m_operation_context;
        this->m_buffer_size = ticket.m_buffer_size;

        delete[] this->m_buffer;
        this->m_buffer = new unsigned char[this->m_buffer_size];
        this->m_buffer = std::move (ticket.m_buffer);
    }

    return *this;
}

// Ticket destructor.
Ticket::~Ticket ()
{
    delete[] this->m_buffer;
}

// get_ticket_id call. Get Ticket's identifier.
uint64_t Ticket::get_ticket_id () const
{
    return this->m_ticket_id;
}

// get_operations call. Get Ticket's total of operations of current request.
int Ticket::get_total_operations () const
{
    return this->m_operations;
}

// get_payload call. Get Ticket's I/O cost.
long Ticket::get_payload () const
{
    return this->m_payload;
}

// get_operation_type call. Get Ticket's I/O operation type.
int Ticket::get_operation_type () const
{
    return this->m_operation_type;
}

// get_operation_context call. Get Ticket's I/O operation context.
int Ticket::get_operation_context () const
{
    return this->m_operation_context;
}

// get_buffer_size call.Get Ticket's buffer size.
std::size_t Ticket::get_buffer_size () const
{
    return this->m_buffer_size;
}

// get_buffer call. Get pointer to Ticket's buffer.
unsigned char* Ticket::get_buffer () const
{
    return this->m_buffer;
}

// to_string call. Return a string object containing the Ticket's identifiers.
std::string Ticket::to_string () const
{
    std::stringstream stream;
    stream << this->m_ticket_id << ", ";
    stream << this->m_operations << ", ";
    stream << this->m_payload << ", ";
    stream << this->m_operation_type << ", ";
    stream << this->m_operation_context;

    return stream.str ();
}

} // namespace paio::enforcement
