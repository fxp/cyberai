/* OpenSSL 3.4.1 — ssl3_cbc.c: ssl3_cbc_digest_record B: MAC block hashing — L215-320 */
            return 0;
        md_final_raw = tls1_sha512_final_raw;
        md_transform =
            (void (*)(void *ctx, const unsigned char *block))SHA512_Transform;
        md_size = 64;
        md_block_size = 128;
        md_length_size = 16;
    } else {
        /*
         * ssl3_cbc_record_digest_supported should have been called first to
         * check that the hash function is supported.
         */
        if (md_out_size != NULL)
            *md_out_size = 0;
        return ossl_assert(0);
    }

    if (!ossl_assert(md_length_size <= MAX_HASH_BIT_COUNT_BYTES)
            || !ossl_assert(md_block_size <= MAX_HASH_BLOCK_SIZE)
            || !ossl_assert(md_size <= EVP_MAX_MD_SIZE))
        return 0;

    header_length = 13;
    if (is_sslv3) {
        header_length = mac_secret_length
                        + sslv3_pad_length
                        + 8  /* sequence number */
                        + 1  /* record type */
                        + 2; /* record length */
    }

    /*
     * variance_blocks is the number of blocks of the hash that we have to
     * calculate in constant time because they could be altered by the
     * padding value. In SSLv3, the padding must be minimal so the end of
     * the plaintext varies by, at most, 15+20 = 35 bytes. (We conservatively
     * assume that the MAC size varies from 0..20 bytes.) In case the 9 bytes
     * of hash termination (0x80 + 64-bit length) don't fit in the final
     * block, we say that the final two blocks can vary based on the padding.
     * TLSv1 has MACs up to 48 bytes long (SHA-384) and the padding is not
     * required to be minimal. Therefore we say that the final |variance_blocks|
     * blocks can
     * vary based on the padding. Later in the function, if the message is
     * short and there obviously cannot be this many blocks then
     * variance_blocks can be reduced.
     */
    variance_blocks = is_sslv3 ? 2
                               : (((255 + 1 + md_size + md_block_size - 1)
                                   / md_block_size) + 1);
    /*
     * From now on we're dealing with the MAC, which conceptually has 13
     * bytes of `header' before the start of the data (TLS) or 71/75 bytes
     * (SSLv3)
     */
    len = data_plus_mac_plus_padding_size + header_length;
    /*
     * max_mac_bytes contains the maximum bytes of bytes in the MAC,
     * including * |header|, assuming that there's no padding.
     */
    max_mac_bytes = len - md_size - 1;
    /* num_blocks is the maximum number of hash blocks. */
    num_blocks =
        (max_mac_bytes + 1 + md_length_size + md_block_size -
         1) / md_block_size;
    /*
     * In order to calculate the MAC in constant time we have to handle the
     * final blocks specially because the padding value could cause the end
     * to appear somewhere in the final |variance_blocks| blocks and we can't
     * leak where. However, |num_starting_blocks| worth of data can be hashed
     * right away because no padding value can affect whether they are
     * plaintext.
     */
    num_starting_blocks = 0;
    /*
     * k is the starting byte offset into the conceptual header||data where
     * we start processing.
     */
    k = 0;
    /*
     * mac_end_offset is the index just past the end of the data to be MACed.
     */
    mac_end_offset = data_size + header_length;
    /*
     * c is the index of the 0x80 byte in the final hash block that contains
     * application data.
     */
    c = mac_end_offset % md_block_size;
    /*
     * index_a is the hash block number that contains the 0x80 terminating
     * value.
     */
    index_a = mac_end_offset / md_block_size;
    /*
     * index_b is the hash block number that contains the 64-bit hash length,
     * in bits.
     */
    index_b = (mac_end_offset + md_length_size) / md_block_size;
    /*
     * bits is the hash-length in bits. It includes the additional hash block
     * for the masked HMAC key, or whole of |header| in the case of SSLv3.
     */

    /*
     * For SSLv3, if we're going to have any starting blocks then we need at
     * least two because the header is larger than a single block.
     */
