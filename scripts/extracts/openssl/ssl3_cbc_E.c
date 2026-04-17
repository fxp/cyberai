/* OpenSSL 3.4.1 — ssl3_cbc.c: ssl3_cbc_digest_record C: padding verify — L320-390 */
     */
    if (num_blocks > variance_blocks + (is_sslv3 ? 1 : 0)) {
        num_starting_blocks = num_blocks - variance_blocks;
        k = md_block_size * num_starting_blocks;
    }

    bits = 8 * mac_end_offset;
    if (!is_sslv3) {
        /*
         * Compute the initial HMAC block. For SSLv3, the padding and secret
         * bytes are included in |header| because they take more than a
         * single block.
         */
        bits += 8 * md_block_size;
        memset(hmac_pad, 0, md_block_size);
        if (!ossl_assert(mac_secret_length <= sizeof(hmac_pad)))
            return 0;
        memcpy(hmac_pad, mac_secret, mac_secret_length);
        for (i = 0; i < md_block_size; i++)
            hmac_pad[i] ^= 0x36;

        md_transform(md_state.c, hmac_pad);
    }

    if (length_is_big_endian) {
        memset(length_bytes, 0, md_length_size - 4);
        length_bytes[md_length_size - 4] = (unsigned char)(bits >> 24);
        length_bytes[md_length_size - 3] = (unsigned char)(bits >> 16);
        length_bytes[md_length_size - 2] = (unsigned char)(bits >> 8);
        length_bytes[md_length_size - 1] = (unsigned char)bits;
    } else {
        memset(length_bytes, 0, md_length_size);
        length_bytes[md_length_size - 5] = (unsigned char)(bits >> 24);
        length_bytes[md_length_size - 6] = (unsigned char)(bits >> 16);
        length_bytes[md_length_size - 7] = (unsigned char)(bits >> 8);
        length_bytes[md_length_size - 8] = (unsigned char)bits;
    }

    if (k > 0) {
        if (is_sslv3) {
            size_t overhang;

            /*
             * The SSLv3 header is larger than a single block. overhang is
             * the number of bytes beyond a single block that the header
             * consumes: either 7 bytes (SHA1) or 11 bytes (MD5). There are no
             * ciphersuites in SSLv3 that are not SHA1 or MD5 based and
             * therefore we can be confident that the header_length will be
             * greater than |md_block_size|. However we add a sanity check just
             * in case
             */
            if (header_length <= md_block_size) {
                /* Should never happen */
                return 0;
            }
            overhang = header_length - md_block_size;
            md_transform(md_state.c, header);
            memcpy(first_block, header + md_block_size, overhang);
            memcpy(first_block + overhang, data, md_block_size - overhang);
            md_transform(md_state.c, first_block);
            for (i = 1; i < k / md_block_size - 1; i++)
                md_transform(md_state.c, data + md_block_size * i - overhang);
        } else {
            /* k is a multiple of md_block_size. */
            memcpy(first_block, header, 13);
            memcpy(first_block + 13, data, md_block_size - 13);
            md_transform(md_state.c, first_block);
            for (i = 1; i < k / md_block_size; i++)
                md_transform(md_state.c, data + md_block_size * i - 13);
        }
    }
