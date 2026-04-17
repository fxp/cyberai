xmlParseCharDataInternal(xmlParserCtxtPtr ctxt, int partial) {
    const xmlChar *in;
    int nbchar = 0;
    int line = ctxt->input->line;
    int col = ctxt->input->col;
    int ccol;

    GROW;
    /*
     * Accelerated common case where input don't need to be
     * modified before passing it to the handler.
     */
    in = ctxt->input->cur;
    do {
get_more_space:
        while (*in == 0x20) { in++; ctxt->input->col++; }
        if (*in == 0xA) {
            do {
                ctxt->input->line++; ctxt->input->col = 1;
                in++;
            } while (*in == 0xA);
            goto get_more_space;
        }
        if (*in == '<') {
            nbchar = in - ctxt->input->cur;
            if (nbchar > 0) {
                const xmlChar *tmp = ctxt->input->cur;
                ctxt->input->cur = in;

                if ((ctxt->sax != NULL) &&
                    (ctxt->disableSAX == 0) &&
                    (ctxt->sax->ignorableWhitespace !=
                     ctxt->sax->characters)) {
                    if (areBlanks(ctxt, tmp, nbchar, 1)) {
                        if (ctxt->sax->ignorableWhitespace != NULL)
                            ctxt->sax->ignorableWhitespace(ctxt->userData,
                                                   tmp, nbchar);
                    } else {
                        if (ctxt->sax->characters != NULL)
                            ctxt->sax->characters(ctxt->userData,
                                                  tmp, nbchar);
                        if (*ctxt->space == -1)
                            *ctxt->space = -2;
                    }
                } else if ((ctxt->sax != NULL) &&
                           (ctxt->disableSAX == 0) &&
                           (ctxt->sax->characters != NULL)) {
                    ctxt->sax->characters(ctxt->userData,
                                          tmp, nbchar);
                }
            }
            return;
        }

get_more:
        ccol = ctxt->input->col;
        while (test_char_data[*in]) {
            in++;
            ccol++;
        }
        ctxt->input->col = ccol;
        if (*in == 0xA) {
            do {
                ctxt->input->line++; ctxt->input->col = 1;
                in++;
            } while (*in == 0xA);
            goto get_more;
        }
        if (*in == ']') {
            if ((in[1] == ']') && (in[2] == '>')) {
                xmlFatalErr(ctxt, XML_ERR_MISPLACED_CDATA_END, NULL);
                ctxt->input->cur = in + 1;
                return;
            }
            in++;
            ctxt->input->col++;
            goto get_more;
        }
        nbchar = in - ctxt->input->cur;
        if (nbchar > 0) {
            if ((ctxt->sax != NULL) &&
                (ctxt->disableSAX == 0) &&
                (ctxt->sax->ignorableWhitespace !=
                 ctxt->sax->characters) &&
                (IS_BLANK_CH(*ctxt->input->cur))) {
                const xmlChar *tmp = ctxt->input->cur;
                ctxt->input->cur = in;

                if (areBlanks(ctxt, tmp, nbchar, 0)) {
                    if (ctxt->sax->ignorableWhitespace != NULL)
                        ctxt->sax->ignorableWhitespace(ctxt->userData,
                                                       tmp, nbchar);
                } else {
                    if (ctxt->sax->characters != NULL)
                        ctxt->sax->characters(ctxt->userData,
                                              tmp, nbchar);
                    if (*ctxt->space == -1)
                        *ctxt->space = -2;
                }
                line = ctxt->input->line;
                col = ctxt->input->col;
            } else if ((ctxt->sax != NULL) &&
                       (ctxt->disableSAX == 0)) {
                if (ctxt->sax->characters != NULL)
                    ctxt->sax->characters(ctxt->userData,
                                          ctxt->input->cur, nbchar);
                line = ctxt->input->line;
                col = ctxt->input->col;
            }
        }
        ctxt->input->cur = in;
        if (*in == 0xD) {
            in++;
            if (*in == 0xA) {
                ctxt->input->cur = in;
                in++;
                ctxt->input->line++; ctxt->input->col = 1;
                continue; /* while */
            }
            in--;
        }
        if (*in == '<') {
            return;
        }
        if (*in == '&') {
            return;
        }
        SHRINK;
        GROW;
        in = ctxt->input->cur;
    } while (((*in >= 0x20) && (*in <= 0x7F)) ||
             (*in == 0x09) || (*in == 0x0a));
    ctxt->input->line = line;
    ctxt->input->col = col;
    xmlParseCharDataComplex(ctxt, partial);
