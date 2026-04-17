/* OpenSSL 3.4.1 — tasn_dec.c: asn1_find_end + asn1_collect: indefinite length — L979-1110 */
static int asn1_find_end(const unsigned char **in, long len, char inf)
{
    uint32_t expected_eoc;
    long plen;
    const unsigned char *p = *in, *q;
    /* If not indefinite length constructed just add length */
    if (inf == 0) {
        *in += len;
        return 1;
    }
    expected_eoc = 1;
    /*
     * Indefinite length constructed form. Find the end when enough EOCs are
     * found. If more indefinite length constructed headers are encountered
     * increment the expected eoc count otherwise just skip to the end of the
     * data.
     */
    while (len > 0) {
        if (asn1_check_eoc(&p, len)) {
            expected_eoc--;
            if (expected_eoc == 0)
                break;
            len -= 2;
            continue;
        }
        q = p;
        /* Just read in a header: only care about the length */
        if (!asn1_check_tlen(&plen, NULL, NULL, &inf, NULL, &p, len,
                             -1, 0, 0, NULL)) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            return 0;
        }
        if (inf) {
            if (expected_eoc == UINT32_MAX) {
                ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
                return 0;
            }
            expected_eoc++;
        } else {
            p += plen;
        }
        len -= p - q;
    }
    if (expected_eoc) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_MISSING_EOC);
        return 0;
    }
    *in = p;
    return 1;
}

/*
 * This function collects the asn1 data from a constructed string type into
 * a buffer. The values of 'in' and 'len' should refer to the contents of the
 * constructed type and 'inf' should be set if it is indefinite length.
 */

#ifndef ASN1_MAX_STRING_NEST
/*
 * This determines how many levels of recursion are permitted in ASN1 string
 * types. If it is not limited stack overflows can occur. If set to zero no
 * recursion is allowed at all. Although zero should be adequate examples
 * exist that require a value of 1. So 5 should be more than enough.
 */
# define ASN1_MAX_STRING_NEST 5
#endif

static int asn1_collect(BUF_MEM *buf, const unsigned char **in, long len,
                        char inf, int tag, int aclass, int depth)
{
    const unsigned char *p, *q;
    long plen;
    char cst, ininf;
    p = *in;
    inf &= 1;
    /*
     * If no buffer and not indefinite length constructed just pass over the
     * encoded data
     */
    if (!buf && !inf) {
        *in += len;
        return 1;
    }
    while (len > 0) {
        q = p;
        /* Check for EOC */
        if (asn1_check_eoc(&p, len)) {
            /*
             * EOC is illegal outside indefinite length constructed form
             */
            if (!inf) {
                ERR_raise(ERR_LIB_ASN1, ASN1_R_UNEXPECTED_EOC);
                return 0;
            }
            inf = 0;
            break;
        }

        if (!asn1_check_tlen(&plen, NULL, NULL, &ininf, &cst, &p,
                             len, tag, aclass, 0, NULL)) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            return 0;
        }

        /* If indefinite length constructed update max length */
        if (cst) {
            if (depth >= ASN1_MAX_STRING_NEST) {
                ERR_raise(ERR_LIB_ASN1, ASN1_R_NESTED_ASN1_STRING);
                return 0;
            }
            if (!asn1_collect(buf, &p, plen, ininf, tag, aclass, depth + 1))
                return 0;
        } else if (plen && !collect_data(buf, &p, plen))
            return 0;
        len -= p - q;
    }
    if (inf) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_MISSING_EOC);
        return 0;
    }
    *in = p;
    return 1;
}

static int collect_data(BUF_MEM *buf, const unsigned char **p, long plen)
{
    int len;
    if (buf) {
        len = buf->length;
        if (!BUF_MEM_grow_clean(buf, len + plen)) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_BUF_LIB);
            return 0;
