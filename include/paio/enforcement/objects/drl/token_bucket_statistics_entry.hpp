/**
 *   Written by Ricardo Macedo.
 *   Copyright (c) 2020-2022 INESC TEC.
 **/

#ifndef PAIO_TOKEN_BUCKET_STATISTICS_ENTRY_HPP
#define PAIO_TOKEN_BUCKET_STATISTICS_ENTRY_HPP

namespace paio::enforcement {

/**
 * TBStatsEntry class.
 * This class is used to store token-bucket statistic entries.
 * Each entry is made of 3 parameters:
 *  - m_normalized_empty_bucket: defines a normalized value that marks when the token-bucket
 *  could not consume;
 *  - m_tokens_left: defines the number of tokens left in bucket when the consume operations was not
 *  fulfilled;
 *  - m_collection_timestamp: defined the time, in milliseconds, that marks the statistic collection
 * Each of TBStatsEntry object is then grouped and sent to the controller upon being
 * requested. Thus, this object is then used in the DRL enforcement object and TokenBucket, as well
 * as the southbound interface.
 * Before being dispatched through the network, it must be first serialized in binary format.
 */
class TBStatsEntry {
private:
    float m_normalized_empty_bucket { 0 };
    double m_tokens_left { 0 };
    uint64_t m_collection_timestamp { 0 };

public:
    /**
     * TBStatsEntry default constructor.
     */
    TBStatsEntry () = default;

    /**
     * TBStatsEntry parameterized constructor.
     * @param normalized_bucket_value Normalized value that marks when the bucket could not consume
     * @param tokens_left number of tokens left in the bucket when it could not consume
     */
    TBStatsEntry (const float& normalized_bucket_value, const double& tokens_left) :
        m_normalized_empty_bucket { normalized_bucket_value },
        m_tokens_left { tokens_left }
    { }

    /**
     * TBStatsEntry parameterized constructor.
     * @param normalized_bucket_value Normalized value that marks when the bucket could not consume
     * @param tokens_left number of tokens left in the bucket when it could not consume
     */
    TBStatsEntry (const float& normalized_bucket_value,
        const double& tokens_left,
        const uint64_t& timestamp) :
        m_normalized_empty_bucket { normalized_bucket_value },
        m_tokens_left { tokens_left },
        m_collection_timestamp { timestamp }
    { }

    /**
     * TBStatsEntry default destructor.
     */
    ~TBStatsEntry () = default;

    /**
     * get_normalized_empty_bucket: get the normalized token value of the bucket
     * @return Returns a copy of the m_normalized_empty_bucket parameter.
     */
    [[nodiscard]] float get_normalized_empty_bucket () const
    {
        return this->m_normalized_empty_bucket;
    }

    /**
     * get_tokens_left: get the total number of tokens left when the bucket could not consume.
     * @return Return a copy of the m_tokens_left parameter.
     */
    [[nodiscard]] double get_tokens_left () const
    {
        return this->m_tokens_left;
    }

    /**
     * get_collection_timestamp: get the time point at which the collection was made.
     * @return Return a copy of the m_collection_timestamp parameter.
     */
    [[nodiscard]] uint64_t get_collection_timestamp () const
    {
        return this->m_collection_timestamp;
    }
};

} // namespace paio::enforcement

#endif // PAIO_TOKEN_BUCKET_STATISTICS_ENTRY_HPP
