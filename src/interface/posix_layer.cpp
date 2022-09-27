/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#include <paio/interface/posix_layer.hpp>

namespace paio {

// PosixLayer default constructor.
PosixLayer::PosixLayer () : InstanceInterface {}
{
    Logging::log_debug ("PosixLayer instance constructor.");
}

// PosixLayer parameterized constructor.
PosixLayer::PosixLayer (std::shared_ptr<PaioStage> stage_ptr) :
    InstanceInterface { std::move (stage_ptr) }
{
    Logging::log_debug ("PosixLayer (explicit) parameterized instance constructor.");
    this->set_default_operation_type (static_cast<int> (POSIX::no_op));
    this->set_default_operation_context (static_cast<int> (POSIX::no_op));
}

// PosixLayer parameterized constructor.
PosixLayer::PosixLayer (std::shared_ptr<PaioStage> stage_ptr, const long& default_workflow_id) :
    InstanceInterface { std::move (stage_ptr) }
{
    Logging::log_debug ("PosixLayer parameterized instance constructor.");
    this->set_default_workflow_id (default_workflow_id);
    this->set_default_operation_type (static_cast<int> (POSIX::no_op));
    this->set_default_operation_context (static_cast<int> (POSIX::no_op));
}

// PosixLayer parameterized constructor.
PosixLayer::PosixLayer (std::shared_ptr<PaioStage> stage_ptr,
    const long& default_workflow_id,
    const int& default_operation_type,
    const int& default_operation_context) :
    InstanceInterface { std::move (stage_ptr),
        default_workflow_id,
        default_operation_type,
        default_operation_context }
{
    Logging::log_debug ("PosixLayer (full) parameterized instance constructor.");
}

// PosixLayer default destructor.
PosixLayer::~PosixLayer ()
{
    Logging::log_debug_explicit ("PAIO Posix Layer destructor.");
}

// set_default_workflow_id call. Set new value for m_default_workflow_id.
void PosixLayer::set_default_workflow_id (const long& workflow_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_workflow_id (workflow_id);
}

// set_default_operation_type call. Set new value for m_default_operation_type.
void PosixLayer::set_default_operation_type (const int& operation_type)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_operation_type (operation_type);
}

// set_default_operation_context call. Set new value for m_default_operation_context.
void PosixLayer::set_default_operation_context (const int& operation_context)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_operation_context (operation_context);
}

// set_default_secondary_workflow_identifier call. Set new value for
// m_default_secondary_workflow_identifier.
void PosixLayer::set_default_secondary_workflow_identifier (
    const std::string& workflow_secondary_id)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    InstanceInterface::set_default_secondary_workflow_identifier (workflow_secondary_id);
}

// set_io_transformation call. Enable/disable m_has_io_transformation flag.
void PosixLayer::set_io_transformation (const bool& value)
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    this->m_has_io_transformation = value;
}

// build_context_object call. Build Context containing default I/O classifiers to enforce request.
Context PosixLayer::build_context_object ()
{
    std::lock_guard<std::mutex> guard (this->m_lock);
    // build Context object
    return this->build_context_object (this->m_default_workflow_id,
        this->m_default_operation_type,
        this->m_default_operation_context,
        1,
        1);
}

// build_context_object call. Build Context containing all I/O classifiers to enforce request.
Context PosixLayer::build_context_object (const long& workflow_id,
    const int& operation_type,
    const int& operation_context,
    const uint64_t& operation_size,
    const int& total_operations)
{
    // build Context object
    return Context { workflow_id,
        operation_type,
        operation_context,
        operation_size,
        total_operations };
}

// enforce call. Enforce I/O requests at the data plane stage.
void PosixLayer::enforce (const Context& context, Result& result)
{
    InstanceInterface::enforce (context, result);
}

// enforce call. Enforce I/O requests at the data plane stage.
void PosixLayer::enforce (const Context& context,
    const void* buffer,
    const size_t& size,
    Result& result)
{
    InstanceInterface::enforce (context, buffer, size, result);
}

// to_string call. Generate a string with the PosixLayer interface values.
std::string PosixLayer::to_string ()
{
    std::string message { "PosixLayer {" };
    {
        std::lock_guard<std::mutex> guard (this->m_lock);
        message.append (InstanceInterface::to_string ()).append ("}");
    }
    return message;
}

// write call. Differentiate and enforce requests of type write.
ssize_t PosixLayer::write (int fd, const void* buf, size_t count)
{
    // build Context object
    Context context = this->build_context_object (this->m_default_workflow_id,
        static_cast<int> (POSIX::write),
        this->m_default_operation_context,
        count,
        1);

    // invoke PAIO-enabled write
    return this->write (fd, buf, count, context);
}

// write call. Differentiate and enforce requests of type write.
ssize_t PosixLayer::write (int fd, const void* buf, size_t count, const Context& context)
{
    // create Result object
    Result result {};

    // verify if data plane stage will enforce transformations over the request (change the original
    // request's content), and enforce the request accordingly
    if (this->m_has_io_transformation) {
        // enforce request with buffer and count
        this->enforce (context, buf, count, result);
    } else {
        // enforce request only with Context object
        this->enforce (context, result);
    }

    // verify if stage has I/O transformations (change the original content of the request)
    // and verify the result status code from the PAIO enforcement request
    if (this->m_has_io_transformation && result.get_result_status () == ResultStatus::success) {
        return ::write (fd, result.get_content (), result.get_content_size ());
    } else if (result.get_result_status () == ResultStatus::success) {
        return ::write (fd, buf, count);
    } else {
        Logging::log_error ("PosixLayer: write operation was not successfully enforced.");
        return -1;
    }
}

// pwrite call. Differentiate and enforce requests of type pwrite.
ssize_t PosixLayer::pwrite (int fd, const void* buf, size_t count, off_t offset)
{
    // build Context object
    Context context = this->build_context_object (this->m_default_workflow_id,
        static_cast<int> (POSIX::pwrite),
        this->m_default_operation_context,
        count,
        1);

    // invoke PAIO-enabled pwrite
    return this->pwrite (fd, buf, count, offset, context);
}

// pwrite call. Differentiate and enforce requests of type pwrite.
ssize_t
PosixLayer::pwrite (int fd, const void* buf, size_t count, off_t offset, const Context& context)
{
    // create Result object
    Result result {};

    // verify if data plane stage will enforce transformations over the request (change the original
    // request's content), and enforce the request accordingly
    if (this->m_has_io_transformation) {
        // enforce request with buffer and count
        this->enforce (context, buf, count, result);
    } else {
        // enforce request only with Context object
        this->enforce (context, result);
    }

    // verify if stage has I/O transformation (changed the original content of the request)
    // and verify the result status code from the PAIO enforcement request
    if (this->m_has_io_transformation && result.get_result_status () == ResultStatus::success) {
        return ::pwrite (fd, result.get_content (), result.get_content_size (), offset);
    } else if (result.get_result_status () == ResultStatus::success) {
        return ::pwrite (fd, buf, count, offset);
    } else {
        Logging::log_error ("PosixLayer: pwrite operation was not successfully enforced.");
        return -1;
    }
}

// pwrite64 call. Differentiate and enforce requests of type pwrite64 (large file support).
#if defined(__USE_LARGEFILE64)
ssize_t PosixLayer::pwrite64 (int fd, const void* buf, size_t size, off64_t offset)
{
    // build Context object
    Context context = this->build_context_object (this->m_default_workflow_id,
        static_cast<int> (POSIX::pwrite64),
        this->m_default_operation_context,
        size,
        1);

    // invoke PAIO-enabled pwrite64
    return this->pwrite64 (fd, buf, size, offset, context);
}
#endif

// pwrite64 call. Differentiate and enforce requests of type pwrite64 (large file support).
#if defined(__USE_LARGEFILE64)
ssize_t
PosixLayer::pwrite64 (int fd, const void* buf, size_t size, off64_t offset, const Context& context)
{
    // create Result object
    Result result {};

    // verify if data plane stage will enforce transformations over the request (change the original
    // request's content), and enforce the request accordingly
    if (this->m_has_io_transformation) {
        // enforce request with buffer and count
        this->enforce (context, buf, size, result);
    } else {
        // enforce request only with Context object
        this->enforce (context, result);
    }

    // verify if stage has I/O transformation (changed the original content of the request)
    // and verify the result status code from the PAIO enforcement request
    if (this->m_has_io_transformation && result.get_result_status () == ResultStatus::success) {
        return ::pwrite64 (fd, result.get_content (), result.get_content_size (), offset);
    } else if (result.get_result_status () == ResultStatus::success) {
        return ::pwrite64 (fd, buf, size, offset);
    } else {
        Logging::log_error ("PosixLayer: pwrite64 operation was not successfully enforced.");
        return -1;
    }
}
#endif

// read call. Differentiate and enforce requests of type read.
ssize_t PosixLayer::read (int fd, void* buf, size_t count)
{
    // build Context object
    Context context = this->build_context_object (this->m_default_workflow_id,
        static_cast<int> (POSIX::read),
        this->m_default_operation_context,
        count,
        1);

    // invoke PAIO-enabled read
    return this->read (fd, buf, count, context);
}

// read call. Differentiate and enforce requests of type read.
ssize_t PosixLayer::read (int fd, void* buf, size_t count, const Context& context)
{
    ssize_t read_bytes;

    // create Result object
    Result result {};

    // verify if data plane stage will enforce transformations over the request (change the original
    // request's content), and enforce the request accordingly.
    // Contrarily to write-based operations, if the data plane stage has I/O transformations, read
    // first and then enforce the I/O mechanism
    if (this->m_has_io_transformation) {
        // read bytes from file system
        read_bytes = ::read (fd, buf, count);

        // validate if the buffer has content (i.e., the read operation has read bytes)
        if (read_bytes > 0) {
            // enforce request over buffer and count elements
            this->enforce (context, buf, read_bytes, result);

            // validate if request was successfully enforced or not
            if (result.get_result_status () != ResultStatus::success) {
                read_bytes = -1;
                Logging::log_error ("PosixLayer: read operation was not successfully enforced.");
            } else {
                // update read_bytes value since transformations can change the content's size
                read_bytes = static_cast<ssize_t> (result.get_content_size ());

                // reallocate buffer to the new size
                auto new_buf = ::realloc (buf, read_bytes);

                if (new_buf == nullptr) {
                    Logging::log_error ("PosixLayer: realloc failed.");
                    read_bytes = -1;
                } else {
                    // update buffer pointer
                    std::memcpy (new_buf, result.get_content (), read_bytes);
                }
            }
        }

        return read_bytes;
    } else {
        // enforce request only with Context object, and then read from file system
        this->enforce (context, result);
        return ::read (fd, buf, count);
    }
}

// pread call. Differentiate and enforce requests of type pread.
ssize_t PosixLayer::pread (int fd, void* buf, size_t count, off_t offset)
{
    // build default Context object with custom
    Context context = this->build_context_object (this->m_default_workflow_id,
        static_cast<int> (POSIX::pread),
        this->m_default_operation_context,
        count,
        1);

    // invoke PAIO-enabled pread
    return this->pread (fd, buf, count, offset, context);
}

// pread call. Differentiate and enforce requests of type pread.
ssize_t PosixLayer::pread (int fd, void* buf, size_t count, off_t offset, const Context& context)
{
    ssize_t read_bytes;

    // create Result object
    Result result {};

    // verify if data plane stage will enforce transformations over the request (change the original
    // request's content), and enforce the request accordingly.
    // Contrarily to write-based operations, if the data plane stage has I/O transformations, read
    // first and then enforce the I/O mechanism
    if (this->m_has_io_transformation) {
        // read bytes from file system
        read_bytes = ::pread (fd, buf, count, offset);

        // validate if the buffer has content (i.e., the read operation has read bytes)
        if (read_bytes > 0) {
            // enforce request over buffer and count elements
            this->enforce (context, buf, read_bytes, result);

            // validate if request was successfully enforced or not
            if (result.get_result_status () != ResultStatus::success) {
                read_bytes = -1;
                Logging::log_error ("PosixLayer: pread operation was not successfully enforced.");
            } else {
                // update read_bytes value since transformations can change the content's size
                read_bytes = static_cast<ssize_t> (result.get_content_size ());

                // reallocate buffer to the new size
                auto new_buf = ::realloc (buf, read_bytes);

                if (new_buf == nullptr) {
                    Logging::log_error ("PosixLayer: realloc failed.");
                    read_bytes = -1;
                } else {
                    // update buffer pointer
                    std::memcpy (new_buf, result.get_content (), read_bytes);
                }
            }
        }

        return read_bytes;

    } else {
        // enforce request only with Context object, and then read from file system
        this->enforce (context, result);
        return ::pread (fd, buf, count, offset);
    }
}

// pread64 call. Differentiate and enforce requests of type pread64 (large file support).
#if defined(__USE_LARGEFILE64)
ssize_t PosixLayer::pread64 (int fd, void* buf, size_t size, off64_t offset)
{
    // build default Context object with custom
    Context context = this->build_context_object (this->m_default_workflow_id,
        static_cast<int> (POSIX::pread64),
        this->m_default_operation_context,
        size,
        1);

    // invoke PAIO-enabled pread
    return this->pread64 (fd, buf, size, offset, context);
}

#endif

// pread64 call. Differentiate and enforce requests of type pread64 (large file support).
#if defined(__USE_LARGEFILE64)
ssize_t PosixLayer::pread64 (int fd, void* buf, size_t size, off64_t offset, const Context& context)
{
    ssize_t read_bytes;

    // create Result object
    Result result {};

    // verify if data plane stage will enforce transformations over the request (change the original
    // request's content), and enforce the request accordingly.
    // Contrarily to write-based operations, if the data plane stage has I/O transformations, read
    // first and then enforce the I/O mechanism
    if (this->m_has_io_transformation) {
        // read bytes from file system
        read_bytes = ::pread64 (fd, buf, size, offset);

        // validate if the buffer has content (i.e., the read operation has read bytes)
        if (read_bytes > 0) {
            // enforce request over buffer and size elements
            this->enforce (context, buf, read_bytes, result);

            // validate if request was successfully enforced or not
            if (result.get_result_status () != ResultStatus::success) {
                read_bytes = -1;
                Logging::log_error ("PosixLayer: pread operation was not successfully enforced.");
            } else {
                // update read_bytes value since transformations can change the content's size
                read_bytes = static_cast<ssize_t> (result.get_content_size ());

                // reallocate buffer to the new size
                auto new_buf = ::realloc (buf, read_bytes);

                if (new_buf == nullptr) {
                    Logging::log_error ("PosixLayer: realloc failed.");
                    read_bytes = -1;
                } else {
                    // update buffer pointer
                    std::memcpy (new_buf, result.get_content (), read_bytes);
                }
            }
        }

        return read_bytes;

    } else {
        // enforce request only with Context object, and then read from file system
        this->enforce (context, result);
        return ::pread64 (fd, buf, size, offset);
    }
}

#endif

// close call. Differentiate and enforce requests of type close.
int PosixLayer::close (int fd)
{
    Logging::log_error ("PosixLayer: close operation not implemented; bypassing enforcement.");
    return ::close (fd);
}

// fclose call. Differentiate and enforce requests of type fclose.
int PosixLayer::fclose (FILE* stream)
{
    Logging::log_error ("PosixLayer: fclose operation not implemented; bypassing enforcement.");
    return ::fclose (stream);
}

// open call. Differentiate and enforce requests of type open.
int PosixLayer::open (const char* path, int flags, ...)
{
    Logging::log_error ("PosixLayer: open operation not implemented; bypassing enforcement.");

    if (flags & O_CREAT) {
        va_list args;

        va_start (args, flags);
        mode_t mode = va_arg (args, int);
        va_end (args);

        return ::open (path, flags, mode);
    } else {
        return ::open (path, flags);
    }
}

// open64 call. Differentiate and enforce requests of type open64 (support for LFS).
#if defined(__USE_LARGEFILE64)
int PosixLayer::open64 (const char* path, int flags, ...)
{
    Logging::log_error ("PosixLayer: open64 operation not implemented; bypassing enforcement.");

    if (flags & O_CREAT) {
        va_list args;

        va_start (args, flags);
        mode_t mode = va_arg (args, int);
        va_end (args);

        return ::open64 (path, flags, mode);
    } else {
        return ::open64 (path, flags);
    }
}
#endif

// creat call. Differentiate and enforce requests of type creat.
int PosixLayer::creat (const char* path, mode_t mode)
{
    Logging::log_error ("PosixLayer: creat operation not implemented; bypassing enforcement.");
    return ::creat (path, mode);
}

// creat64 call. Differentiate and enforce requests of type creat64 (support for LFS).
#if defined(__USE_LARGEFILE64)
int PosixLayer::creat64 (const char* path, mode_t mode)
{
    Logging::log_error ("PosixLayer: creat64 operation not implemented; bypassing enforcement.");
    return ::creat64 (path, mode);
}
#endif

// openat call. Differentiate and enforce requests of type openat.
int PosixLayer::openat (int dirfd, const char* path, int flags, ...)
{
    Logging::log_error ("PosixLayer: openat operation not implemented; bypassing enforcement.");

    if (flags & O_CREAT) {
        va_list args;

        va_start (args, flags);
        mode_t mode = va_arg (args, int);
        va_end (args);

        return ::openat (dirfd, path, flags, mode);
    } else {
        return ::openat (dirfd, path, flags);
    }
}

// fopen call. Differentiate and enforce requests of type fopen.
FILE* PosixLayer::fopen (const char* pathname, const char* mode)
{
    Logging::log_error ("PosixLayer: fopen operation not implemented; bypassing enforcement.");
    return ::fopen (pathname, mode);
}

// fdopen call. Differentiate and enforce requests of type fdopen.
FILE* PosixLayer::fdopen (int fd, const char* mode)
{
    Logging::log_error ("PosixLayer: fdopen operation not implemented; bypassing enforcement.");
    return ::fdopen (fd, mode);
}

// rename call. Differentiate and enforce requests of type rename.
int PosixLayer::rename (const char* old_path, const char* new_path)
{
    Logging::log_error ("PosixLayer: rename operation not implemented; bypassing enforcement.");
    return ::rename (old_path, new_path);
}

// renameat call. Differentiate and enforce requests of type renameat.
int PosixLayer::renameat (int olddirfd, const char* old_path, int newdirfd, const char* new_path)
{
    Logging::log_error ("PosixLayer: renameat operation not implemented; bypassing enforcement.");
    return ::renameat (olddirfd, old_path, newdirfd, new_path);
}

// unlink call. Differentiate and enforce requests of type unlink.
int PosixLayer::unlink (const char* path)
{
    Logging::log_error ("PosixLayer: unlink operation not implemented; bypassing enforcement.");
    return ::unlink (path);
}

// unlinkat call. Differentiate and enforce requests of type unlinkat.
int PosixLayer::unlinkat (int dirfd, const char* pathname, int flags)
{
    Logging::log_error ("PosixLayer: unlinkat operation not implemented; bypassing enforcement.");
    return ::unlinkat (dirfd, pathname, flags);
}

// mkdir call. Differentiate and enforce requests of type mkdir.
int PosixLayer::mkdir (const char* path, mode_t mode)
{
    Logging::log_error ("PosixLayer: mkdir operation not implemented; bypassing enforcement.");
    return ::mkdir (path, mode);
}

// mkdirat call. Differentiate and enforce requests of type mkdirat.
int PosixLayer::mkdirat (int dirfd, const char* path, mode_t mode)
{
    Logging::log_error ("PosixLayer: mkdirat operation not implemented; bypassing enforcement.");
    return ::mkdirat (dirfd, path, mode);
}

// rmdir call. Differentiate and enforce requests of type rmdir.
int PosixLayer::rmdir (const char* path)
{
    Logging::log_error ("PosixLayer: rmdir operation not implemented; bypassing enforcement.");
    return ::rmdir (path);
}

// mknod call. Differentiate and enforce requests of type mknod.
int PosixLayer::mknod (const char* path, mode_t mode, dev_t dev)
{
    Logging::log_error ("PosixLayer: mknod operation not implemented; bypassing enforcement.");
    return ::mknod (path, mode, dev);
}

// mknodat call. Differentiate and enforce requests of type mknodat.
int PosixLayer::mknodat ([[maybe_unused]] int dirfd,
    [[maybe_unused]] const char* path,
    [[maybe_unused]] mode_t mode,
    [[maybe_unused]] dev_t dev)
{
    Logging::log_error ("PosixLayer: mknodat operation not implemented; bypassing enforcement.");
#ifdef __linux__
    return ::mknodat (dirfd, path, mode, dev);
#else
    return -1;
#endif
}

// getxattr call. Differentiate and enforce requests of type getxattr.
ssize_t PosixLayer::getxattr ([[maybe_unused]] const char* path,
    [[maybe_unused]] const char* name,
    [[maybe_unused]] void* value,
    [[maybe_unused]] size_t size)
{
    Logging::log_error ("PosixLayer: getxattr operation not implemented; bypassing enforcement.");
#ifdef __linux__
    return ::getxattr (path, name, value, size);
#else
    return -1;
#endif
}

// lgetxattr call. Differentiate and enforce requests of type lgetxattr.
ssize_t PosixLayer::lgetxattr ([[maybe_unused]] const char* path,
    [[maybe_unused]] const char* name,
    [[maybe_unused]] void* value,
    [[maybe_unused]] size_t size)
{
    Logging::log_error ("PosixLayer: lgetxattr operation not implemented; bypassing enforcement.");
#ifdef __linux__
    return ::lgetxattr (path, name, value, size);
#else
    return -1;
#endif
}

// fgetxattr call. Differentiate and enforce requests of type fgetxattr.
ssize_t PosixLayer::fgetxattr ([[maybe_unused]] int fd,
    [[maybe_unused]] const char* name,
    [[maybe_unused]] void* value,
    [[maybe_unused]] size_t size)
{
    Logging::log_error ("PosixLayer: fgetxattr operation not implemented; bypassing enforcement.");
#ifdef __linux__
    return ::fgetxattr (fd, name, value, size);
#else
    return -1;
#endif
}

// setxattr call. Differentiate and enforce requests of type setxattr.
int PosixLayer::setxattr ([[maybe_unused]] const char* path,
    [[maybe_unused]] const char* name,
    [[maybe_unused]] const void* value,
    [[maybe_unused]] size_t size,
    [[maybe_unused]] int flags)
{
    Logging::log_error ("PosixLayer: setxattr operation not implemented; bypassing enforcement.");
#ifdef __linux__
    return ::setxattr (path, name, value, size, flags);
#else
    return -1;
#endif
}

// lsetxattr call. Differentiate and enforce requests of type lsetxattr.
int PosixLayer::lsetxattr ([[maybe_unused]] const char* path,
    [[maybe_unused]] const char* name,
    [[maybe_unused]] const void* value,
    [[maybe_unused]] size_t size,
    [[maybe_unused]] int flags)
{
    Logging::log_error ("PosixLayer: lsetxattr operation not implemented; bypassing enforcement.");
#ifdef __linux__
    return ::lsetxattr (path, name, value, size, flags);
#else
    return -1;
#endif
}

// fsetxattr call. Differentiate and enforce requests of type fsetxattr.
int PosixLayer::fsetxattr ([[maybe_unused]] int fd,
    [[maybe_unused]] const char* name,
    [[maybe_unused]] const void* value,
    [[maybe_unused]] size_t size,
    [[maybe_unused]] int flags)
{
    Logging::log_error ("PosixLayer: fsetxattr operation not implemented; bypassing enforcement.");
#ifdef __linux__
    return ::fsetxattr (fd, name, value, size, flags);
#else
    return -1;
#endif
}

// posix_base call. Base operation (to be used when the POSIX interface is not suited).
ssize_t PosixLayer::posix_base (const void* buf, size_t count)
{
    // build Context object
    Context context = this->build_context_object ();

    // invoke PAIO-enabled noop operation
    return this->posix_base (buf, count, context);
}

// posix_base call. Base operation (to be used when the POSIX interface is not suited).
ssize_t PosixLayer::posix_base (const void* buf, size_t count, const Context& context)
{
    // create Result object
    Result result {};

    // verify if data plane stage will enforce transformations over the request (change the original
    // request's content), and enforce the request accordingly
    if (this->m_has_io_transformation) {
        // enforce request with buffer and count
        this->enforce (context, buf, count, result);
    } else {
        // enforce request only with Context object
        this->enforce (context, result);
    }

    if (result.get_result_status () == ResultStatus::success) {
        return (result.get_content_size () > 0) ? static_cast<ssize_t> (result.get_content_size ())
                                                : static_cast<ssize_t> (count);
    } else {
        Logging::log_error ("PosixLayer: noop operation was not successfully enforced ("
            + context.to_string () + ").");
        return -1;
    }
}

} // namespace paio
