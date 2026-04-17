    while (*cur != '\0') {
        size_t indx = out - buffer;
        if (indx + 100 > buffer_size) {

	    growBufferReentrant();
	    out = &buffer[indx];
	}

	/*
	 * By default one have to encode at least '<', '>', '"' and '&' !
	 */
	if (*cur == '<') {
	    const xmlChar *end;

	    /*
	     * Special handling of server side include in HTML attributes
	     */
	    if (html && attr &&
	        (cur[1] == '!') && (cur[2] == '-') && (cur[3] == '-') &&
	        ((end = xmlStrstr(cur, BAD_CAST "-->")) != NULL)) {
	        while (cur != end) {
		    *out++ = *cur++;
		    indx = out - buffer;
		    if (indx + 100 > buffer_size) {
			growBufferReentrant();
			out = &buffer[indx];
		    }
		}
		*out++ = *cur++;
		*out++ = *cur++;
		*out++ = *cur++;
		continue;
	    }
	    *out++ = '&';
	    *out++ = 'l';
	    *out++ = 't';
	    *out++ = ';';
	} else if (*cur == '>') {
	    *out++ = '&';
	    *out++ = 'g';
	    *out++ = 't';
	    *out++ = ';';
	} else if (*cur == '&') {
	    /*
	     * Special handling of &{...} construct from HTML 4, see
	     * http://www.w3.org/TR/html401/appendix/notes.html#h-B.7.1
	     */
	    if (html && attr && (cur[1] == '{') &&
	        (strchr((const char *) cur, '}'))) {
	        while (*cur != '}') {
		    *out++ = *cur++;
		    indx = out - buffer;
		    if (indx + 100 > buffer_size) {
			growBufferReentrant();
			out = &buffer[indx];
		    }
		}
		*out++ = *cur++;
		continue;
	    }
	    *out++ = '&';
	    *out++ = 'a';
	    *out++ = 'm';
	    *out++ = 'p';
	    *out++ = ';';
	} else if (((*cur >= 0x20) && (*cur < 0x80)) ||
	    (*cur == '\n') || (*cur == '\t') || ((html) && (*cur == '\r'))) {
	    /*
	     * default case, just copy !
	     */
	    *out++ = *cur;
	} else if (*cur >= 0x80) {
	    if (((doc != NULL) && (doc->encoding != NULL)) || (html)) {
		/*
		 * Bjørn Reese <br@sseusa.com> provided the patch
	        xmlChar xc;
	        xc = (*cur & 0x3F) << 6;
	        if (cur[1] != 0) {
		    xc += *(++cur) & 0x3F;
		    *out++ = xc;
	        } else
		 */
		*out++ = *cur;
	    } else {
		/*
		 * We assume we have UTF-8 input.
		 * It must match either:
		 *   110xxxxx 10xxxxxx
		 *   1110xxxx 10xxxxxx 10xxxxxx
		 *   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		 * That is:
		 *   cur[0] is 11xxxxxx
		 *   cur[1] is 10xxxxxx
		 *   cur[2] is 10xxxxxx if cur[0] is 111xxxxx
		 *   cur[3] is 10xxxxxx if cur[0] is 1111xxxx
		 *   cur[0] is not 11111xxx
		 */
		char buf[13], *ptr;
		int val, l;

                l = 4;
                val = xmlGetUTF8Char(cur, &l);
                if (val < 0) {
                    val = 0xFFFD;
                    cur++;
                } else {
                    if (!IS_CHAR(val))
                        val = 0xFFFD;
                    cur += l;
		}
		/*
		 * We could do multiple things here. Just save as a char ref
		 */
		snprintf(buf, sizeof(buf), "&#x%X;", val);
		buf[sizeof(buf) - 1] = 0;
		ptr = buf;
		while (*ptr != 0) *out++ = *ptr++;
		continue;
	    }
	} else if (IS_BYTE_CHAR(*cur)) {
	    char buf[11], *ptr;

	    snprintf(buf, sizeof(buf), "&#%d;", *cur);
	    buf[sizeof(buf) - 1] = 0;
            ptr = buf;
	    while (*ptr != 0) *out++ = *ptr++;
	}
	cur++;
    }
    *out = 0;
    return(buffer);

mem_error:
    xmlFree(buffer);
    return(NULL);
}

/**
 * xmlEncodeAttributeEntities:
 * @doc:  the document containing the string
 * @input:  A string to convert to XML.
 *
 * Do a global encoding of a string, replacing the predefined entities
 * and non ASCII values with their entities and CharRef counterparts for
 * attribute values.
 *
 * Returns A newly allocated string with the substitution done.
 */
xmlChar *
xmlEncodeAttributeEntities(xmlDocPtr doc, const xmlChar *input) {
    return xmlEncodeEntitiesInternal(doc, input, 1);
}

/**
 * xmlEncodeEntitiesReentrant:
 * @doc:  the document containing the string
 * @input:  A string to convert to XML.
 *
 * Do a global encoding of a string, replacing the predefined entities
 * and non ASCII values with their entities and CharRef counterparts.
 * Contrary to xmlEncodeEntities, this routine is reentrant, and result
 * must be deallocated.
 *
 * Returns A newly allocated string with the substitution done.
