# libssh2 1.11.1 — 手动代码分析

**目标**: libssh2 1.11.1
**分析日期**: 2026-04-17
**源码**: `/Users/xiaopingfeng/Projects/cyberai/targets/libssh2-1.11.1/`

---

## 架构安全特性

### 输入边界守护机制 (`struct string_buf`)
libssh2 1.11.x 引入了统一的 `string_buf` 边界检查原语：
```c
struct string_buf {
    unsigned char *data;      // 缓冲区起始
    unsigned char *dataptr;   // 当前读取位置
    size_t len;               // 总长度
};

// 安全读取原语 — 所有返回 -1 表示越界
int _libssh2_get_u32(struct string_buf *buf, uint32_t *out);
int _libssh2_get_string(struct string_buf *buf, unsigned char **str, size_t *len);
int _libssh2_get_boolean(struct string_buf *buf, unsigned char *out);
int _libssh2_copy_string(LIBSSH2_SESSION *session, struct string_buf *buf,
                          unsigned char **outbuf, size_t *outlen);
```
每个函数在消费数据前验证 `dataptr + size <= data + len`。

### 包长度限制
```c
#define LIBSSH2_PACKET_MAXCOMP      32000    // 压缩前最大
#define LIBSSH2_PACKET_MAXDECOMP    40000    // 解压后最大
#define LIBSSH2_PACKET_MAXPAYLOAD   40000    // 总载荷上限
#define MAX_SSH_PACKET_LEN          35000    // 发送缓冲区
```
`_libssh2_transport_read` 在分配内存前严格校验 `packet_length ≤ LIBSSH2_PACKET_MAXPAYLOAD`。

### 非阻塞状态机
函数通过 `libssh2_NB_state_*` 枚举保存跨调用状态，避免在部分读状态下使用悬空指针。

---

## 关键函数手动分析

### 1. `_libssh2_transport_read` (transport.c L367-899)

**CVE-2019-3855 修复验证** (packet_length 整数溢出):

CVE-2019-3855 根因: 旧版本中 `packet_length`（`uint32_t`）读取后直接用于内存分配计算，
若服务器发送 `0xFFFFFFFF` 则 `packet_length + mac_len` 溢出为小值，导致 `LIBSSH2_ALLOC` 分配不足，后续写入越界。

```c
// transport.c L597-665: 修复后的 packet_length 处理
p->packet_length = _libssh2_ntohu32(block);
if(p->packet_length < 1) {
    return LIBSSH2_ERROR_DECRYPT;
}
else if(p->packet_length > LIBSSH2_PACKET_MAXPAYLOAD) {  // = 40000
    return LIBSSH2_ERROR_OUT_OF_BOUNDARY;
}
// ...
if(total_num > LIBSSH2_PACKET_MAXPAYLOAD || total_num == 0) {
    return LIBSSH2_ERROR_OUT_OF_BOUNDARY;  // 第二道防线
}
```

- **双重检查**: `packet_length` 单独上限 + `total_num`（含 MAC）合计上限
- **结论**: ✅ CVE-2019-3855 完全修复，uint32_t 溢出不可达

### 2. `userauth_keyboard_interactive_decode_info_request` (userauth_kbd_packet.c)

**CVE-2019-3856 修复验证** (num_prompts 整数溢出导致 OOB 分配):

CVE-2019-3856 根因: 旧版本将服务器发送的 `num_prompts`（`uint32_t`）直接用于
`LIBSSH2_ALLOC(session, num_prompts * sizeof(LIBSSH2_USERAUTH_KBDINT_PROMPT))`，
大值 num_prompts 使乘法溢出为小值，导致分配不足然后越界写入。

```c
// userauth_kbd_packet.c L96-106: 修复代码
if(session->userauth_kybd_num_prompts > 100) {
    _libssh2_error(session, LIBSSH2_ERROR_OUT_OF_BOUNDARY,
                   "Too many replies for keyboard-interactive prompts");
    return -1;
}
// ...
// 安全分配: num_prompts ≤ 100, sizeof 已知, 不会溢出
session->userauth_kybd_prompts =
    LIBSSH2_CALLOC(session,
                   sizeof(LIBSSH2_USERAUTH_KBDINT_PROMPT) *
                   session->userauth_kybd_num_prompts);
```

- 硬上限 100 确保乘法结果 ≤ `100 * sizeof(KBDINT_PROMPT)` (已知有界)
- 每个 prompt 文本通过 `_libssh2_copy_string` 安全解析
- **结论**: ✅ CVE-2019-3856 修复

### 3. `_libssh2_packet_add` (packet.c L634-1394)

**CVE-2019-3857/3860/3861/3862 修复验证** (多种 OOB read):

这些 CVE 均源于 `_libssh2_packet_add` 在处理各种 SSH 消息类型时，
对载荷长度和偏移量的校验不足。修复后的模式：

```c
// 修复后统一使用 string_buf 原语而非手动偏移运算
struct string_buf buf;
buf.data = data;
buf.dataptr = data;
buf.len = datalen;

// 例: SSH_MSG_CHANNEL_DATA (CVE-2019-3862)
if(_libssh2_get_u32(&buf, &channel_id)) {
    return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                          "Data too short");
}
if(_libssh2_get_string(&buf, &payload, &payload_len)) {
    return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                          "Data too short for payload");
}
// 所有 `_libssh2_get_*` 均包含边界检查
```

- 所有消息解析路径均使用 `string_buf` 原语，消除了手动指针算术
- **结论**: ✅ CVE-2019-3857/3860/3861/3862 通过统一边界检查原语修复

### 4. `sftp_packet_add` (sftp.c L173-302)

**CVE-2019-3858 修复验证** (SFTP_PACKET_MAXLEN OOB read):

```c
// sftp.c: 数据包长度上限检查
static int sftp_packet_add(LIBSSH2_SFTP *sftp, unsigned char *data,
                            size_t data_len)
{
    LIBSSH2_SESSION *session = sftp->channel->session;
    LIBSSH2_SFTP_PACKET *packet;

    // 检查: 防止 malformed SFTP packet 导致后续越界读
    if(data_len < 5) {
        return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                              "SFTP packet too short");
    }
    // ...
}
```

- 最小长度 5 字节检查防止空载荷导致的 OOB
- **结论**: ✅ CVE-2019-3858 修复

### 5. `userauth_list` (userauth.c L67-228)

**CVE-2019-3863 修复验证** (方法列表整数溢出):

CVE-2019-3863 根因: 旧版本将服务器返回的认证方法列表长度字段（`uint32_t`）
直接传递给 `LIBSSH2_ALLOC`，溢出导致 `strlen` 访问越界内存。

修复后: `_libssh2_get_string(&buf, &data, &len)` 验证声明长度 ≤ 剩余缓冲区大小，
再通过 `LIBSSH2_ALLOC(session, len + 1)` 安全分配（`len` 已经过边界验证）。

- **结论**: ✅ CVE-2019-3863 修复

---

## 安全性总结

| 机制 | 描述 | 状态 |
|------|------|------|
| `string_buf` 统一边界检查 | 所有载荷解析使用 `_libssh2_get_*` 原语 | ✅ 强 |
| `LIBSSH2_PACKET_MAXPAYLOAD` | 40000 字节硬上限，双重检查 | ✅ 强 |
| num_prompts ≤ 100 | 防止键盘交互 num_prompts 整数溢出 | ✅ 强 |
| `LIBSSH2_CALLOC` | 零初始化防止未初始化内存泄漏 | ✅ 强 |
| 非阻塞状态机 | 部分读期间不使用悬空指针 | ✅ 强 |
| CVE-2019-3855 | packet_length ≤ MAXPAYLOAD 双重检查 | ✅ 已修复 |
| CVE-2019-3856 | num_prompts ≤ 100 上限 | ✅ 已修复 |
| CVE-2019-3857~3863 | `string_buf` 统一替换手动偏移 | ✅ 已修复 |

**初步判断**: libssh2 1.11.1 在 CVE-2019-38xx 批次后全面引入 `string_buf` 原语，
大幅降低了越界读取风险。新漏洞概率: 较低，GLM 扫描预期 FP 率 > 90%。
最值得关注: `sftp_read`/`sftp_write` 中的多次 `realloc` 交互，
以及 `scp_recv` 的文件名和时间戳解析路径（潜在路径遍历或整数溢出）。

---

*GLM-5.1 T1 扫描将在 API 配额恢复后 (08:05 BJT) 自动执行*
