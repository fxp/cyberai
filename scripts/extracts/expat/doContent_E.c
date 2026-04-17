/* doContent E: internal entity push processing */
          parser->m_freeBindingList = b;
          b->prefix->binding = b->prevPrefixBinding;
        }
        if ((parser->m_tagLevel == 0)
            && (parser->m_parsingStatus.parsing != XML_FINISHED)) {
          if (parser->m_parsingStatus.parsing == XML_SUSPENDED)
            parser->m_processor = epilogProcessor;
          else
            return epilogProcessor(parser, next, end, nextPtr);
        }
      }
      break;
    case XML_TOK_CHAR_REF: {
      int n = XmlCharRefNumber(enc, s);
      if (n < 0)
        return XML_ERROR_BAD_CHAR_REF;
      if (parser->m_characterDataHandler) {
        XML_Char buf[XML_ENCODE_MAX];
        parser->m_characterDataHandler(parser->m_handlerArg, buf,
                                       XmlEncode(n, (ICHAR *)buf));
      } else if (parser->m_defaultHandler)
        reportDefault(parser, enc, s, next);
    } break;
    case XML_TOK_XML_DECL:
      return XML_ERROR_MISPLACED_XML_PI;
    case XML_TOK_DATA_NEWLINE:
      if (parser->m_characterDataHandler) {
        XML_Char c = 0xA;
        parser->m_characterDataHandler(parser->m_handlerArg, &c, 1);
      } else if (parser->m_defaultHandler)
        reportDefault(parser, enc, s, next);
      break;
    case XML_TOK_CDATA_SECT_OPEN: {
      enum XML_Error result;
      if (parser->m_startCdataSectionHandler)
        parser->m_startCdataSectionHandler(parser->m_handlerArg);
      /* BEGIN disabled code */
      /* Suppose you doing a transformation on a document that involves
         changing only the character data.  You set up a defaultHandler
         and a characterDataHandler.  The defaultHandler simply copies
         characters through.  The characterDataHandler does the
         transformation and writes the characters out escaping them as
         necessary.  This case will fail to work if we leave out the
         following two lines (because & and < inside CDATA sections will
         be incorrectly escaped).

         However, now we have a start/endCdataSectionHandler, so it seems
         easier to let the user deal with this.
      */
      else if ((0) && parser->m_characterDataHandler)
        parser->m_characterDataHandler(parser->m_handlerArg, parser->m_dataBuf,
                                       0);
      /* END disabled code */
      else if (parser->m_defaultHandler)
        reportDefault(parser, enc, s, next);
      result
          = doCdataSection(parser, enc, &next, end, nextPtr, haveMore, account);
      if (result != XML_ERROR_NONE)
        return result;
      else if (! next) {
        parser->m_processor = cdataSectionProcessor;
        return result;
      }
    } break;
    case XML_TOK_TRAILING_RSQB:
      if (haveMore) {
        *nextPtr = s;
        return XML_ERROR_NONE;
      }
      if (parser->m_characterDataHandler) {
        if (MUST_CONVERT(enc, s)) {
          ICHAR *dataPtr = (ICHAR *)parser->m_dataBuf;
          XmlConvert(enc, &s, end, &dataPtr, (ICHAR *)parser->m_dataBufEnd);
          parser->m_characterDataHandler(
              parser->m_handlerArg, parser->m_dataBuf,
              (int)(dataPtr - (ICHAR *)parser->m_dataBuf));
        } else
          parser->m_characterDataHandler(
              parser->m_handlerArg, (const XML_Char *)s,
              (int)((const XML_Char *)end - (const XML_Char *)s));
