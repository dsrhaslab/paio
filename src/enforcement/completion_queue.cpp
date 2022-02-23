/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/enforcement/completion_queue.hpp>

namespace paio::enforcement {

// CompletionQueue default constructor.
CompletionQueue::CompletionQueue () = default;

// CompletionQueue default destructor.
CompletionQueue::~CompletionQueue () = default;

// get_size call. Return the size of the completion queue. Thread-safe.
int CompletionQueue::get_size ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    return static_cast<int> (this->m_queue.size ());
}

// size call. Return the size of the completion queue.
int CompletionQueue::size ()
{
    return static_cast<int> (this->m_queue.size ());
}

// enqueue call. Enqueue a Result object in the queue.
void CompletionQueue::enqueue (const Result& result)
{
    std::unique_lock<std::mutex> lock (this->m_lock);
    bool is_empty = (size () == 0);

    // emplace Result object
    this->m_queue.push (std::move (result));

    // notify all waiting threads if CompletionQueue.size() == 0 before emplace
    if (is_empty) {
        this->m_is_empty.notify_all ();
    }
}

// dequeue call. Dequeue a Result object. Invoked by the background thread.
void CompletionQueue::dequeue (const uint64_t& ticket_id, Result& result)
{
    std::unique_lock<std::mutex> lock (this->m_lock);
    bool my_ticket = false;

    // dequeue correct Result object
    while (!my_ticket) {
        // wait for elements to be enqueued
        while (this->size () == 0) {
            this->m_is_empty.wait (lock);
        }

        // peek Result.ticketId: verify if the Result being handled respects to 'ticket_id'
        if (ticket_id == this->m_queue.front ().get_ticket_id ()) {
            my_ticket = true;
        }
    }

    // pass reference to 'result' and remove element from queue
    result = this->m_queue.front ();
    this->m_queue.pop ();
}

} // namespace paio::enforcement