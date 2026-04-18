// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 19/39

value,"true") == 0);
              break;
            }
          if (LocaleCompare(keyword,"transform") == 0)
            {
              AffineMatrix
                affine,
                current,
                transform;

              GetAffineMatrix(&transform);
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  ");
              tokens=SVGKeyValuePairs(svg_info,'(',')',value,&number_tokens);
              if (tokens == (char **) NULL)
                break;
              for (j=0; j < ((ssize_t) number_tokens-1); j+=2)
              {
                char
                  *token_value;

                keyword=(char *) tokens[j];
                token_value=(char *) tokens[j+1];
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "    %s: %s",keyword,token_value);
                current=transform;
                GetAffineMatrix(&affine);
                switch (*keyword)
                {
                  case 'M':
                  case 'm':
                  {
                    if (LocaleCompare(keyword,"matrix") == 0)
                      {
                        p=token_value;
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        affine.sx=StringToDouble(token_value,(char **) NULL);
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        if (*token == ',')
                          (void) GetNextToken(p,&p,MagickPathExtent,token);
                        affine.rx=StringToDouble(token,&next_token);
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        if (*token == ',')
                          (void) GetNextToken(p,&p,MagickPathExtent,token);
                        affine.ry=StringToDouble(token,&next_token);
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        if (*token == ',')
                          (void) GetNextToken(p,&p,MagickPathExtent,token);
                        affine.sy=StringToDouble(token,&next_token);
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        if (*token == ',')
                          (void) GetNextToken(p,&p,MagickPathExtent,token);
                        affine.tx=StringToDouble(token,&next_token);
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        if (*token == ',')
                          (void) GetNextToken(p,&p,MagickPathExtent,token);
                        affine.ty=StringToDouble(token,&next_token);
                        break;
                      }
                    break;
                  }
                  case 'R':
                  case 'r':
                  {
                    if (LocaleCompare(keyword,"rotate") == 0)
                      {
                        double
                          angle,
                          x,
                          y;