/* OpenSSL 3.4.1 — tasn_dec.c: asn1_item_embed_d2i A: SEQUENCE decode — L110-220 */
                                   OSSL_LIB_CTX *libctx, const char *propq)
{
    int rv;

    if (pval == NULL || it == NULL) {
        ERR_raise(ERR_LIB_ASN1, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }
    rv = asn1_item_embed_d2i(pval, in, len, it, tag, aclass, opt, ctx, 0,
                             libctx, propq);
    if (rv <= 0)
        ASN1_item_ex_free(pval, it);
    return rv;
}

int ASN1_item_ex_d2i(ASN1_VALUE **pval, const unsigned char **in, long len,
                     const ASN1_ITEM *it,
                     int tag, int aclass, char opt, ASN1_TLC *ctx)
{
    return asn1_item_ex_d2i_intern(pval, in, len, it, tag, aclass, opt, ctx,
                                   NULL, NULL);
}

ASN1_VALUE *ASN1_item_d2i_ex(ASN1_VALUE **pval,
                             const unsigned char **in, long len,
                             const ASN1_ITEM *it, OSSL_LIB_CTX *libctx,
                             const char *propq)
{
    ASN1_TLC c;
    ASN1_VALUE *ptmpval = NULL;

    if (pval == NULL)
        pval = &ptmpval;
    asn1_tlc_clear_nc(&c);
    if (asn1_item_ex_d2i_intern(pval, in, len, it, -1, 0, 0, &c, libctx,
                                propq) > 0)
        return *pval;
    return NULL;
}

ASN1_VALUE *ASN1_item_d2i(ASN1_VALUE **pval,
                          const unsigned char **in, long len,
                          const ASN1_ITEM *it)
{
    return ASN1_item_d2i_ex(pval, in, len, it, NULL, NULL);
}

/*
 * Decode an item, taking care of IMPLICIT tagging, if any. If 'opt' set and
 * tag mismatch return -1 to handle OPTIONAL
 */

static int asn1_item_embed_d2i(ASN1_VALUE **pval, const unsigned char **in,
                               long len, const ASN1_ITEM *it,
                               int tag, int aclass, char opt, ASN1_TLC *ctx,
                               int depth, OSSL_LIB_CTX *libctx,
                               const char *propq)
{
    const ASN1_TEMPLATE *tt, *errtt = NULL;
    const ASN1_EXTERN_FUNCS *ef;
    const ASN1_AUX *aux;
    ASN1_aux_cb *asn1_cb;
    const unsigned char *p = NULL, *q;
    unsigned char oclass;
    char seq_eoc, seq_nolen, cst, isopt;
    long tmplen;
    int i;
    int otag;
    int ret = 0;
    ASN1_VALUE **pchptr;

    if (pval == NULL || it == NULL) {
        ERR_raise(ERR_LIB_ASN1, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }
    if (len <= 0) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_TOO_SMALL);
        return 0;
    }
    aux = it->funcs;
    if (aux && aux->asn1_cb)
        asn1_cb = aux->asn1_cb;
    else
        asn1_cb = 0;

    if (++depth > ASN1_MAX_CONSTRUCTED_NEST) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_NESTED_TOO_DEEP);
        goto err;
    }

    switch (it->itype) {
    case ASN1_ITYPE_PRIMITIVE:
        if (it->templates) {
            /*
             * tagging or OPTIONAL is currently illegal on an item template
             * because the flags can't get passed down. In practice this
             * isn't a problem: we include the relevant flags from the item
             * template in the template itself.
             */
            if ((tag != -1) || opt) {
                ERR_raise(ERR_LIB_ASN1,
                          ASN1_R_ILLEGAL_OPTIONS_ON_ITEM_TEMPLATE);
                goto err;
            }
            return asn1_template_ex_d2i(pval, in, len, it->templates, opt, ctx,
                                        depth, libctx, propq);
        }
        return asn1_d2i_ex_primitive(pval, in, len, it,
                                     tag, aclass, opt, ctx);

    case ASN1_ITYPE_MSTRING:
