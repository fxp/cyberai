/* OpenSSL 3.4.1 — tasn_dec.c: asn1_ex_c2i: content byte decode + int conversion — L831-980 */
static int asn1_ex_c2i(ASN1_VALUE **pval, const unsigned char *cont, int len,
                       int utype, char *free_cont, const ASN1_ITEM *it)
{
    ASN1_VALUE **opval = NULL;
    ASN1_STRING *stmp;
    ASN1_TYPE *typ = NULL;
    int ret = 0;
    const ASN1_PRIMITIVE_FUNCS *pf;
    ASN1_INTEGER **tint;
    pf = it->funcs;

    if (pf && pf->prim_c2i)
        return pf->prim_c2i(pval, cont, len, utype, free_cont, it);
    /* If ANY type clear type and set pointer to internal value */
    if (it->utype == V_ASN1_ANY) {
        if (*pval == NULL) {
            typ = ASN1_TYPE_new();
            if (typ == NULL)
                goto err;
            *pval = (ASN1_VALUE *)typ;
        } else
            typ = (ASN1_TYPE *)*pval;

        if (utype != typ->type)
            ASN1_TYPE_set(typ, utype, NULL);
        opval = pval;
        pval = &typ->value.asn1_value;
    }
    switch (utype) {
    case V_ASN1_OBJECT:
        if (!ossl_c2i_ASN1_OBJECT((ASN1_OBJECT **)pval, &cont, len))
            goto err;
        break;

    case V_ASN1_NULL:
        if (len) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_NULL_IS_WRONG_LENGTH);
            goto err;
        }
        *pval = (ASN1_VALUE *)1;
        break;

    case V_ASN1_BOOLEAN:
        if (len != 1) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_BOOLEAN_IS_WRONG_LENGTH);
            goto err;
        } else {
            ASN1_BOOLEAN *tbool;
            tbool = (ASN1_BOOLEAN *)pval;
            *tbool = *cont;
        }
        break;

    case V_ASN1_BIT_STRING:
        if (!ossl_c2i_ASN1_BIT_STRING((ASN1_BIT_STRING **)pval, &cont, len))
            goto err;
        break;

    case V_ASN1_INTEGER:
    case V_ASN1_ENUMERATED:
        tint = (ASN1_INTEGER **)pval;
        if (!ossl_c2i_ASN1_INTEGER(tint, &cont, len))
            goto err;
        /* Fixup type to match the expected form */
        (*tint)->type = utype | ((*tint)->type & V_ASN1_NEG);
        break;

    case V_ASN1_OCTET_STRING:
    case V_ASN1_NUMERICSTRING:
    case V_ASN1_PRINTABLESTRING:
    case V_ASN1_T61STRING:
    case V_ASN1_VIDEOTEXSTRING:
    case V_ASN1_IA5STRING:
    case V_ASN1_UTCTIME:
    case V_ASN1_GENERALIZEDTIME:
    case V_ASN1_GRAPHICSTRING:
    case V_ASN1_VISIBLESTRING:
    case V_ASN1_GENERALSTRING:
    case V_ASN1_UNIVERSALSTRING:
    case V_ASN1_BMPSTRING:
    case V_ASN1_UTF8STRING:
    case V_ASN1_OTHER:
    case V_ASN1_SET:
    case V_ASN1_SEQUENCE:
    default:
        if (utype == V_ASN1_BMPSTRING && (len & 1)) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_BMPSTRING_IS_WRONG_LENGTH);
            goto err;
        }
        if (utype == V_ASN1_UNIVERSALSTRING && (len & 3)) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_UNIVERSALSTRING_IS_WRONG_LENGTH);
            goto err;
        }
        if (utype == V_ASN1_GENERALIZEDTIME && (len < 15)) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_GENERALIZEDTIME_IS_TOO_SHORT);
            goto err;
        }
        if (utype == V_ASN1_UTCTIME && (len < 13)) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_UTCTIME_IS_TOO_SHORT);
            goto err;
        }
        /* All based on ASN1_STRING and handled the same */
        if (*pval == NULL) {
            stmp = ASN1_STRING_type_new(utype);
            if (stmp == NULL) {
                ERR_raise(ERR_LIB_ASN1, ERR_R_ASN1_LIB);
                goto err;
            }
            *pval = (ASN1_VALUE *)stmp;
        } else {
            stmp = (ASN1_STRING *)*pval;
            stmp->type = utype;
        }
        /* If we've already allocated a buffer use it */
        if (*free_cont) {
            ASN1_STRING_set0(stmp, (unsigned char *)cont /* UGLY CAST! */, len);
            *free_cont = 0;
        } else {
            if (!ASN1_STRING_set(stmp, cont, len)) {
                ERR_raise(ERR_LIB_ASN1, ERR_R_ASN1_LIB);
                ASN1_STRING_free(stmp);
                *pval = NULL;
                goto err;
            }
        }
        break;
    }
    /* If ASN1_ANY and NULL type fix up value */
    if (typ && (utype == V_ASN1_NULL))
        typ->value.ptr = NULL;

    ret = 1;
 err:
    if (!ret) {
        ASN1_TYPE_free(typ);
        if (opval)
            *opval = NULL;
    }
    return ret;
}

/*
 * This function finds the end of an ASN1 structure when passed its maximum
 * length, whether it is indefinite length and a pointer to the content. This
 * is more efficient than calling asn1_collect because it does not recurse on
 * each indefinite length header.
 */

static int asn1_find_end(const unsigned char **in, long len, char inf)
{
