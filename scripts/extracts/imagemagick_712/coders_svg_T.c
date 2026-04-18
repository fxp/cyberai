// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 20/39



                        p=token_value;
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        angle=StringToDouble(token_value,(char **) NULL);
                        affine.sx=cos(DegreesToRadians(fmod(angle,360.0)));
                        affine.rx=sin(DegreesToRadians(fmod(angle,360.0)));
                        affine.ry=(-sin(DegreesToRadians(fmod(angle,360.0))));
                        affine.sy=cos(DegreesToRadians(fmod(angle,360.0)));
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        if (*token == ',')
                          (void) GetNextToken(p,&p,MagickPathExtent,token);
                        x=StringToDouble(token,&next_token);
                        (void) GetNextToken(p,&p,MagickPathExtent,token);
                        if (*token == ',')
                          (void) GetNextToken(p,&p,MagickPathExtent,token);
                        y=StringToDouble(token,&next_token);
                        y=StringToDouble(token,&next_token);
                        affine.tx=(-1.0*(svg_info->bounds.x+x*
                          cos(DegreesToRadians(fmod(angle,360.0)))-y*
                          sin(DegreesToRadians(fmod(angle,360.0)))))+x;
                        affine.ty=(-1.0*(svg_info->bounds.y+x*
                          sin(DegreesToRadians(fmod(angle,360.0)))+y*
                          cos(DegreesToRadians(fmod(angle,360.0)))))+y;
                        break;
                      }
                    break;
                  }
                  case 'S':
                  case 's':
                  {
                    if (LocaleCompare(keyword,"scale") == 0)
                      {
                        for (p=token_value; *p != '\0'; p++)
                          if ((isspace((int) ((unsigned char) *p)) != 0) ||
                              (*p == ','))
                            break;
                        affine.sx=GetUserSpaceCoordinateValue(svg_info,1,token_value);
                        affine.sy=affine.sx;
                        if (*p != '\0')
                          affine.sy=GetUserSpaceCoordinateValue(svg_info,-1,
                            p+1);
                        svg_info->scale[svg_info->n]=ExpandAffine(&affine);
                        break;
                      }
                    if (LocaleCompare(keyword,"skewX") == 0)
                      {
                        affine.sx=svg_info->affine.sx;
                        affine.ry=tan(DegreesToRadians(fmod(
                          GetUserSpaceCoordinateValue(svg_info,1,token_value),
                          360.0)));
                        affine.sy=svg_info->affine.sy;
                        break;
                      }
                    if (LocaleCompare(keyword,"skewY") == 0)
                      {
                        affine.sx=svg_info->affine.sx;
                        affine.rx=tan(DegreesToRadians(fmod(
                          GetUserSpaceCoordinateValue(svg_info,-1,token_value),
                          360.0)));
                        affine.sy=svg_info->affine.sy;
                        break;
                      }
                    break;
                  }
                  case 'T':
                  case 't':
                  {
                    if (LocaleCompare(keyword,"translate") == 0)
                      {
                        for (p=token_value; *p != '\0'; p++)
                          if ((isspace((int) ((unsigned char) *p)) != 0) ||
                              (*p == ','))
                            break;
                        affine.tx=GetUserSpaceCoordinateValue(svg_info,1,token_value);
                        affine.ty=0;
                        if (*p != '\0')
                          affine.ty=GetUserSpaceCoordinateValue(svg_info,-1,
                            p+1);
                        break;
                      }
                    break;
                  }
                  default:
                    break;
                }
                transform.sx=affine.sx*current.sx+affine.ry*current.rx;
                transform.rx=affine.rx*current.sx+affine.sy*current.rx;
                transform.ry=affine.sx*current.ry+affine.ry*current.sy;
                transform.sy=affine.rx*current.ry+affine.sy*current.sy;
                transform.tx=affine.tx*current.sx+affine.ty*current.ry+
                  current.tx;
                transform.ty=affine.tx*current.rx+affine.ty*current.sy+
                  current.ty;
              }
              (void) FormatLocaleFile(svg_info->file,
                "affine %g %g %g %g %g %g\n",transform.sx,transform.rx,
                transform.ry,transform.sy,transform.tx,transform.ty);
              for (j=0; tokens[j] != (char *) NULL; j++)
                tokens[j]=DestroyString(tokens[j]);
              tokens=(char **) RelinquishMagickMemory(tokens);
              break;
       