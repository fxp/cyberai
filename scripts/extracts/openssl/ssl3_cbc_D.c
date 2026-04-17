/* OpenSSL 3.4.1 — ssl3_cbc.c: ssl3_cbc_digest_record C: output + constant-time ops — L390-486 */
    }

    memset(mac_out, 0, sizeof(mac_out));

    /*
     * We now process the final hash blocks. For each block, we construct it
     * in constant time. If the |i==index_a| then we'll include the 0x80
     * bytes and zero pad etc. For each block we selectively copy it, in
     * constant time, to |mac_out|.
     */
    for (i = num_starting_blocks; i <= num_starting_blocks + variance_blocks;
         i++) {
        unsigned char block[MAX_HASH_BLOCK_SIZE];
        unsigned char is_block_a = constant_time_eq_8_s(i, index_a);
        unsigned char is_block_b = constant_time_eq_8_s(i, index_b);

        for (j = 0; j < md_block_size; j++) {
            unsigned char b = 0, is_past_c, is_past_cp1;

            if (k < header_length)
                b = header[k];
            else if (k < data_plus_mac_plus_padding_size + header_length)
                b = data[k - header_length];
            k++;

            is_past_c = is_block_a & constant_time_ge_8_s(j, c);
            is_past_cp1 = is_block_a & constant_time_ge_8_s(j, c + 1);
            /*
             * If this is the block containing the end of the application
             * data, and we are at the offset for the 0x80 value, then
             * overwrite b with 0x80.
             */
            b = constant_time_select_8(is_past_c, 0x80, b);
            /*
             * If this block contains the end of the application data
             * and we're past the 0x80 value then just write zero.
             */
            b = b & ~is_past_cp1;
            /*
             * If this is index_b (the final block), but not index_a (the end
             * of the data), then the 64-bit length didn't fit into index_a
             * and we're having to add an extra block of zeros.
             */
            b &= ~is_block_b | is_block_a;

            /*
             * The final bytes of one of the blocks contains the length.
             */
            if (j >= md_block_size - md_length_size) {
                /* If this is index_b, write a length byte. */
                b = constant_time_select_8(is_block_b,
                                           length_bytes[j -
                                                        (md_block_size -
                                                         md_length_size)], b);
            }
            block[j] = b;
        }

        md_transform(md_state.c, block);
        md_final_raw(md_state.c, block);
        /* If this is index_b, copy the hash value to |mac_out|. */
        for (j = 0; j < md_size; j++)
            mac_out[j] |= block[j] & is_block_b;
    }

    md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL)
        goto err;

    if (EVP_DigestInit_ex(md_ctx, md, NULL /* engine */) <= 0)
        goto err;
    if (is_sslv3) {
        /* We repurpose |hmac_pad| to contain the SSLv3 pad2 block. */
        memset(hmac_pad, 0x5c, sslv3_pad_length);

        if (EVP_DigestUpdate(md_ctx, mac_secret, mac_secret_length) <= 0
            || EVP_DigestUpdate(md_ctx, hmac_pad, sslv3_pad_length) <= 0
            || EVP_DigestUpdate(md_ctx, mac_out, md_size) <= 0)
            goto err;
    } else {
        /* Complete the HMAC in the standard manner. */
        for (i = 0; i < md_block_size; i++)
            hmac_pad[i] ^= 0x6a;

        if (EVP_DigestUpdate(md_ctx, hmac_pad, md_block_size) <= 0
            || EVP_DigestUpdate(md_ctx, mac_out, md_size) <= 0)
            goto err;
    }
    ret = EVP_DigestFinal(md_ctx, md_out, &md_out_size_u);
    if (ret && md_out_size)
        *md_out_size = md_out_size_u;

    ret = 1;
 err:
    EVP_MD_CTX_free(md_ctx);
    return ret;
}
