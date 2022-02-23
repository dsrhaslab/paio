/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_ENFORCEMENT_OBJECT_DRL_OPTIONS_HPP
#define PAIO_ENFORCEMENT_OBJECT_DRL_OPTIONS_HPP

#include <cstdint>

/*
 * ***********************************************************************************************
 * DRL Options: Default configurations for the DynamicRateLimiter enforcement object.
 * ***********************************************************************************************
 */

/**
 * TokenBucketType class.
 * Defines the type of the token-bucket to use.
 *  - normal: uses the TokenBucket class and performs the refill operation arithmetically.
 *  - threaded: uses the TokenBucketThreaded class and performs the refill operation in a dedicated
 *  thread that runs in background.
 */
enum class TokenBucketType { normal = 1, threaded = 2 };

/**
 * drl_option_token_bucket_type: Defines the type of TokenBucket to be used. By default, it uses
 * a normal TokenBucket class.
 */
const TokenBucketType drl_option_token_bucket_type = TokenBucketType::normal;

/**
 * drl_option_convergence_factor: Defines the default convergence factor for DynamicRateLimiter I/O
 * cost model estimation.
 */
constexpr static const float drl_option_convergence_factor = 0.5;

/**
 * drl_option_default_time_unit: Defines the default timeunit value, which is Microseconds.
 * timeunit {MICROSECONDS = 1; MILLISECONDS = 2; SECONDS = 3; NANOSECONDS = 4}
 */
const int drl_option_default_time_unit = 1;

/**
 * drl_option_collect_statistics: Defines the default option for collecting statistics in the DRL
 * object. BY default, statistic collection at DRL objects is disabled.
 */
const bool drl_option_collect_statistics = false;

/**
 * drl_option_gc_sliding_window: Defines the sliding window (in milliseconds) of the token-bucket's
 * statistics garbage collector.
 */
const uint64_t drl_option_gc_sliding_window = 5000000;

/**
 * drl_option_collect_statistics_max_size: Defines the maximum size of token-bucket statistics held
 * in the buffer.
 */
const int drl_option_collect_statistics_max_size = 100;

#endif // PAIO_ENFORCEMENT_OBJECT_DRL_OPTIONS_HPP
