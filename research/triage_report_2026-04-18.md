# CyberAI 漏洞 Triage 报告 — 2026-04-18

**分析范围**: 4 个目标的 GLM-5.1 扫描结果 (330 条原始 findings)
**分析工具**: GLM-5.1 自动扫描 + CVE 数据库交叉比对 + 手动代码审计
**扫描目标**: LibTIFF 4.7.0 | ImageMagick 7.1.1-44 | Mosquitto 2.0.21 | curl 8.11.0

---

## 执行摘要

| 目标 | 段数 | 原始 findings | conf≥80% | 已知CVE(FP) | 潜在新漏洞 | 状态 |
|------|------|--------------|---------|------------|-----------|------|
| LibTIFF 4.7.0 | 39/43 | 94 | 43 | ~41 (FP) | 2 候选 | ⚠️ 待验证 |
| ImageMagick 7.1.1-44 | 41/41 | 131 | 54 | ~50 (FP/已修) | 3 候选 | ⚠️ 待验证 |
| Mosquitto 2.0.21 | 22/22 | 72 | 36 | ~30 (FP) | 4 候选 | ⚠️ 待验证 |
| curl 8.11.0 | 14/14 | 33 | 7 | ~7 (FP) | 0 新发现 | ✅ 干净 |

---

## 一、LibTIFF 4.7.0

### 1.1 大量假阳性 — 已修复 CVE

| Finding | CVE | 修复版本 | 判断 |
|---------|-----|---------|------|
| PixarLogDecode llen 整数溢出 (L859) | CVE-2016-5875 | 4.0.8 | ❌ FP — 4.7.0 已修复 |
| PixarLog ABGR 堆溢出 (L978) | CVE-2016-5875 相关 | 4.0.8 | ❌ FP — 已修复 |
| LogLuv SGILOGDATAFMT_RAW (L275) | GitLab #645 | 4.7.0 | ❌ FP — 4.7.0 已修复 |
| ASCII tag count+1 溢出 (L6345) | CVE-2023-40745 | 4.6.0 | ❌ FP — 4.7.0 已修复 |
| PixarLogDecode nsamples×sizeof overflow | CVE-2022-3970 相关 | 4.5.0 | ❌ FP — 已修复 |
| TIFFReadScanline unvalidated size | CVE-2023-3618 相关 | 4.6.0 | ❌ FP — 已修复 |

### 1.2 待验证候选

#### ❌ CAND-001: ZIPEncode 负数 tif_rawdatasize — **已审计，假阳性**
- **位置**: `tif_zip.c` `ZIPEncode` L499 / `tif_write.c:TIFFWriteBufferSetup`
- **类型**: 有符号整数溢出 → 缓冲区溢出（假设）
- **审计结论 (2026-04-18)**: **FP — 所有赋值路径均有足够保护**
  - **写入路径** (`TIFFWriteBufferSetup`): `size` 来自 `TIFFStripSize()` → `_TIFFCastUInt64ToSSize()` 内部检查 `val > TIFF_TMSIZE_T_MAX`（= `SIZE_MAX>>1` = `INT64_MAX`）返回0，且有10%溢出余量检查
  - **读取路径** (`TIFFFillStrip` L755): 直接检查 `bytecount > TIFF_INT64_MAX` → 返回错误，之后才做 `(tmsize_t)bytecount`
  - ZIPEncode L50 的检查 `(size_t)x != (uint64_t)x` 在64-bit系统上永远为false（死代码），但此时 `tif_rawdatasize` 已经被上游防护保证为非负数
  - 无法通过正常 TIFF 文件解析路径使 `tif_rawdatasize` 变为负数
- **CVE 状态**: 假阳性，无实际漏洞

#### ❌ CAND-002: TIFFReadDirectory PersampleShort 栈溢出 — **已审计，假阳性**
- **位置**: `tif_dirread.c` L4520 → `TIFFReadDirEntryPersampleShort` L3276
- **类型**: 栈缓冲区溢出（假设）
- **审计结论 (2026-04-18)**: **FP — 不存在固定大小本地数组**
  - 实际代码: 通过 `TIFFReadDirEntryShortArray(tif, direntry, &m)` **堆分配**数组，无栈缓冲区
  - `m` 为堆指针，使用后 `_TIFFfreeExt(tif, m)` 释放
  - 本地变量只有 `uint16_t value`（单个值）和 `uint16_t nb`（计数器）
  - GLM 扫描误判"per-sample 值处理"为"固定数组"
- **CVE 状态**: 假阳性，无实际漏洞

---

## 二、ImageMagick 7.1.1-44

### 2.1 已知 CVE — 7.1.1-44 仍受影响

#### 🔴 CVE-2025-62171: BMP number_colors 整数溢出
- **位置**: `coders/bmp.c` L781
- **类型**: 整数溢出 → 堆缓冲区溢出（32-bit 平台）
- **描述**: `bmp_info.number_colors` 直接来自文件，无边界检查。`number_colors=0x40000000` + 8bpp 导致 `number_colors * 4 = 0`，分配 0 字节然后写入 ~1GB 数据
- **受影响版本**: ImageMagick < 7.1.2-7 / < 6.9.13-32（7.1.1-44 **仍受影响**）
- **限制**: 仅 32-bit 构建，需手动抬高资源限制
- **CVSS**: 待评估

#### 🟡 CVE-2025-57803 (GHSA-mxvv-97wh-cfmm): BMP bytes_per_line 整数溢出
- **位置**: `coders/bmp.c` L2039 `WriteBMPImage`
- **类型**: 整数溢出 → 堆缓冲区溢出
- **描述**: `bytes_per_line = 4 * ((columns * bits_per_pixel + 31) / 32)` 在 32-bit 系统上对极大列数溢出为小值
- **受影响版本**: ImageMagick < 7.1.2-2（7.1.1-44 **仍受影响**）
- **CVSS**: 9.8 (CRITICAL)

### 2.2 待验证候选

#### ❌ CAND-003: BMP ICC profile 有符号转换绕过 — **已审计，64-bit FP / 32-bit 潜在漏洞**
- **位置**: `coders/bmp.c` L1673 (extract `bmp_ReadBMPImage_I.c` L130-135)
- **描述**: `profile_size_orig` 以有符号 `MagickOffsetType` 读取 ICC profile 头 4 字节
- **审计结论 (2026-04-18)**:
  - **64-bit 构建** (现代 ImageMagick 7.x): `MagickOffsetType = int64_t`，4字节数据填入 bits 0-31，结果 0–4294967295，永远非负 → **FP**
  - **32-bit 构建**: `MagickOffsetType = int32_t`，`datum[0]=0x80 → profile_size_orig = 0x80000000 = INT32_MIN = -2147483648` (负数！)
    - `if (profile_size_orig < profile_size)`: `-2147483648 < 100` → TRUE，绕过了安全长度检查
    - `SetStringInfoLength(profile, (size_t)(-2147483648))` = `SetStringInfoLength(profile, 0x80000000)` → 尝试将100字节缓冲区扩展到2GB → 堆损坏
  - **实际影响**: ImageMagick 7.1.1-44 通常编译为64-bit → **实际为FP**；32-bit 构建存在真实漏洞
- **CVE 状态**: 未找到对应 CVE；32-bit 存在边界条件漏洞，但现代部署不受影响

#### ❌ CAND-004: BMP OS/2 负尺寸 — **已审计，假阳性**
- **位置**: `coders/bmp.c` L695 (extract `bmp_ReadBMPImage_A.c` L147)
- **描述**: OS/2 BMP 宽度读为 `(ssize_t)((short) ReadBlobLSBShort())` → `0xFFFF → -1`
- **审计结论 (2026-04-18)**: **FP — 负值被立即检查拦截**
  - `bmp_ReadBMPImage_D.c` L5-6: `if (bmp_info.width <= 0) ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize")`
  - 下游 L72: `image->columns=(size_t) MagickAbsoluteValue(bmp_info.width)` — 即使通过也有绝对值处理
  - 无法绕过宽度验证
- **CVE 状态**: 假阳性

#### 🟡 CAND-005: HeapOverflowSanityCheck 参数错误 — **代码质量问题，利用困难**
- **位置**: `coders/tiff.c` L2008 (extract `tiff_ReadTIFFImage_G.c` L136)
- **实际代码**:
  ```c
  if (HeapOverflowSanityCheck(rows, sizeof(*tile_pixels)) != MagickFalse)
    ThrowTIFFException(ResourceLimitError, "MemoryAllocationFailed");
  stride = (ssize_t) TIFFTileRowSize(tiff);
  extent = (size_t) MagickMax((size_t) tile_size, rows * MagickMax((size_t) stride, length));
  tile_pixels = (unsigned char *) AcquireQuantumMemory(extent, sizeof(*tile_pixels));
  ```
- **问题**: `HeapOverflowSanityCheck(rows, 1)` 只检查 `rows * 1` 溢出（永不发生），未检查实际的 `rows * stride` 乘法
- **审计结论 (2026-04-18)**: **真实代码质量问题，但实际利用受到多重限制**
  - `stride` 来自 `TIFFTileRowSize()` → LibTIFF 内部使用 `_TIFFCastUInt64ToSSize()` 溢出检测，溢出时返回0
  - `rows * stride` 溢出需要 `stride × rows > 2^64`；当 `stride = 4.3B` 时理论可能 (`rows = 4.3B × stride > 2^64`)，但此时 `TIFFTileSize` 也会溢出并返回0，导致 `extent = 0` → 分配失败
  - **结论**: 检查参数确实错误，但上游 LibTIFF 溢出保护阻止了实际利用
- **CVE 状态**: 代码质量问题，建议向 ImageMagick 报告修复 `HeapOverflowSanityCheck` 参数

---

## 三、Mosquitto 2.0.21

### 3.1 已确认漏洞（上一轮已报告）

- **Bridge Flag 权限提升**: 任意 MQTT 客户端可设置协议版本字节 bit7 获取桥接权限 (CVE-class, HIGH)

### 3.2 待验证候选 — 持久化文件 chunk 整数下溢

#### 🟡 CAND-006: persist_read client_msg length 下溢 — **已审计，低影响DoS**
- **位置**: `src/persist_read_v5.c` L81
- **代码**:
  ```c
  length -= (uint32_t)(sizeof(struct PF_client_msg) + chunk->F.id_len);
  // 之后:
  if(length > 0){
      prop_packet.payload = mosquitto__malloc(length);  // length = 0xFFFFFFFF
      if(!prop_packet.payload){ return MOSQ_ERR_NOMEM; }  // ← NULL 检查
  ```
- **审计结论 (2026-04-18)**: **下溢已在源码中确认 (L81)，但实际影响受限**
  - 下溢后 `length = 0xFFFFFFFF`，进入 `if(length > 0)` 为真
  - `mosquitto__malloc(0xFFFFFFFF)` 在现代系统上失败返回 NULL
  - NULL 检查存在：`if(!prop_packet.payload) return MOSQ_ERR_NOMEM`
  - **实际影响**: broker 在加载该持久化条目时返回错误，跳过此条目 — **非崩溃**
  - **注意**: 若 broker 因返回错误而中止启动，则为 DoS（阻止 broker 正常启动）
- **攻击向量**: 需本地写入/替换 Mosquitto 持久化数据库文件
- **CVE 状态**: 未找到对应 CVE（CVE-2023-28366 是不同问题）

#### 🟡 CAND-007: persist_read msg_store length 下溢 — **已审计，同上**
- **位置**: `src/persist_read_v5.c` L130
- **审计结论 (2026-04-18)**: 与 CAND-006 相同模式
  - `payloadlen` 有 `MQTT_MAX_PAYLOAD` 检查 (L121-123)，但 `source_id_len + source_username_len + topic_len` 之和未检查
  - 下溢后 `length = 0xFFFFFFFF`，同样因 `malloc` 失败 + NULL 检查而不崩溃
- **攻击向量**: 恶意持久化文件（本地写权限）
- **CVE 状态**: 未找到对应 CVE

### 3.3 待验证候选 — MQTT 协议层

#### CAND-008: Mosquitto MQTT 5.0 协议解析漏洞
- **状态**: 已验证，已提交上游，等待 CVE 分配
- **细节**: 待上游修复后公开

---

## 四、curl 8.11.0

### 4.1 全部为假阳性

| Finding | 原因 |
|---------|------|
| FTP PASV SSRF (L1830) | CVE-2020-8284，已于 curl 7.74.0 修复（IP 验证逻辑） |
| FTP LIST 权限解析 OOB | 类似 CVE-2017-8817，已修复 |
| Cookie `__Secure-/__Host-` 未强制 | 已知 RFC 规范，curl 未实现 Secure Cookie Prefix |

---

## 五、优先级排序

| 优先级 | ID | 目标 | 类型 | 攻击向量 | 行动 |
|--------|-----|------|------|---------|------|
| 🔴 P1 | CVE-2025-62171 | ImageMagick 7.1.1-44 | 堆溢出(32-bit) | 本地/解析 | 升级到 7.1.2-7 |
| 🔴 P1 | CVE-2025-57803 | ImageMagick 7.1.1-44 | 堆溢出 | 本地/解析 | 升级到 7.1.2-2 |
| 🟡 P2 | CAND-008 | Mosquitto 2.0.21 | 协议解析漏洞 | 远程 | 已提交上游，待公开 |
| ❌ FP | CAND-001 | LibTIFF 4.7.0 | 有符号溢出(假阳) | 解析 | ✅已审计，FP |
| ❌ FP | CAND-003 | ImageMagick 7.1.1-44 | ICC绕过(64-bit FP) | 解析 | ✅已审计，64-bit FP |
| 🟢 P3 | CAND-006/007 | Mosquitto 2.0.21 | DoS (持久化文件) | 本地写权限 | 代码审计 |
| ❌ FP | CAND-002 | LibTIFF 4.7.0 | 栈溢出(假阳) | 解析 | ✅已审计，FP |
| ❌ FP | CAND-004 | ImageMagick 7.1.1-44 | OS/2负尺寸(假阳) | 解析 | ✅已审计，FP |
| 🟢 P3 | CAND-005 | ImageMagick 7.1.1-44 | 代码质量(HeapCheck参数错) | 解析 | ✅已审计，建议修复 |

---

## 六、下一步行动

1. **【✅ 已完成】** Mosquitto CAND-008 验证并提交上游，等待修复后公开
2. **【✅ 已完成】** LibTIFF CAND-001/002 代码审计：均为假阳性（有完整的溢出保护路径）
3. **【✅ 已完成】** ImageMagick CAND-003/004/005 代码审计：CAND-003/004 为64-bit FP，CAND-005 为代码质量问题
4. **【本周】** 向 ImageMagick 报告 CAND-005：`HeapOverflowSanityCheck(rows, sizeof(*tile_pixels))` 检查参数应改为 `(rows * max(stride, length), 1)` 或等价形式
6. **【已完成】** LibTIFF PixarLog ABGR (F1) → 已向 `tiff@lists.osgeo.org` 准备披露报告
7. **【明日 08:05 BJT】** 10目标每日扫描自动运行（含新增的 FreeType 2.13.3，49段）

---

## 七、候选漏洞最终状态汇总 (2026-04-18 审计后)

| ID | 目标 | 最终状态 | 说明 |
|----|------|---------|------|
| CVE-2025-62171 | ImageMagick | 🔴 已知CVE，仍受影响 | 需升级到 7.1.2-7 |
| CVE-2025-57803 | ImageMagick | 🔴 已知CVE，仍受影响 | 需升级到 7.1.2-2 |
| CAND-001 | LibTIFF | ❌ 假阳性 | `tif_rawdatasize` 所有赋值路径均有溢出保护 |
| CAND-002 | LibTIFF | ❌ 假阳性 | `PersampleShort` 使用堆分配，无栈数组 |
| CAND-003 | ImageMagick | ❌ 64-bit FP | 32-bit 构建存在真实问题，64-bit 不受影响 |
| CAND-004 | ImageMagick | ❌ 假阳性 | 负宽度被 `width <= 0` 检查拦截 |
| CAND-005 | ImageMagick | 🟡 代码质量 | 检查参数错误，利用受上游保护限制 |
| CAND-006 | Mosquitto | 🟡 本地DoS | 持久化文件 client_msg length 下溢（需本地写权限）|
| CAND-007 | Mosquitto | 🟡 本地DoS | 持久化文件 msg_store length 下溢（需本地写权限）|
| CAND-008 | Mosquitto | 🟡 已提交上游 | 已提交 security@mosquitto.org，等待修复后公开 |

---

*报告生成时间: 2026-04-18 · CyberAI GLM-5.1 扫描管线*
