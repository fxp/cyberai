# libpng 1.6.45 — 手动代码分析

**目标**: libpng 1.6.45
**分析日期**: 2026-04-17
**源码**: `/Users/xiaopingfeng/Projects/cyberai/targets/libpng-1.6.45/`

---

## 总体安全特性

libpng 1.6.45 的主要防御机制（7层）：

| 机制 | 覆盖范围 | 效果 |
|------|---------|------|
| `png_get_uint_31()` | 所有 4-byte chunk 长度/尺寸字段 | 限制所有值 < 2^31 |
| `png_check_chunk_length()` | 每个 chunk handler 入口 | 拒绝超过 user_limit 的 chunk |
| `PNG_USER_WIDTH_MAX/HEIGHT_MAX` | IHDR 宽高验证 | 默认上限 1,000,000 像素 |
| 两遍 inflate 策略 | `png_decompress_chunk` | 先 NULL 输出测大小再分配，防大分配 |
| `PNG_INFLATE_BUF_SIZE` | inflate 本地缓冲 | 分块防止单次超大解压 |
| `png_icc_check_length()` + `png_icc_check_header()` | iCCP chunk | 验证 ICC profile 长度和内部一致性 |
| `PNG_SIZE_MAX / sizeof(entry)` | sPLT/量化表分配 | 防止乘法溢出 |

---

## 分段手动分析

### 1. png_handle_IHDR

**文件**: `pngrutil.c`
**关键路径**:
```c
length = png_read_chunk_header(png_ptr);  // png_check_chunk_length() called here
if(length != 13) png_chunk_error(..., "invalid");
width  = png_get_uint_31(png_ptr, buf);   // < 2^31
height = png_get_uint_31(png_ptr, buf+4); // < 2^31
// PNG_USER_WIDTH_MAX check: if(width > PNG_USER_WIDTH_MAX) png_error()
// PNG_USER_HEIGHT_MAX check: same
```

- `length == 13` 精确匹配，无 OOB 读
- `png_get_uint_31()` 同时限制 width/height < 2^31
- `PNG_USER_WIDTH_MAX/HEIGHT_MAX` (default 1M) 二次限制
- `PNG_ROWBYTES(pixel_depth, width)` 使用 size_t 乘法，pixel_depth ≤ 64，width ≤ 1M，乘积 ≤ 8MB，无溢出
- **结论**: ✅ 安全

---

### 2. png_handle_PLTE

**文件**: `pngrutil.c`
**关键路径**:
```c
if(length > (unsigned int)(3 * PNG_MAX_PALETTE_LENGTH)) png_chunk_benign_error(...);
// PNG_MAX_PALETTE_LENGTH = 256
num = length / 3;
png_ptr->palette = (png_colorp)png_malloc(png_ptr, num * (sizeof(png_color)));
png_crc_read(png_ptr, (png_bytep)palette, length);
```

- `length` 被限制在 ≤ 768 (3 × 256)
- `num ≤ 256`，`num × sizeof(png_color) = 256 × 3 = 768` 字节固定上界，无溢出
- `png_crc_read` 读取恰好 `length` 字节到 `palette`，已分配足够空间
- **结论**: ✅ 安全

---

### 3. png_inflate

**文件**: `pngrutil.c`
**关键路径**:
```c
#define ZLIB_IO_MAX ((uInt)0x7fffffff)
avail = (uInt)(-1);
if(avail > (uInt)output_size) avail = (uInt)output_size;
if(avail > ZLIB_IO_MAX) avail = ZLIB_IO_MAX;
png_ptr->zstream.avail_out = avail;
```

- `ZLIB_IO_MAX = 0x7FFFFFFF` 防止 zlib `uInt` 截断
- `avail_out` 由调用者的 `output_size` 参数控制，函数不超出调用者分配的缓冲区
- inflate 循环以 `Z_STREAM_END` 或错误为终止条件，不无限扩展
- **结论**: ✅ 安全

---

### 4. png_decompress_chunk

**文件**: `pngrutil.c`
**关键路径**:
```c
limit = png_ptr->user_chunk_malloc_max;
// Pass 1: inflate to NULL to find output size
result = png_inflate(png_ptr, type, 1/*finish*/, NULL, NULL/*output*/, &lzsize);
// lzsize is the decompressed size
if(lzsize > limit) png_error(..., "too large");
// Pass 2: allocate then inflate for real
output = png_malloc(png_ptr, lzsize + prefix_size + terminate);
result = png_inflate(png_ptr, type, 1/*finish*/, input, output, &lzsize);
```

- 两遍策略：第一遍仅计算大小（NULL输出），第二遍才分配
- `lzsize > limit` 阻止超大分配（limit 由用户设置，默认等同 `PNG_SIZE_MAX`）
- 分配量 `lzsize + prefix_size + terminate` 全部在 limit 验证后计算，`prefix_size` 由内部已知值控制，无溢出
- **结论**: ✅ 安全

---

### 5. png_handle_iCCP

**文件**: `pngrutil.c`
**关键路径**:
```c
// keyword: max 81 bytes (min(81, length))
for(tag_length = 0; tag_length < 80; ++tag_length) { ... }  // NUL search ≤ 80 次
// Read minimal 132 bytes to check profile header
png_icc_check_length(png_ptr, colorspace, "iCCP", profile_length);
// → validates profile_length <= user_chunk_malloc_max
png_icc_check_header(png_ptr, colorspace, "iCCP", profile_length, profile, color_type);
// → validates internal ICC header fields for consistency
```

- keyword 搜索上限 80 次，NUL 终止保证
- `png_icc_check_length()` 拒绝超出 `user_chunk_malloc_max` 的 profile
- `png_icc_check_header()` 验证 ICC magic number、version、profile class 等
- **结论**: ✅ 安全

---

### 6. png_handle_sPLT

**文件**: `pngrutil.c`
**关键路径**:
```c
entry_size = (depth == 8 ? 6 : 10);
nentries = (png_uint_32)(data_length / entry_size);
// Division prevents overcount; any remainder means malformed chunk
if(data_length % entry_size != 0) png_error(..., "invalid data length");
// Guard against malloc overflow:
if(nentries > PNG_SIZE_MAX / (sizeof(png_sPLT_entry))) png_error(..., "too many");
new_palette->entries = png_malloc_array(png_ptr, nentries, sizeof(png_sPLT_entry));
```

- `nentries × entry_size = data_length` — 整除关系保证不多读
- `nentries > PNG_SIZE_MAX / sizeof(png_sPLT_entry)` 防乘法溢出
- **行为注意点**: `depth` 字段不验证为 8 或 16 — 其他值 (如 4、32) 会使 `entry_size=10`，导致数据解析错误，但 **内存安全不受影响**（malloc 量由整除 `data_length` 决定）
- **结论**: ✅ 内存安全 (minor behavioral issue: depth not validated)

---

### 7. png_handle_tRNS

**文件**: `pngrutil.c`
**关键路径**:
- Palette type: `if(length > (unsigned)png_ptr->num_palette) png_chunk_error()`; 读取恰好 `num_palette` 字节
- Grayscale type: `if(length != 2) png_chunk_error()`
- RGB type: `if(length != 6) png_chunk_error()`

- 三种色彩类型均有精确长度验证
- Palette 模式限制 ≤ 256 条目（`num_palette ≤ 256`）
- **结论**: ✅ 安全

---

### 8. png_handle_tEXt

**文件**: `pngrutil.c`
**关键路径**:
```c
buffer = png_read_buffer(png_ptr, length + 1, 2/*warn*/);
// png_read_buffer: 保留内部缓冲区，只有在 length+1 > current_size 时才 realloc
// length 来自 png_check_chunk_length() — 已限制 < 2^31
// length + 1 ≤ 2^31, size_t 在 32/64 位均 ≥ 4 字节 → 安全
png_crc_read(png_ptr, buffer, length);
buffer[length] = 0;  // NUL-terminate
```

- `length + 1` 加法：`length` 已经过 `png_check_chunk_length()` 限制 ≤ `PNG_UINT_31_MAX = 2^31-1`，故 `length+1 ≤ 2^31`，在 size_t 范围内安全
- 内部缓冲区由 `png_read_buffer` 管理，只增不减（复用优化）
- **结论**: ✅ 安全

---

### 9. png_handle_zTXt

**文件**: `pngrutil.c`
**关键路径**:
- keyword 读取与 tEXt 相同（max 80 字节 NUL 搜索）
- compression_method 验证 == 0（唯一合法值）
- 压缩数据通过 `png_decompress_chunk()` 解压（见分析 #4，两遍策略保护）
- **结论**: ✅ 安全

---

### 10. png_handle_iTXt

**文件**: `pngrutil.c`
**关键路径**:
- keyword、language tag、translated keyword 均有独立 NUL 搜索上限
- 压缩标志检查：`if(comp_flag != 0 && comp_flag != 1)` 拒绝非法值
- 压缩内容通过 `png_decompress_chunk()` 处理（两遍策略保护）
- **已知注意点** (`pngpread.c L670,688`):
  ```c
  /* TODO: WARNING: TRUNCATION ERROR: DANGER WILL ROBINSON: */
  png_ptr->current_text_size = (png_size_t)png_ptr->push_length;
  // push_length is uInt (32-bit on all platforms)
  // png_size_t = size_t (≥32-bit)
  // On 64-bit: no truncation. On 32-bit: no truncation either (same width)
  ```
  开发者留的 TODO 注释；在实际平台上不会截断（两者均为 32 位或 size_t ≥ 32 位）
- **结论**: ✅ 安全

---

### 11. png_handle_pCAL

**文件**: `pngrutil.c`
**关键路径**:
```c
// Read buffer with length+1 (same protection as tEXt)
// equation_type validated in switch: 0-3 are handled
default: png_chunk_benign_error(..., "unrecognized equation type");
// After default: parsing continues (!)  → allocates params[] based on nparams
```

- **行为注意点**: unknown equation type 不 abort，而是继续执行 `nparams` 参数分配和解析
- 内存安全：`nparams` 来自 chunk 数据，但 `params` 数组分配量由 `nparams` 精确控制，且 chunk 总长度已被 `png_check_chunk_length()` 限制
- 最坏情况：解析无意义数据，不产生堆溢出
- **结论**: ✅ 内存安全 (behavioral: unknown equation type not aborted)

---

### 12. png_handle_sCAL

**文件**: `pngrutil.c`
**关键路径**:
- unit byte: 仅接受 1 (meter) 或 2 (radian)
- width/height strings: 用 `strtod()` 解析，无 OOB
- chunk data 已通过 `png_read_buffer` + `length` 限制
- **结论**: ✅ 安全

---

### 13. png_combine_row / png_do_read_interlace

**文件**: `pngrutil.c`
**关键路径**:
- `png_combine_row`: 目标行缓冲区由调用者提供，大小 = `png_get_rowbytes()`（基于 width × pixel_depth，已在 IHDR 验证）
- 像素位操作全部在 `png_get_rowbytes()` 范围内
- `png_do_read_interlace`: Adam7 交织访问使用 `PNG_PASS_COLS(width, pass)` 宏，返回当前通道列数，不超过原始 width
- **结论**: ✅ 安全

---

### 14. png_push_read_chunk (push parser)

**文件**: `pngpread.c`
**关键路径**:
```c
png_ptr->push_length = png_get_uint_31(png_ptr, chunk_length);  // < 2^31
// Dispatch to individual handlers — all have length pre-validated
```

- 状态机架构，每个状态只处理固定字节数
- `push_length` 通过 `png_get_uint_31()` 限制 < 2^31
- 进入各 handler 前 chunk data 必须完整缓冲
- **结论**: ✅ 安全

---

## 最终结论

| 函数 | 结论 | 备注 |
|------|------|------|
| png_handle_IHDR | ✅ 安全 | 精确长度匹配 + uint_31 + user limits |
| png_handle_PLTE | ✅ 安全 | 固定上界 768 字节 |
| png_inflate | ✅ 安全 | ZLIB_IO_MAX 防 uInt 截断 |
| png_decompress_chunk | ✅ 安全 | 两遍策略，先测大小后分配 |
| png_handle_iCCP | ✅ 安全 | 多重验证 (length + header) |
| png_handle_sPLT | ✅ 安全 | depth 未验证但内存安全 |
| png_handle_tRNS | ✅ 安全 | 精确长度匹配（3 种类型） |
| png_handle_tEXt | ✅ 安全 | length+1 在 uint_31 范围内安全 |
| png_handle_zTXt | ✅ 安全 | 两遍解压保护 |
| png_handle_iTXt | ✅ 安全 | 已知 TODO 注释不构成实际截断 |
| png_handle_pCAL | ✅ 安全 | unknown type 不 abort 但内存安全 |
| png_handle_sCAL | ✅ 安全 | 仅接受两个合法 unit 值 |
| png_combine_row | ✅ 安全 | 行缓冲区大小由 IHDR 决定 |
| png_push_read_chunk | ✅ 安全 | uint_31 保护 + 状态机 |

**内存安全漏洞**: 0
**行为安全问题**: 2（sPLT depth 未验证、pCAL unknown type 不 abort）

libpng 1.6.45 在分析范围内不含可利用漏洞。代码在关键数学安全（防整数溢出）和解压大小控制上的设计比 libtiff、ImageMagick 等同类库更为严谨。

---

## GLM 扫描 FP 预测

当 GLM 扫描恢复时，预期会出现以下假阳性：

| 预期 FP | 原因 |
|---------|------|
| `length+1` overflow in png_read_buffer | length 已被 check_chunk_length 限制 < 2^31 |
| `malloc(nentries × entry_size)` overflow in sPLT | PNG_SIZE_MAX / sizeof(entry) 守护已存在 |
| `png_decompress_chunk` 内存分配无界 | 两遍策略已有 limit 保护 |
| zlib `avail_out` uInt 截断 | ZLIB_IO_MAX 已处理 |
| `push_length` 32→size_t 截断 | uint_31 保证 ≤ 2^31，size_t ≥ 32 位安全 |
| `profile_length` 超大分配 | png_icc_check_length 前置验证 |

---

*手动分析完成。等待 GLM 扫描结果 (API 配额每日 08:00 BJT 重置)*
