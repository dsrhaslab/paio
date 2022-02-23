/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_POSIX_LAYER_HPP
#define PAIO_POSIX_LAYER_HPP

#include <cstdarg>
#include <fcntl.h>
#include <paio/interface/instance_interface.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>

namespace paio {

/**
 * PosixLayer class.
 * The PosixLayer class implements the InstanceInterface methods, and is used for communication
 * between an I/O layer and a PAIO data plane stage. This interface provides the means to establish
 * the connection between workflows and the PAIO internal enforcement mechanisms.
 * Contrarily to PaioInstance, it exposes the build_context_object method, that allow building
 * Context objects (which contain the necessary metadata/classifiers to differentiate and classify
 * a request, and several (SDS-enabled) POSIX calls.
 * Specifically, POSIX methods are provided with and without the Context object. The vanilla
 * version, i.e., without the Context object, it provides the same interface (which promotes
 * transparency) since the Context object is created inside the method. However, it does not allow
 * context propagation. The extended version on the other hand, i.e., with Context object, does not
 * follow the same interface (loss of transparency) as the Context object needs to be created
 * outside the PosixLayer, but allows context propagation.
 * For an Application to use PAIO with POSIX file system backend, it only needs to replace the
 * original POSIX for the corresponding SDS-enabled one (considering respective vanilla and extended
 * versions).
 * TODO:
 *  - fix enforce methods in the case of !Status::Enforced;
 *  - test buffer size reallocation in read, pread, and pread64 methods (when using I/O
 *  transformations (compression, encryption), the size of the buffer can be different before and
 *  after the enforcement);
 *  - implement close, fclose, open, open64, creat, creat64, openat, fopen, fdopen, rename,
 *  renameat, unlink, unlinkat, mkdir, mkdirat, rmdir, mknod, mknodat, getxattr, lgetxattr,
 *  fgetxattr, setxattr, lsetxattr, and fsetxattr methods;
 *  - create testing class.
 */
class PosixLayer : public InstanceInterface {

protected:
    bool m_has_io_transformation { option_default_has_io_transformation };
    std::mutex m_lock;

    /**
     * enforce: Method for enforcing I/O requests at the data plane stage.
     * This method receives the Context object (which specifies all I/O classifiers) and a Result
     * object (which will contain the result after the enforcement). Further, this method assumes
     * that a performance-oriented mechanism will be applied, and thus, the request's content (i.e.,
     * buffer) and size are set with nullptr and 0, respectively.
     * @param context Context object containing all necessary metadata/classifiers to enforce the
     * I/O request.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     */
    void enforce (const Context& context, Result& result) override;

    /**
     * enforce_request: Method for enforcing I/O requests at the data plane stage.
     * This method receives the Context object (which specifies all I/O classifiers), the request's
     * content and size, and a Result object (which will contain the result after the enforcement).
     * @param context Context object containing all necessary metadata/classifiers to enforce the
     * I/O request.
     * @param buffer Content to be enforced.
     * @param size Size of the passed data content.
     * @param result Reference to a Result object that will store the result of enforcing the
     * corresponding enforcement mechanism over the request.
     */
    void enforce (const Context& context,
        const void* buffer,
        const size_t& size,
        Result& result) override;

public:
    /**
     * PosixLayer default constructor.
     * Initializes an InstanceInterface with default parameters.
     */
    PosixLayer ();

    /**
     * PosixLayer (explicit) parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     */
    explicit PosixLayer (std::shared_ptr<PaioStage> stage_ptr);

    /**
     * PosixLayer parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object
     * and the default workflow identifier. The default operation type and context are initialized
     * with default classifiers, namely POSIX::no_op.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     * @param default_workflow_id Defines the workflow identifier of the PosixLayer.
     */
    PosixLayer (std::shared_ptr<PaioStage> stage_ptr, const long& default_workflow_id);

    /**
     * PosixLayer parameterized constructor.
     * Initializes an InstanceInterface with the respective shared pointer to a PaioStage object,
     * the default workflow identifier, and the default operation type and context.
     * @param stage_ptr Shared-pointer to a PaioStage object.
     * @param default_workflow_id Defines the workflow identifier of the PosixLayer.
     * @param default_operation_type Defines the operation type of the PosixLayer.
     * @param default_operation_context Defines the operation context of the PosixLayer.
     */
    PosixLayer (std::shared_ptr<PaioStage> stage_ptr,
        const long& default_workflow_id,
        const int& default_operation_type,
        const int& default_operation_context);

    /**
     * PosixLayer default destructor.
     */
    ~PosixLayer () override;

    /**
     * set_default_workflow_id: Set new value in the workflow identifier I/O classifier.
     * This method is thread-safe.
     * @param workflow_id New value to be set in m_default_workflow_id.
     */
    void set_default_workflow_id (const long& workflow_id) override;

    /**
     * set_default_operation_type: Set new value in the default operation type I/O classifier.
     * This method is thread-safe.
     * @param operation_type New value to be set in m_default_operation_type.
     */
    void set_default_operation_type (const int& operation_type) override;

    /**
     * set_default_operation_context: Set new value in the default operation context I/O classifier.
     * This method is thread-safe.
     * @param operation_context New value to be set in m_default_operation_context.
     */
    void set_default_operation_context (const int& operation_context) override;

    /**
     * set_default_secondary_workflow_identifier: Set new value in the workflow secondary I/O
     * classifier.
     * This method is thread-safe.
     * @param workflow_secondary_id New value to be set in m_default_secondary_workflow_identifier.
     */
    void set_default_secondary_workflow_identifier (
        const std::string& workflow_secondary_id) override;

    /**
     * set_io_transformation: Define if I/O transformations will be used to handle the I/O requests
     * accordingly. For example, considering an enforcement object that performs encryption: in
     * write-based requests, the transformation should be done before submitting the request to the
     * file system (encrypt (content) -> write (content')); in read-based requests, the operation
     * should be done first and then apply the respective transformation
     * (read (content) -> decrypt (content)).
     * This method is thread-safe.
     * @param value New value to be set in the m_has_io_transformation.
     */
    void set_io_transformation (const bool& value);

    /**
     * build_context_object: Method for building Context objects for the differentiation and
     * classification of application requests at the Paio data plane stage.
     * The resulting context object will apply the default I/O classifiers, namely
     * m_default_workflow_id, m_default_operation_type, and m_default_operation_context.
     * @return Returns the respective Context object (following a RVO mechanism).
     */
    Context build_context_object () override;

    /**
     * build_context_object: Method for building Context objects for the differentiation and
     * classification of application requests at the Paio data plane stage.
     * @param workflow_id Defines the workflow identifier used to submit the request (from the
     * application to the data plane stage).
     * @param operation_type Defines the type of the submitted operation (from the application to
     * the data plane stage).
     * @param operation_context Defines the context of the submitted operation (from the application
     * to the data plane stage).
     * @param operation_size Defines the size of the operation to be enforced.
     * @param total_operations Defines the total number of operations that will be enforced.
     * @return Returns the respective Context object (following an RVO mechanism).
     */
    Context build_context_object (const long& workflow_id,
        const int& operation_type,
        const int& operation_context,
        const uint64_t& operation_size,
        const int& total_operations) override;

    /**
     * write: Write to a file descriptor.
     * This method follows the same interface as POSIX. To do so, the Context object is created
     * internally, and the operation is submitted to the PAIO data plane stage to be enforced.
     * @param fd File descriptor of the referred file.
     * @param buf Buffer that contains the content to be written.
     * @param count Number of bytes to be written.
     * @return On success, the number of bytes written is returned. On error, -1 is returned.
     */
    ssize_t write (int fd, const void* buf, size_t count);

    /**
     * write: Write to a file descriptor.
     * This method extends the original POSIX interface by exposing a Context object, which must be
     * created outside the PAIO scope. The operation is then submitted to the PAIO data plane stage
     * to be enforced.
     * @param fd File descriptor of the referred file.
     * @param buf Buffer that contains the content to be written.
     * @param count Number of bytes to be written.
     * @param context Defines the classifiers to differentiate and classify the operation in the
     * data plane stage.
     * @return On success, the number of bytes written is returned. On error, -1 is returned.
     */
    ssize_t write (int fd, const void* buf, size_t count, const Context& context);

    /**
     * pwrite: Write to a file descriptor at a given offset.
     * This method follows the same interface as POSIX. To do so, the Context object is created
     * internally, and the operation is submitted to the PAIO data plane stage to be enforced.
     * @param fd File descriptor of the referred file.
     * @param buf Buffer that contains the content to be written.
     * @param count Number of bytes to be written.
     * @param offset Initial offset to begin the write.
     * @return On success, the number of bytes written is returned. On error, -1 is returned.
     */
    ssize_t pwrite (int fd, const void* buf, size_t count, off_t offset);

    /**
     * pwrite: Write to a file descriptor at a given offset.
     * This method extends the original POSIX interface by exposing a Context object, which must be
     * created outside the PAIO scope. The operation is then submitted to the PAIO data plane stage
     * to be enforced.
     * @param fd File descriptor of the referred file.
     * @param buf Buffer that contains the content to be written.
     * @param count Number of bytes to be written.
     * @param offset Initial offset to begin the write.
     * @param context Defines the classifiers to differentiate and classify the operation in the
     * data plane stage.
     * @return On success, the number of bytes written is returned. On error, -1 is returned.
     */
    ssize_t pwrite (int fd, const void* buf, size_t count, off_t offset, const Context& context);

/**
 * pwrite64: Write to a file descriptor at a given offset.
 * This method is designed for Large File Support (LFS), and follows the POSIX interface.
 * To do so, the Context object is created internally, and the operation is submitted to the
 * PAIO data plane stage to be enforced.
 * @param fd File descriptor of the referred file.
 * @param buf Buffer that contains the content to be written.
 * @param count Number of bytes to be written.
 * @param offset Initial offset to begin the write.
 * @return On success, the number of bytes written is returned. On error, -1 is returned.
 */
#if defined(__USE_LARGEFILE64)
    ssize_t pwrite64 (int fd, const void* buf, size_t size, off64_t offset);
#endif

/**
 * pwrite64: Write to a file descriptor at a given offset.
 * This method is designed for Large File Support (LFS), and extends the original POSIX
 * interface, by exposing a Context object, which must be created outside the PAIO scope.
 * The operation is then submitted to the PAIO data plane stage to be enforced.
 * @param fd File descriptor of the referred file.
 * @param buf Buffer that contains the content to be written.
 * @param count Number of bytes to be written.
 * @param offset Initial offset to begin the write.
 * @param context Defines the classifiers to differentiate and classify the operation in the
 * data plane stage.
 * @return On success, the number of bytes written is returned. On error, -1 is returned.
 */
#if defined(__USE_LARGEFILE64)
    ssize_t pwrite64 (int fd, const void* buf, size_t size, off64_t offset, const Context& context);
#endif

    /**
     * read: Read from a file descriptor.
     * This method follows the same interface as POSIX. To do so, the Context object is created
     * internally, and the operation is submitted to the PAIO data plane stage to be enforced.
     * @param fd File descriptor of the referred file.
     * @param buf Buffer that will emplace the content to be read.
     * @param count Number of bytes to be read.
     * @return On success, the number of bytes read is returned (zero indicates end of file), and
     * the file position is advanced by this number. On error, -1 is returned.
     */
    ssize_t read (int fd, void* buf, size_t count);

    /**
     * read: Read from a file descriptor.
     * This method extends the original POSIX interface by exposing a Context object, which must be
     * created outside the PAIO scope. The operation is then submitted to the PAIO data plane stage
     * to be enforced.
     * @param fd File descriptor of the referred file.
     * @param buf Buffer that will emplace the content to be read.
     * @param count Number of bytes to be read.
     * @param context Defines the classifiers to differentiate and classify the operation in the
     * data plane stage.
     * @return On success, the number of bytes read is returned (zero indicates end of file), and
     * the file position is advanced by this number. On error, -1 is returned.
     */
    ssize_t read (int fd, void* buf, size_t count, const Context& context);

    /**
     * pread: Read from a file descriptor at a given offset.
     * This method follows the same interface as POSIX. To do so, the Context object is created
     * internally, and the operation is submitted to the PAIO data plane stage to be enforced.
     * @param fd File descriptor of the referred file.
     * @param buf Buffer that will emplace the content to be read.
     * @param count Number of bytes to be read.
     * @param offset Initial offset to begin the read.
     * @return On success, the number of bytes read is returned (zero indicates end of file), and
     * the file position is advanced by this number. On error, -1 is returned.
     */
    ssize_t pread (int fd, void* buf, size_t count, off_t offset);

    /**
     * pread: Read from a file descriptor at a given offset.
     * This method extends the original POSIX interface by exposing a Context object, which must be
     * created outside the PAIO scope. The operation is then submitted to the PAIO data plane stage
     * to be enforced.
     * @param fd File descriptor of the referred file.
     * @param buf Buffer that will emplace the content to be read.
     * @param count Number of bytes to be read.
     * @param offset Initial offset to begin the read.
     * @param context Defines the classifiers to differentiate and classify the operation in the
     * data plane stage.
     * @return On success, the number of bytes read is returned (zero indicates end of file), and
     * the file position is advanced by this number. On error, -1 is returned.
     */
    ssize_t pread (int fd, void* buf, size_t count, off_t offset, const Context& context);

/**
 * pread64: Read from a file descriptor at a given offset.
 * This method is designed for Large File Support (LFS), and follows the POSIX interface.
 * To do so, the Context object is created internally, and the operation is submitted to the
 * PAIO data plane stage to be enforced.
 * @param fd File descriptor of the referred file.
 * @param buf Buffer that will emplace the content to be read.
 * @param count Number of bytes to be read.
 * @param offset Initial offset to begin the read.
 * @return On success, the number of bytes read is returned (zero indicates end of file), and
 * the file position is advanced by this number. On error, -1 is returned.
 */
#if defined(__USE_LARGEFILE64)
    ssize_t pread64 (int fd, void* buf, size_t size, off64_t offset);
#endif

/**
 * pread64: Read from a file descriptor at a given offset.
 * This method is designed for Large File Support (LFS), and extends the original POSIX
 * interface, by exposing a Context object, which must be created outside the PAIO scope. The
 * operation is then submitted to the PAIO data plane stage to be enforced.
 * @param fd File descriptor of the referred file.
 * @param buf Buffer that will emplace the content to be read.
 * @param count Number of bytes to be read.
 * @param offset Initial offset to begin the read.
 * @return On success, the number of bytes read is returned (zero indicates end of file), and
 * the file position is advanced by this number. On error, -1 is returned.
 */
#if defined(__USE_LARGEFILE64)
    ssize_t pread64 (int fd, void* buf, size_t size, off64_t offset, const Context& context);
#endif

    /**
     * close: Close a file descriptor.
     * @param fd File descriptor of the referred file.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int close (int fd);

    /**
     * fclose: Flushes the stream pointed to by stream and closes the underlying file descriptor.
     * @param stream Stream to be flushed and closed.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int fclose (FILE* stream);

    /**
     * open: Open a file at the given path.
     * @param path Path to the file to be opened.
     * @param flags Flags to be used to open the file.
     * @return On success, a file descriptor is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int open (const char* path, int flags, ...);

    /**
     * open64: Open a file at the given path, under LFS support.
     * @param path Path to the file to be opened.
     * @param flags Flags to be used to open the file.
     * @return On success, a file descriptor is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
#if defined(__USE_LARGEFILE64)
    int open64 (const char* path, int flags, ...);
#endif

    /**
     * creat: Open and possibly create a file or device at the given path.
     * @param path Path to the file to be opened.
     * @param mode Mode to be used to open the file.
     * @return On success, a file descriptor is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int creat (const char* path, mode_t mode);

    /**
     * creat64: Open and possibly create a file or device at the given path, under LFS support.
     * @param path Path to the file to be opened.
     * @param mode Mode to be used to open the file.
     * @return On success, a file descriptor is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
#if defined(__USE_LARGEFILE64)
    int creat64 (const char* path, mode_t mode);
#endif

    /**
     * openat: Open a file at the given path, relative to the given directory.
     * @param dirfd File descriptor of the directory.
     * @param path Path to the file to be opened.
     * @param flags Flags to be used to open the file.
     * @return On success, a file descriptor is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int openat (int dirfd, const char* path, int flags, ...);

    /**
     * fopen: Open a file whose name is the string pointed to by pathname, and associates a stream
     * with it.
     * @param pathname Path to the file to be opened.
     * @param mode Mode to be used to open the file.
     * @return On success, a pointer to the opened stream is returned. On error, NULL is returned,
     * and errno is set appropriately.
     */
    FILE* fopen (const char* pathname, const char* mode);

    /**
     * fdopen: Associate a stream with a the existing file descriptor.
     * @param fd File descriptor of the referred file.
     * @param mode Mode to be used to open the file.
     * @return On success, a pointer to the opened stream is returned. On error, NULL is returned,
     * and errno is set appropriately.
     */
    FILE* fdopen (int fd, const char* mode);

    /**
     * rename: renames a file, moving it between directories if required.
     * @param old_path Path to the file to be renamed.
     * @param new_path Path to the new file name.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int rename (const char* old_path, const char* new_path);

    /**
     * renameat: renames a file, moving it between directories if required.
     * @param olddirfd File descriptor of the directory containing the file to be renamed.
     * @param old_path Path to the file to be renamed.
     * @param newdirfd File descriptor of the directory where the file will be moved to.
     * @param new_path Path to the new file name.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int renameat (int olddirfd, const char* old_path, int newdirfd, const char* new_path);

    /**
     * unlink: Deletes a file.
     * @param path Path to the file to be deleted.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int unlink (const char* path);

    /**
     * unlinkat: Deletes a file.
     * @param dirfd File descriptor of the directory containing the file to be deleted.
     * @param pathname Path to the file to be deleted.
     * @param flags Flags to be used to delete the file.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int unlinkat (int dirfd, const char* pathname, int flags);

    /**
     * mkdir: Creates a directory.
     * @param path Path to the directory to be created.
     * @param mode Mode to be used to create the directory.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int mkdir (const char* path, mode_t mode);

    /**
     * mkdirat: Creates a directory.
     * @param dirfd File descriptor of the directory where the directory will be created.
     * @param path Path to the directory to be created.
     * @param mode Mode to be used to create the directory.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int mkdirat (int dirfd, const char* path, mode_t mode);

    /**
     * rmdir: Removes a directory.
     * @param path Path to the directory to be removed.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int rmdir (const char* path);

    /**
     * mknod: Creates a filesystem node (file, device special file, or named pipe) named path, with
     * attributed specified by mode and dev.
     * @param path Path to the file to be created.
     * @param mode Mode to be used to create the file.
     * @param dev Device number to be used to create the file.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int mknod (const char* path, mode_t mode, dev_t dev);

    /**
     * mknodat: Creates a filesystem node (file, device special file, or named pipe) named path,
     * with attributed specified by mode and dev.
     * @param dirfd File descriptor of the directory where the file will be created.
     * @param path Path to the file to be created.
     * @param mode Mode to be used to create the file.
     * @param dev Device number to be used to create the file.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int mknodat (int dirfd, const char* path, mode_t mode, dev_t dev);

    /**
     * getxattr: Retrieves extended attributes of the file specified by path.
     * @param path Path to the file.
     * @param name Name of the extended attribute to be retrieved.
     * @param value Pointer to the buffer where the value of the extended attribute will be stored.
     * @param size Size of the buffer pointed by value.
     * @return On success, the number of bytes written to value is returned. On error, -1 is
     * returned, and errno is set appropriately.
     */
    ssize_t getxattr (const char* path, const char* name, void* value, size_t size);

    /**
     * lgetxattr: Retrieves extended attributes of the file specified by path.
     * @param path Path to the file.
     * @param name Name of the extended attribute to be retrieved.
     * @param value Pointer to the buffer where the value of the extended attribute will be stored.
     * @param size Size of the buffer pointed by value.
     * @return On success, the number of bytes written to value is returned. On error, -1 is
     * returned, and errno is set appropriately.
     */
    ssize_t lgetxattr (const char* path, const char* name, void* value, size_t size);

    /**
     * fgetxattr: Retrieves extended attributes of the file specified by path.
     * @param fd File descriptor of the file.
     * @param name Name of the extended attribute to be retrieved.
     * @param value Pointer to the buffer where the value of the extended attribute will be stored.
     * @param size Size of the buffer pointed by value.
     * @return On success, the number of bytes written to value is returned. On error, -1 is
     * returned, and errno is set appropriately.
     */
    ssize_t fgetxattr (int fd, const char* name, void* value, size_t size);

    /**
     * setxattr: Sets extended attributes of the file specified by path.
     * @param path Path to the file.
     * @param name Name of the extended attribute to be set.
     * @param value Pointer to the buffer where the value of the extended attribute will be stored.
     * @param size Size of the buffer pointed by value.
     * @param flags Flags that control the operation of setxattr.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int setxattr (const char* path, const char* name, const void* value, size_t size, int flags);

    /**
     * lsetxattr: Sets extended attributes of the file specified by path.
     * @param path Path to the file.
     * @param name Name of the extended attribute to be set.
     * @param value Pointer to the buffer where the value of the extended attribute will be stored.
     * @param size Size of the buffer pointed by value.
     * @param flags Flags that control the operation of lsetxattr.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int lsetxattr (const char* path, const char* name, const void* value, size_t size, int flags);

    /**
     * fsetxattr: Sets extended attributes of the file specified by path.
     * @param fd File descriptor of the file.
     * @param name Name of the extended attribute to be set.
     * @param value Pointer to the buffer where the value of the extended attribute will be stored.
     * @param size Size of the buffer pointed by value.
     * @param flags Flags that control the operation of fsetxattr.
     * @return On success, zero is returned. On error, -1 is returned, and errno is set
     * appropriately.
     */
    int fsetxattr (int fd, const char* name, const void* value, size_t size, int flags);

    /**
     * posix_base: Base operation to be used in scenarios where that original POSIX API is not
     * applicable or not suited to express the desired operation. The Context object is created
     * internally.
     * @param buf Buffer that contains the data to be enforced.
     * @param count Number of bytes in the buffer.
     * @return On success, returns the number of bytes processed. On error, -1 is returned.
     */
    ssize_t posix_base (const void* buf, size_t count);

    /**
     * posix_base: Base operation to be used in scenarios where that original POSIX API is not
     * applicable or not suited to express the desired operation.
     * @param buf Buffer that contains the data to be enforced.
     * @param count Number of bytes in the buffer.
     * @param context Defines the classifiers to differentiate and classify the operation in the
     * data plane stage.
     * @return On success, returns the number of bytes processed. On error, -1 is returned.
     */
    ssize_t posix_base (const void* buf, size_t count, const Context& context);

    /**
     * to_string: Generate a string with the PosixLayer interface values.
     * @return Returns the PosixLayer's values in string-based format.
     */
    std::string to_string () override;
};
} // namespace paio

#endif // PAIO_POSIX_LAYER_HPP
