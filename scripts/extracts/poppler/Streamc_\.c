// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 28/36



static const FlateCode flateFixedLitCodeTabCodes[512] = {
    { .len = 7, .val = 0x0100 }, { .len = 8, .val = 0x0050 }, { .len = 8, .val = 0x0010 }, { .len = 8, .val = 0x0118 }, { .len = 7, .val = 0x0110 }, { .len = 8, .val = 0x0070 }, { .len = 8, .val = 0x0030 }, { .len = 9, .val = 0x00c0 },
    { .len = 7, .val = 0x0108 }, { .len = 8, .val = 0x0060 }, { .len = 8, .val = 0x0020 }, { .len = 9, .val = 0x00a0 }, { .len = 8, .val = 0x0000 }, { .len = 8, .val = 0x0080 }, { .len = 8, .val = 0x0040 }, { .len = 9, .val = 0x00e0 },
    { .len = 7, .val = 0x0104 }, { .len = 8, .val = 0x0058 }, { .len = 8, .val = 0x0018 }, { .len = 9, .val = 0x0090 }, { .len = 7, .val = 0x0114 }, { .len = 8, .val = 0x0078 }, { .len = 8, .val = 0x0038 }, { .len = 9, .val = 0x00d0 },
    { .len = 7, .val = 0x010c }, { .len = 8, .val = 0x0068 }, { .len = 8, .val = 0x0028 }, { .len = 9, .val = 0x00b0 }, { .len = 8, .val = 0x0008 }, { .len = 8, .val = 0x0088 }, { .len = 8, .val = 0x0048 }, { .len = 9, .val = 0x00f0 },
    { .len = 7, .val = 0x0102 }, { .len = 8, .val = 0x0054 }, { .len = 8, .val = 0x0014 }, { .len = 8, .val = 0x011c }, { .len = 7, .val = 0x0112 }, { .len = 8, .val = 0x0074 }, { .len = 8, .val = 0x0034 }, { .len = 9, .val = 0x00c8 },
    { .len = 7, .val = 0x010a }, { .len = 8, .val = 0x0064 }, { .len = 8, .val = 0x0024 }, { .len = 9, .val = 0x00a8 }, { .len = 8, .val = 0x0004 }, { .len = 8, .val = 0x0084 }, { .len = 8, .val = 0x0044 }, { .len = 9, .val = 0x00e8 },
    { .len = 7, .val = 0x0106 }, { .len = 8, .val = 0x005c }, { .len = 8, .val = 0x001c }, { .len = 9, .val = 0x0098 }, { .len = 7, .val = 0x0116 }, { .len = 8, .val = 0x007c }, { .len = 8, .val = 0x003c }, { .len = 9, .val = 0x00d8 },
    { .len = 7, .val = 0x010e }, { .len = 8, .val = 0x006c }, { .len = 8, .val = 0x002c }, { .len = 9, .val = 0x00b8 }, { .len = 8, .val = 0x000c }, { .len = 8, .val = 0x008c }, { .len = 8, .val = 0x004c }, { .len = 9, .val = 0x00f8 },
    { .len = 7, .val = 0x0101 }, { .len = 8, .val = 0x0052 }, { .len = 8, .val = 0x0012 }, { .len = 8, .val = 0x011a }, { .len = 7, .val = 0x0111 }, { .len = 8, .val = 0x0072 }, { .len = 8, .val = 0x0032 }, { .len = 9, .val = 0x00c4 },
    { .len = 7, .val = 0x0109 }, { .len = 8, .val = 0x0062 }, { .len = 8, .val = 0x0022 }, { .len = 9, .val = 0x00a4 }, { .len = 8, .val = 0x0002 }, { .len = 8, .val = 0x0082 }, { .len = 8, .val = 0x0042 }, { .len = 9, .val = 0x00e4 },
    { .len = 7, .val = 0x0105 }, { .len = 8, .val = 0x005a }, { .len = 8, .val = 0x001a }, { .len = 9, .val = 0x0094 }, { .len = 7, .val = 0x0115 }, { .len = 8, .val = 0x007a }, { .len = 8, .val = 0x003a }, { .len = 9, .val = 0x00d4 },
    { .len = 7, .val = 0x010d }, { .len = 8, .val = 0x006a }, { .len = 8, .val = 0x002a }, { .len = 9, .val = 0x00b4 }, { .len = 8, .val = 0x000a }, { .len = 8, .val = 0x008a }, { .len = 8, .val = 0x004a }, { .len = 9, .val = 0x00f4 },
    { .len = 7, .val = 0x0103 }, { .len = 8, .val = 0x0056 }, { .len = 8, .val = 0x0016 }, { .len = 8, .val = 0x011e }, { .len = 7, .val = 0x0113 }, { .len = 8, .val = 0x0076 }, { .len = 8, .val = 0x0036 }, { .len = 9, .val = 0x00cc },
    { .len = 7, .val = 0x010b }, { .len = 8, .val = 0x0066 }, { .len = 8, .val = 0x0026 }, { .len = 9, .val = 0x00ac }, { .len = 8, .val = 0x0006 }, { .len = 8, .val = 0x0086 }, { .len = 8, .val = 0x0046 }, { .len = 9, .val = 0x00ec },
    { .len = 7, .val = 0x0107 }, { .len = 8, .val = 0x005e }, { .len = 8, .val = 0x001e }, { .len = 9, .val = 0x009c }, { .len = 7, .val = 0x0117 }, { .len = 8, .val = 0x007e }, { .len = 8, .val = 0x003e }, { .len = 9, .val = 0x00dc },
    { .len = 7, .val = 0x010f }, { .len = 8, .val = 0x006e }, { .len = 8, .val = 0x002e }, { .len = 9, .val = 0x00bc }, { .len = 8, .val = 0x000e }, { .len = 8, .val = 0x008e }, { .len = 8, .val = 0x004e }, { .len = 9, .val = 0x00fc },
    { .len = 7, .val = 0x0100 }, { .len = 8, .val = 0x0051 }, { .len = 8, .val = 0x0011 }, { .len = 8, .val = 0x0119 }, { .len = 7, .val = 0x0110 }, { .len = 8, .val = 0x0071 }, { .len = 8, .val = 0x0031 }, { .len = 9, .val = 0x00c2 },
    { .len = 7, .val = 0x0108 }, { .len = 8, .val = 0x0061 }, { .len = 8, .val = 0x0021 }, { .len = 9, .val = 0x00a2 }, { .len = 8, .val = 0x0001 }, { .len = 8, .val = 0x0081 }, { .len = 8, .val = 0x0041 }, { .len = 9, .val = 0x00e2 },
    { .len = 7, .val = 0x0104 }, { .len = 8, .val = 0x0059 }, { .len = 8, .val = 0x0019 }, { .len = 9, .val = 0x0092 }, { .len = 7, .val = 0x0114 }, { .len = 8, .val = 0x0079 }, { .len = 8, .val = 0x0039 }, { .len = 9, .val = 0x00d2 },
    { .len = 7, .val = 0x010c }, { .len = 8, .val = 0x0069 }, { .len = 8, .val = 0x0029 }, { .len = 9, .val = 0x00b2 }, { .len = 8, .val = 0x0009 }, { .len = 8, .val = 0x0089 }, { .len = 8, .val = 0x0049 }, { .len = 9, .val = 0x00f2 },
    { .len = 7, .val = 0x0102 }, { .len = 8, .val = 0x0055 }, { .len = 8, .val = 0x0015 }, { .len = 8, .val = 0x011d }, { .len = 7, .val = 0x0112 }, { .len = 8, .val = 0x0075 }, { .len = 8, .val = 0x0035 }, { .len = 9, .