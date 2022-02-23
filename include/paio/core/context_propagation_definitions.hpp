/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_CONTEXT_PROPAGATION_DEFINITIONS_HPP
#define PAIO_CONTEXT_PROPAGATION_DEFINITIONS_HPP

/**
 * TODO:
 *  - currently, operation type and operation context can be used interchangeably; consider having
 *  different set of I/O classifiers for the operation context and operation type. With this change,
 *  we will need on the statistics class to collect statistics based on operation type, operation
 *  context, or both. Also, we will need to change the RulesParser and SouthboundConnectionHandler
 *  classes.
 */
namespace paio::core {

/**
 * ContextType: Defines the available operation context classifiers.
 */
enum class ContextType {
    PAIO_GENERAL = 0,
    POSIX = 1,
    POSIX_META = 2,
    LSM_KVS_SIMPLE = 3,
    LSM_KVS_DETAILED = 4,
    KVS = 5
};

// ------------------------------------------------------------------------------------

/**
 * GENERAL definitions.
 * This can be used as generic definitions for I/O requests to be classified.
 * Currently, it considers the main context of the operation (foreground or background) and its
 * priority (high or low). This can be later adjusted as the number of use cases increases.
 */
enum class PAIO_GENERAL {
    foreground = 1,
    background = 2,
    high_priority = 3,
    low_priority = 4,
    no_op = 0
};

const int paio_general_size = 5;

// ------------------------------------------------------------------------------------

/**
 * LSM_KVS_DETAILED Definitions.
 * Defines the context of LSM KVS operations.
 * This is a "detailed" version of the context of POSIX operations submitted from the LSM key-value
 * store, since it decouples compactions in different levels (namely, compactions from L0->L0,
 * L0->L1, L1->L2, L2->L3, and LN).
 * This is useful when intercepting and controlling the I/O operations of LSM-based key-values
 * stores, such as RocksDB, LevelDB, PebblesDB, ...
 */
enum class LSM_KVS_DETAILED {
    bg_flush = 1,
    bg_compaction = 2,
    bg_compaction_L0_L0 = 3,
    bg_compaction_L0_L1 = 4,
    bg_compaction_L1_L2 = 5,
    bg_compaction_L2_L3 = 6,
    bg_compaction_LN = 7,
    foreground = 8,
    no_op = 0
};

const int lsm_kvs_detailed_size = 9;

// ------------------------------------------------------------------------------------

/**
 * LSM_KVS_SIMPLE Definitions.
 * Defines the context of LSM KVS operations.
 * This is a "simplified" version of the context of POSIX operations submitted from the LSM
 * key-value store, since it aggregates compactions by priority. For instance, high-priority
 * compactions (i.e., compactions that directly impact client tail latency) are L0->L0 and L0->L1.
 * Low-priority compactions include L1->L2, L2->L3, and LN.
 * This is useful when intercepting and controlling the I/O operations of LSM-based key-values
 * stores, such as RocksDB, LevelDB, PebblesDB, ...
 */
enum class LSM_KVS_SIMPLE {
    bg_flush = 1,
    bg_compaction_high_priority = 2,
    bg_compaction_low_priority = 3,
    foreground = 4,
    background = 5,
    no_op = 0
};

const int lsm_kvs_simple_size = 6;

// ------------------------------------------------------------------------------------

/**
 * POSIX definitions.
 * Defines the operation type of POSIX applications.
 */
enum class POSIX {
    read = 1,
    write = 2,
    pread = 3,
    pwrite = 4,
    pread64 = 5,
    pwrite64 = 6,
    fread = 7,
    fwrite = 8,
    open = 9,
    open64 = 10,
    creat = 11,
    creat64 = 12,
    openat = 13,
    close = 14,
    fsync = 15,
    fdatasync = 16,
    sync = 17,
    syncfs = 18,
    truncate = 19,
    truncate64 = 20,
    ftruncate = 21,
    ftruncate64 = 22,
    xstat = 23,
    xstat64 = 24,
    lxstat = 25,
    lxstat64 = 26,
    fxstat = 27,
    fxstat64 = 28,
    fxstatat = 29,
    fxstatat64 = 30,
    statfs = 31,
    statfs64 = 32,
    fstatfs = 33,
    fstatfs64 = 34,
    link = 35,
    linkat = 36,
    unlink = 37,
    unlinkat = 38,
    rename = 39,
    renameat = 40,
    symlink = 41,
    symlinkat = 42,
    readlink = 43,
    readlinkat = 44,
    fopen = 45,
    fopen64 = 46,
    fdopen = 47,
    freopen = 48,
    freopen64 = 49,
    fclose = 50,
    fflush = 51,
    access = 52,
    faccessat = 53,
    lseek = 54,
    lseek64 = 55,
    fseek = 56,
    fseek64 = 57,
    ftell = 58,
    fseeko = 59,
    fseeko64 = 60,
    ftello = 61,
    ftello64 = 62,
    mkdir = 63,
    mkdirat = 64,
    readdir = 65,
    readdir64 = 66,
    opendir = 67,
    fdopendir = 68,
    closedir = 69,
    rmdir = 70,
    dirfd = 71,
    getxattr = 72,
    lgetxattr = 73,
    fgetxattr = 74,
    setxattr = 75,
    lsetxattr = 76,
    fsetxattr = 77,
    listxattr = 78,
    llistxattr = 79,
    flistxattr = 80,
    removexattr = 81,
    lremovexattr = 82,
    fremovexattr = 83,
    chmod = 84,
    fchmod = 85,
    fchmodat = 86,
    chown = 87,
    lchown = 88,
    fchown = 89,
    fchownat = 90,
    no_op = 0
};

const int posix_size = 91;

/**
 * POSIX_META definitions.
 * Defines the "meta" operations of POSIX applications.
 *  - foreground and background define the context of the operation;
 *  - high_priority, med_priority, and low_priority define the priority of the operation;
 *  - data_op, meta_op, dir_op, ext_attr_op, and file_mod_op define the class of the operation.
 */
enum class POSIX_META {
    foreground = 1,
    background = 2,
    high_priority = 3,
    med_priority = 4,
    low_priority = 5,
    data_op = 6,
    meta_op = 7,
    dir_op = 8,
    ext_attr_op = 9,
    file_mod_op = 10,
    no_op = 0
};

const int posix_meta_size = 11;

// ------------------------------------------------------------------------------------

/**
 * KVS definitions.
 * Defines the operation type of LSM-based key-value stores, like LevelDB, RocksDB, and PebblesDB.
 */
enum class KVS {
    put = 1,
    get = 2,
    new_iterator = 3,
    delete_ = 4,
    write = 5,
    get_snapshot = 6,
    get_property = 7,
    get_approximate_size = 8,
    compact_range = 9,
    no_op = 0
};

const int kvs_size = 10;

// ------------------------------------------------------------------------------------

} // namespace paio::core

#endif // PAIO_CONTEXT_PROPAGATION_DEFINITIONS_HPP
