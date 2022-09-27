/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2021-2022 INESC TEC.
 **/

#ifndef PAIO_LIBC_HEADERS_HPP
#define PAIO_LIBC_HEADERS_HPP

#include <fcntl.h>
#include <stdio.h>

namespace paio::headers {

// metadata-based operations
using libc_open_variadic_t = int (*) (const char*, int, ...);
using libc_close_t = int (*) (int);
using libc_fopen_t = FILE* (*)(const char*, const char*);
using libc_fclose_t = int (*) (FILE*);

// data-based operations
using libc_read_t = ssize_t (*) (int, void*, size_t);
using libc_write_t = ssize_t (*) (int, const void*, size_t);

} // namespace paio::headers

#endif // PAIO_LIBC_HEADERS_HPP