// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 16/24



  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading quantization tables \"%s\" ...",filename);
  table=(QuantizationTable *) NULL;
  xml=FileToString(filename,~0UL,exception);
  if (xml == (char *) NULL)
    return(table);
  quantization_tables=NewXMLTree(xml,exception);
  if (quantization_tables == (XMLTreeInfo *) NULL)
    {
      xml=DestroyString(xml);
      return(table);
    }
  for (table_iterator=GetXMLTreeChild(quantization_tables,"table");
       table_iterator != (XMLTreeInfo *) NULL;
       table_iterator=GetNextXMLTreeTag(table_iterator))
  {
    attribute=GetXMLTreeAttribute(table_iterator,"slot");
    if ((attribute != (char *) NULL) && (LocaleCompare(slot,attribute) == 0))
      break;
    attribute=GetXMLTreeAttribute(table_iterator,"alias");
    if ((attribute != (char *) NULL) && (LocaleCompare(slot,attribute) == 0))
      break;
  }
  if (table_iterator == (XMLTreeInfo *) NULL)
    {
      xml=DestroyString(xml);
      return(table);
    }
  description=GetXMLTreeChild(table_iterator,"description");
  if (description == (XMLTreeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingElement","<description>, slot \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      xml=DestroyString(xml);
      return(table);
    }
  levels=GetXMLTreeChild(table_iterator,"levels");
  if (levels == (XMLTreeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingElement","<levels>, slot \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      xml=DestroyString(xml);
      return(table);
    }
  table=(QuantizationTable *) AcquireCriticalMemory(sizeof(*table));
  table->slot=(char *) NULL;
  table->description=(char *) NULL;
  table->levels=(unsigned int *) NULL;
  attribute=GetXMLTreeAttribute(table_iterator,"slot");
  if (attribute != (char *) NULL)
    table->slot=ConstantString(attribute);
  content=GetXMLTreeContent(description);
  if (content != (char *) NULL)
    table->description=ConstantString(content);
  attribute=GetXMLTreeAttribute(levels,"width");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute","<levels width>, slot \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      table=DestroyQuantizationTable(table);
      xml=DestroyString(xml);
      return(table);
    }
  table->width=StringToUnsignedLong(attribute);
  if (table->width == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
       "XmlInvalidAttribute","<levels width>, table \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      table=DestroyQuantizationTable(table);
      xml=DestroyString(xml);
      return(table);
    }
  attribute=GetXMLTreeAttribute(levels,"height");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute","<levels height>, table \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      table=DestroyQuantizationTable(table);
      xml=DestroyString(xml);
      return(table);
    }
  table->height=StringToUnsignedLong(attribute);
  if (table->height == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidAttribute","<levels height>, table \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      table=DestroyQuantizationTable(table);
      xml=DestroyString(xml);
      return(table);
    }
  attribute=GetXMLTreeAttribute(levels,"divisor");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute","<levels divisor>, table \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      table=DestroyQuantizationTable(table);
      xml=DestroyString(xml);
      return(table);
    }
  table->divisor=InterpretLocaleValue(attribute,(char **) NULL);
  if (table->divisor == 0.0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidAttribute","<levels divisor>, table \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      table=DestroyQuantizationTable(table);
      xml=DestroyString(xml);
      return(table);
    }
  content=GetXMLTreeContent(levels);
  if (content == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingContent","<levels>, table \"%s\"",slot);
      quantization_tables=DestroyXMLTree(quantization_tables);
      table=DestroyQuantizationTable(table);
      xml=DestroyString(xml);
      return(table);
    }
  length=(size_t) table->width*table->height;
  if (length < 64)
    length=64;
  table->levels=(unsig