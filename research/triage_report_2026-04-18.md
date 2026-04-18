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

#### 🟡 CAND-001: ZIPEncode 负数 tif_rawdatasize
- **位置**: `tif_zip.c` `ZIPEncode` L499
- **类型**: 有符号整数溢出 → 缓冲区溢出
- **描述**: `tif->tif_rawdatasize` 类型为 `tmsize_t`（有符号）。若在内存分配路径中因整数溢出变为负数，后续将负数当作大 `size_t` 使用，导致写入越界
- **CVE 状态**: 未找到对应 CVE，候选新漏洞
- **下一步**: 检查 `tif_rawdatasize` 赋值路径中是否有足够的溢出检查

#### 🟡 CAND-002: TIFFReadDirectory PersampleShort 栈溢出
- **位置**: `tif_dirread.c` L4520
- **类型**: 栈缓冲区溢出
- **描述**: 处理 `BITSPERSAMPLE/SAMPLEFORMAT` 等 per-sample 标签时，先读取 count != 1 的情况，使用固定大小的本地数组存储 `SamplesPerPixel=64` 的样本值。若该本地数组容量不足，可能发生栈溢出
- **CVE 状态**: 未找到直接对应 CVE
- **下一步**: 检查 `_TIFFReadDirEntryPersampleShort` 中本地数组的实际声明大小

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

#### 🟡 CAND-003: BMP ICC profile 有符号转换绕过
- **位置**: `coders/bmp.c` L1673
- **描述**: `profile_size_orig` 以有符号 `MagickOffsetType` 读取 ICC profile 头 4 字节。负值可绕过 `profile_size_orig > file_size` 检查，导致后续越界读
- **CVE 状态**: 未找到对应 CVE，潜在新漏洞
- **下一步**: 构造 ICC profile 头字节 = `0x80000000`，验证检查是否被绕过

#### 🟡 CAND-004: BMP OS/2 负尺寸
- **位置**: `coders/bmp.c` L695
- **描述**: OS/2 BMP (header size=12) 将宽/高读为 `uint16_t` 后转为 `signed short`，`width=0xFFFF → -1`，导致 scanline 计算为负数
- **CVE 状态**: 未找到对应 CVE
- **下一步**: 用 header_size=12, width=0xFFFF 的 OS/2 BMP 测试

#### 🟡 CAND-005: BMP HeapOverflowSanityCheck 不完整
- **位置**: `coders/tiff.c` L2008 `ReadTIFFImage`
- **描述**: `HeapOverflowSanityCheck(rows * sizeof(*tile_pixels))` 其中 `sizeof(*tile_pixels)=1`，等价于检查 `rows * 1`，但实际分配是 `rows * stride`。stride 大时仍会溢出
- **CVE 状态**: 未找到完全匹配的 CVE

---

## 三、Mosquitto 2.0.21

### 3.1 已确认漏洞（上一轮已报告）

- **Bridge Flag 权限提升**: 任意 MQTT 客户端可设置协议版本字节 bit7 获取桥接权限 (CVE-class, HIGH)

### 3.2 待验证候选 — 持久化文件 chunk 整数下溢

#### 🟡 CAND-006: persist_read client_msg length 下溢
- **位置**: `src/persist_read_v5.c` L81
- **代码**:
  ```c
  length -= (uint32_t)(sizeof(struct PF_client_msg) + chunk->F.id_len);
  ```
- **描述**: 若 `chunk->F.id_len` 来自恶意持久化文件且足够大，`length` 从 `uint32_t` 下溢到 `0xFFFFFFFF`，后续 `if(length > 0)` 为真，`mosquitto__malloc(0xFFFFFFFF)` → OOM 崩溃 或 OOB 读
- **攻击向量**: 攻击者需能写入/替换 Mosquitto 的持久化数据库文件（本地权限）
- **CVE 状态**: 未找到对应 CVE（CVE-2023-28366 是不同问题）
- **影响**: DoS（broker 崩溃），配合本地写权限可能提权

#### 🟡 CAND-007: persist_read msg_store length 下溢
- **位置**: `src/persist_read_v5.c` L130
- **代码**:
  ```c
  length -= (uint32_t)(sizeof(struct PF_msg_store) + chunk->F.payloadlen 
                       + chunk->F.source_id_len + chunk->F.source_username_len 
                       + chunk->F.topic_len);
  ```
- **描述**: 同上，多个字段之和超过 `length` 时下溢
- **攻击向量**: 恶意持久化文件
- **CVE 状态**: 未找到对应 CVE

### 3.3 待验证候选 — MQTT 协议层

#### 🟡 CAND-008: property__read proplen 下溢 (远程 DoS)
- **位置**: `lib/property_mosq.c` L24–105 `property__read`
- **描述**: `proplen`（`uint32_t`）通过 `*len -= 1/2/4` 逐步消耗。若声明 `proplen=1`，读完 property identifier (`*len → 0`) 后再减 value 字节数（value 读成功）→ `*len = 0xFFFFFFFF`，`while(proplen > 0)` 继续执行 ~2^32 次迭代直到包数据耗尽
- **攻击向量**: 任意 MQTT 5.0 客户端（**无需认证，remote**）发送特制 CONNECT/PUBLISH 包
- **影响**: DoS — broker 在极短时间内自旋 ~packet_size 次后返回 PROTOCOL 错误，但可通过批量包放大
- **CVE 状态**: 未找到对应 CVE，CVE-2023-3592 是不同问题
- **下一步**: 构造 `proplen=2` 且数据为一个 3-byte 属性 (1 byte id + 2 byte uint16 value)，验证 proplen 是否下溢

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
| 🟡 P2 | CAND-008 | Mosquitto 2.0.21 | DoS (proplen下溢) | **远程，无认证** | PoC 验证 |
| 🟡 P2 | CAND-001 | LibTIFF 4.7.0 | 有符号溢出 | 解析 | 代码审计 |
| 🟡 P2 | CAND-003 | ImageMagick 7.1.1-44 | ICC 绕过 | 解析 | PoC 验证 |
| 🟢 P3 | CAND-006/007 | Mosquitto 2.0.21 | DoS (持久化文件) | 本地写权限 | 代码审计 |
| 🟢 P3 | CAND-002 | LibTIFF 4.7.0 | 栈溢出 | 解析 | 代码审计 |
| 🟢 P3 | CAND-004/005 | ImageMagick 7.1.1-44 | OOB | 解析 | 代码审计 |

---

## 六、下一步行动

1. **【立即】** Mosquitto CAND-008 PoC 验证：发送 `proplen=2` MQTT CONNECT 包，观察 broker CPU 使用
2. **【今日】** LibTIFF CAND-001 代码审计：追踪 `tif_rawdatasize` 所有赋值路径
3. **【本周】** ImageMagick CAND-003 PoC：构造 ICC profile 头 `0x80000000`
4. **【通知】** 如果 CAND-008 可确认：向 Mosquitto 安全邮件 `security@mosquitto.org` 披露
5. **【已完成】** LibTIFF PixarLog ABGR (F1) → 已向 `tiff@lists.osgeo.org` 准备披露报告

---

*报告生成时间: 2026-04-18 · CyberAI GLM-5.1 扫描管线*
