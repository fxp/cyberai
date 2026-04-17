/* OpenSSL 3.4.1 — ssl3_cbc.c: ssl3_cbc_digest_record A: HMAC setup+lengths — L130-215 */
                           const unsigned char *data,
                           size_t data_size,
                           size_t data_plus_mac_plus_padding_size,
                           const unsigned char *mac_secret,
                           size_t mac_secret_length, char is_sslv3)
{
    union {
        OSSL_UNION_ALIGN;
        unsigned char c[sizeof(LARGEST_DIGEST_CTX)];
    } md_state;
    void (*md_final_raw) (void *ctx, unsigned char *md_out);
    void (*md_transform) (void *ctx, const unsigned char *block);
    size_t md_size, md_block_size = 64;
    size_t sslv3_pad_length = 40, header_length, variance_blocks,
        len, max_mac_bytes, num_blocks,
        num_starting_blocks, k, mac_end_offset, c, index_a, index_b;
    size_t bits;          /* at most 18 bits */
    unsigned char length_bytes[MAX_HASH_BIT_COUNT_BYTES];
    /* hmac_pad is the masked HMAC key. */
    unsigned char hmac_pad[MAX_HASH_BLOCK_SIZE];
    unsigned char first_block[MAX_HASH_BLOCK_SIZE];
    unsigned char mac_out[EVP_MAX_MD_SIZE];
    size_t i, j;
    unsigned md_out_size_u;
    EVP_MD_CTX *md_ctx = NULL;
    /*
     * mdLengthSize is the number of bytes in the length field that
     * terminates * the hash.
     */
    size_t md_length_size = 8;
    char length_is_big_endian = 1;
    int ret = 0;

    /*
     * This is a, hopefully redundant, check that allows us to forget about
     * many possible overflows later in this function.
     */
    if (!ossl_assert(data_plus_mac_plus_padding_size < 1024 * 1024))
        return 0;

    if (EVP_MD_is_a(md, "MD5")) {
#ifdef FIPS_MODULE
        return 0;
#else
        if (MD5_Init((MD5_CTX *)md_state.c) <= 0)
            return 0;
        md_final_raw = tls1_md5_final_raw;
        md_transform =
            (void (*)(void *ctx, const unsigned char *block))MD5_Transform;
        md_size = 16;
        sslv3_pad_length = 48;
        length_is_big_endian = 0;
#endif
    } else if (EVP_MD_is_a(md, "SHA1")) {
        if (SHA1_Init((SHA_CTX *)md_state.c) <= 0)
            return 0;
        md_final_raw = tls1_sha1_final_raw;
        md_transform =
            (void (*)(void *ctx, const unsigned char *block))SHA1_Transform;
        md_size = 20;
    } else if (EVP_MD_is_a(md, "SHA2-224")) {
        if (SHA224_Init((SHA256_CTX *)md_state.c) <= 0)
            return 0;
        md_final_raw = tls1_sha256_final_raw;
        md_transform =
            (void (*)(void *ctx, const unsigned char *block))SHA256_Transform;
        md_size = 224 / 8;
    } else if (EVP_MD_is_a(md, "SHA2-256")) {
        if (SHA256_Init((SHA256_CTX *)md_state.c) <= 0)
            return 0;
        md_final_raw = tls1_sha256_final_raw;
        md_transform =
            (void (*)(void *ctx, const unsigned char *block))SHA256_Transform;
        md_size = 32;
    } else if (EVP_MD_is_a(md, "SHA2-384")) {
        if (SHA384_Init((SHA512_CTX *)md_state.c) <= 0)
            return 0;
        md_final_raw = tls1_sha512_final_raw;
        md_transform =
            (void (*)(void *ctx, const unsigned char *block))SHA512_Transform;
        md_size = 384 / 8;
        md_block_size = 128;
        md_length_size = 16;
    } else if (EVP_MD_is_a(md, "SHA2-512")) {
        if (SHA512_Init((SHA512_CTX *)md_state.c) <= 0)
            return 0;
