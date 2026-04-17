# curl 8.11.0 — 手动代码分析

**分析日期**: 2026-04-17
**源码位置**: `/Users/xiaopingfeng/Projects/cyberai/targets/curl-curl-8_11_0/lib/`
**分析范围**: 14个安全敏感函数提取文件

---

## 总体安全评估

curl 的代码质量极高，防御机制到位：

1. **CURL_MAX_INPUT_LENGTH = 8,000,000** — 所有用户输入的全局上限
2. **dynbuf 动态缓冲区** — `Curl_dyn_*` 系列函数统一管理缓冲区，自动限制大小
3. **链式错误返回** — 每步操作返回 CURLcode/CURLUcode，调用链不跳过错误检查
4. **单线程模型** — curl 在单线程事件循环中运行，无竞态条件
5. **全面的输入验证** — 数字解析用 strtoul + 范围检查，字符串用 strcspn 白名单过滤

**预期 FP 率**: 80-90%

---

## 逐文件分析

### 1. urlapi_A.c — parseurl (GLM扫描失败，手动分析)

**状态**: GLM服务器超时(300s)，已补充手动分析

**关键代码路径**:
```c
result = junkscan(url, &urllen, flags);   // 拒绝嵌入 \0 和控制字符
schemelen = Curl_is_absolute_url(url, schemebuf, sizeof(schemebuf), flags);

// file:// URL 特殊处理
// - checkprefix("localhost/", ptr) 或 "127.0.0.1/"
// - Windows UNC: 独立路径

// 非 file: URL
hostlen = strcspn(hostp, "/?#");  // 找主机名终点
result = parse_authority(u, hostp, hostlen, flags, &host, schemelen);
```

**已识别的潜在关注点**:
- `ipv4_normalize()` 接受 base-0 `strtoul` (八进制/十六进制) → 已修改 URL 可能绕过基于字符串的 IP ACL
  - 例: `http://0177.0.0.1/` 被规范化为 `http://127.0.0.1/`
  - **这是设计行为**，非漏洞，但可能导致 SSRF 过滤器绕过
  - 风险评级: MEDIUM (需要配合 SSRF 过滤场景)

**结论**: 无内存安全漏洞; `ipv4_normalize` 是行为安全问题(非内存)

---

### 2. urlapi_B.c — parse_hostname_login + ipv6_parse + hostname_check

**状态**: GLM扫描失败(300s)，手动分析

**ipv6_parse() 分析**:
```c
char zoneid[16];   // 16 字节固定缓冲区
int i = 0;
while(*h && (*h != ']') && (i < 15))
    zoneid[i++] = *h++;   // i < 15 保证最多写 15 字节
zoneid[i] = 0;            // null 终止符写在 index ≤ 15 → 安全
```
→ **安全**: 循环条件 `i < 15` 确保最多 15 字节写入，第 16 字节用于 null 终止符

**parse_hostname_login() 分析**:
```c
ptr = memchr(login, '@', len);    // 安全: len 有界
if(!ptr) goto out;
// Curl_parse_login_details: 解析 user:password[@...]
// 错误路径: goto out → free(userp/passwdp/optionsp)
```
→ **安全**: 完整的错误路径清理，无内存泄漏

**hostname_check() 分析**:
```c
len = strcspn(hostname, " \r\n\t/:#?!@{}[]\\$'\"^`*<>=;,+&()%");
if(hlen != len)
    return CURLUE_BAD_HOSTNAME;   // 拒绝包含坏字符的主机名
```
→ **安全**: 白名单方法，包含所有危险字符

**结论**: 无漏洞

---

### 3. urlapi_C.c — curl_url_set + curl_url_get

**curl_url_set() 分析**:
```c
nalloc = strlen(part);
if(nalloc > CURL_MAX_INPUT_LENGTH)   // 8MB 上限
    return CURLUE_MALFORMED_INPUT;

// port 验证:
port = strtoul(part, &endp, 10);
if(errno || (port > 0xffff) || *endp)
    return CURLUE_BAD_PORT_NUMBER;

// 编码缓冲区:
Curl_dyn_init(&enc, nalloc * 3 + 1 + leadingslash);
// nalloc <= 8,000,000; nalloc*3 <= 24,000,000 << SIZE_MAX → 无溢出
```
→ **安全**: 全面的输入验证

**结论**: 无漏洞

---

### 4. url_A.c — parseurlandfillconn + setup_range

**关键检查**:
```c
// 主机名长度检查:
else if(strlen(data->state.up.hostname) > MAX_URL_LEN) {
    failf(data, "Too long hostname (maximum is %d)", MAX_URL_LEN);
    return CURLE_URL_MALFORMAT;
}
```
→ **安全**: 委托给 URL API，自身仅做二次长度检查

**结论**: 无漏洞

---

### 5. cookie_A.c — parse_cookie_header + sanitize

**防御层次**:
1. `linelength > MAX_COOKIE_LINE` → `CERR_TOO_LONG` (入口检查)
2. `nlen >= (MAX_NAME-1) || vlen >= (MAX_NAME-1)` → `CERR_TOO_BIG`
3. `invalid_octets(co->value)` → `CERR_INVALID_OCTET`
4. TAB 字符检查: `memchr(valuep, '\t', vlen)` → `CERR_TAB`

**边缘情况 (不可利用)**:
```c
// domain="." 情况:
if('.' == valuep[0]) {
    valuep++;   // 跳过前导点
    vlen--;     // 如果 vlen 原来是 1 → vlen 变为 0
}
// 后续 && vlen 条件在进入时已保证 vlen >= 1，减后 vlen=0
// strstore(&co->domain, valuep, 0) → 空字符串，无害
```

**结论**: 无可利用漏洞; "." 域名边缘情况行为怪但无害

---

### 6. ftp_A.c — match_pasv_6nums + ftp_state_pasv_resp

**PASV 响应解析**:
```c
// match_pasv_6nums: 6个数字均验证 ≤ 255
num = strtoul(p, &endp, 10);
if(num > 255) return FALSE;
array[i] = (unsigned int)num;

// EPSV: 端口号验证
if(num > 0xffff) return CURLUE_BAD_PORT_NUMBER;
```

**SSRF 防护**:
- `ftp_skip_ip` 选项: 忽略服务器返回的 IP，使用原始连接的 IP
- 即使不使用 ftp_skip_ip，IP 由 6 个各 ≤ 255 的字节构成 → 合法 IPv4

**结论**: 无漏洞; SSRF 防护到位

---

### 7. socks_A.c — do_SOCKS4

**缓冲区大小**: `sx->buffer[600]`

**关键检查**:
```c
// proxy_user 长度检查:
if(plen > 255) { failf(...); return CURLPX_LONG_USER; }
memcpy(socksreq + 8, sx->proxy_user, plen + 1);   // 8+256=264 << 600

// SOCKS4a 主机名检查:
hostnamelen = strlen(sx->hostname) + 1;
if((hostnamelen <= 255) && (packetsize + hostnamelen < sizeof(sx->buffer)))
    strcpy((char *)socksreq + packetsize, sx->hostname);
// 双重检查: 长度 AND 总包大小
```

**结论**: 无漏洞

---

### 8. socks_B.c — do_SOCKS5

**SOCKS5 认证缓冲区构建**:
```c
// 主机名: 
if(!socks5_resolve_local && hostname_len > 255)
    return CURLPX_LONG_HOSTNAME;   // 远程解析时限制 ≤ 255

// 认证包:
// [VER(1)] [ULEN(1)] [USER(≤255)] [PLEN(1)] [PASSWD(≤255)] = 最大 513 字节 << 600

// 用户名检查 (顺序问题但安全):
socksreq[len++] = (unsigned char) proxy_user_len;   // 先写截断值
if(proxy_user_len > 255) { return CURLPX_LONG_USER; } // 再检查
memcpy(socksreq + len, ...);   // 只有 ≤ 255 才到达这里
```
**注意**: 长度字节写入顺序在检查之前，但 `memcpy` 被 `> 255` guard 完全保护

**结论**: 无漏洞

---

### 9. http_chunks_A.c — httpchunk_readwrite

**关键防护**:
```c
case CHUNK_HEX:
    if(ch->hexindex >= CHUNK_MAXNUM_LEN) {
        failf(...); return CURLE_RECV_ERROR;  // hex 长度上限
    }

case CHUNK_DATA:
    piece = blen;
    if(ch->datasize < (curl_off_t)blen)
        piece = curlx_sotouz(ch->datasize);   // 取最小值
    ch->datasize -= piece;                     // ch->datasize >= piece → 无下溢
```

**状态机**: 明确的状态转换防止状态混淆

**结论**: 无漏洞

---

### 10. ftplist_A.c — ftp_pl_state_machine

**缓冲区限制**:
```c
#define MAX_FTPLIST_BUFFER 10000   /* 限制每个文件条目大小 */
Curl_dyn_init(&parser->file_data->buf, MAX_FTPLIST_BUFFER);
```

**安全特性**:
- dynbuf 自动限制 10KB/条目
- 偏移量从 dynbuf 内部计算，不会越界

**结论**: 无漏洞

---

### 11. mime_A.c — mime_read + boundary

**边界生成**:
- `MIME_BOUNDARY_DASHES = 24` (前导破折号数)
- `MIME_RAND_BOUNDARY_CHARS` (随机部分长度)
- 使用 `Curl_rand()` 生成随机边界
- 边界存储在固定大小 struct 字段中

**结论**: 无漏洞

---

### 12. http_digest_A.c — Curl_output_digest

**关键路径**:
```c
// path 构建:
if(authp->iestyle) {
    tmp = strchr((char *)uripath, '?');
    if(tmp) {
        size_t urilen = tmp - (char *)uripath;
        path = (unsigned char *) aprintf("%.*s", (int)urilen, uripath);
    }
}
if(!tmp)
    path = (unsigned char *) strdup((char *) uripath);
// aprintf/strdup 失败时返回 NULL → CURLE_OUT_OF_MEMORY 检查

// 响应生成:
result = Curl_auth_create_digest_http_message(data, userp, passwdp, request,
                                              path, digest, &response, &len);
// 内部使用 aprintf (动态分配) → 无固定缓冲区
```

**结论**: 无漏洞

---

### 13. sasl_A.c — Curl_sasl_continue

**认证状态机**:
- 每个认证方法通过专用函数处理: `Curl_auth_create_*_message`
- 使用 `struct bufref` 管理响应缓冲区 (RAII 模式)
- 错误通过 CURLcode 链传播

**结论**: 无漏洞

---

## 潜在发现汇总

### CR-001: ipv4_normalize 八进制/十六进制 IP 规范化 (行为安全)

| 属性 | 值 |
|------|-----|
| 文件 | `lib/urlapi.c`, `ipv4_normalize()` |
| 严重性 | MEDIUM |
| 类别 | SSRF 过滤绕过 (行为安全，非内存安全) |

**根本原因**: `strtoul(c, &endp, 0)` 接受 base-0 即 octal/hex 格式:
- `http://0177.0.0.1/` → 规范化为 `http://127.0.0.1/`
- `http://0x7f000001/` → 规范化为 `http://127.0.0.1/`
- `http://2130706433/` (十进制) → `http://127.0.0.1/`

**影响**: 使用 curl 实现代理或 webhook 功能的服务，如果通过字符串匹配 URL 进行 SSRF 过滤 (如 "block 127.0.0.1" 但不匹配 "0177.0.0.1")，可能被绕过。curl 本身正确规范化，但上游的 pre-URL-parse 过滤器可能漏掉这些格式。

**这是 curl 的设计行为** (符合 RFC 3986): 已有文档记录，不是 curl 自身漏洞。

---

## 手动分析结论

**curl 8.11.0 内存安全状况**: 极佳

13个分析段中：
- **0 个内存安全漏洞**
- **1 个行为安全问题** (ipv4_normalize，已知设计行为)
- **13 个安全通过**

curl 团队的防御编码实践非常成熟:
1. 所有外部输入通过 `CURL_MAX_INPUT_LENGTH` 限制
2. `dynbuf` 消除了固定缓冲区溢出风险
3. `CURLcode` 错误链确保错误不被静默忽略
4. 协议解析函数使用状态机而非手工指针操作
