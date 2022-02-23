/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_COMPLETION_QUEUE_HPP
#define PAIO_COMPLETION_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <paio/enforcement/result.hpp>
#include <queue>

namespace paio::enforcement {

/**
 * CompletionQueue class.
 * The CompletionQueue class provides a queue to store the results of previously enforced I/O
 * requests. This is used in conjunction with the SubmissionQueue.
 * Both enqueue and dequeue processes are thread-safe.
 * TODO:
 *  - have multiple threads dequeuing from the SubmissionQueue, enforcing the respective I/O
 *  mechanism, and enqueue in the CompletionQueue;
 *  - move ownership of Result objects.
 */
class CompletionQueue {
    // friend classes of CompletionQueue
    friend class SubmissionQueue;

private:
    std::queue<Result> m_queue;
    std::mutex m_lock;
    std::condition_variable m_is_empty;

    /**
     * size: Return the size of the completion queue. Must be used by a thread-safe method, namely
     * enqueue and dequeue.
     * @return Return m_queue's size.
     */
    int size ();

    /**
     * enqueue: Enqueue a Result object in the CompletionQueue (m_queue). If the queue was
     * previously empty, the thread will notify all waiting threads that a new element has become
     * available and is ready to be consumed.
     * Thread-safe.
     * @param result Result object to be enqueued.
     */
    void enqueue (const Result& result);

public:
    /**
     * CompletionQueue default constructor.
     */
    CompletionQueue ();

    /**
     * CompletionQueue default destructor.
     */
    ~CompletionQueue ();

    /**
     * get_size: Return the size of the completion queue (m_queue).
     * Thread-safe.
     * @return Size of m_queue.
     */
    int get_size ();

    /**
     * dequeue: Dequeue a Result object from the CompletionQueue (m_queue).
     * First, the method verifies if m_queue has elements, and waits in case if it is empty.
     * Otherwise, it peeks the Result's ticket identifier and validate if its the one that we want
     * to handle, namely @param ticket_id. If it is not the ticket, the thread will wait until other
     * thread picks the elements. Otherwise, it moves the enqueued element to @param result and
     * remove it from the queue.
     * Thread-safe.
     * @param ticket_id Ticket identifier that defines which ticket to dequeue.
     * @param result Reference to a result object to emplace the dequeued element.
     */
    void dequeue (const uint64_t& ticket_id, Result& result);
};

} // namespace paio::enforcement

#endif // PAIO_COMPLETION_QUEUE_HPP
