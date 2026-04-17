# libexpat 2.6.4 — 手动代码分析

**目标**: libexpat 2.6.4
**分析日期**: 2026-04-17
**源码**: `/Users/xiaopingfeng/Projects/cyberai/targets/expat-2.6.4/lib/xmlparse.c`

---

## 历史 CVE 修复验证

libexpat 2.4.3 修复了 6 个整数溢出漏洞（CVE-2022-22822 至 22827）。以下验证 2.6.4 中修复是否完整：

### lookup() — CVE-2022-22825 修复

```c
// L7231: 防止 shift 溢出
if (newPower >= sizeof(unsigned long) * 8) {
    return NULL;
}

// L7239: 防止 newSize * sizeof(NAMED*) 溢出  
if (newSize > (size_t)(-1) / sizeof(NAMED *)) {
    return NULL;
}
```

**状态**: ✅ 修复完整，显式注释 "Detect and prevent integer overflow"

### storeAtts() — CVE-2022-22827 修复

```c
// L3371: n + nDefaultAtts 加法溢出保护
if (n > INT_MAX - nDefaultAtts) return XML_ERROR_NO_MEMORY;

// L3383-3386: n + nDefaultAtts + INIT_ATTS_SIZE 溢出保护
if ((nDefaultAtts > INT_MAX - INIT_ATTS_SIZE)
    || (n > INT_MAX - (nDefaultAtts + INIT_ATTS_SIZE))) {
  return XML_ERROR_NO_MEMORY;
}

// L3394-3399: ATTRIBUTE 数组 size_t 乘法保护
#if UINT_MAX >= SIZE_MAX
if ((unsigned)parser->m_attsSize > (size_t)(-1) / sizeof(ATTRIBUTE)) {
  ...
}
#endif
```

**状态**: ✅ 修复完整

### addBinding() — CVE-2022-22822 修复

```c
// L4011: len + EXPAND_SPARE 加法保护
if (len > INT_MAX - EXPAND_SPARE) return XML_ERROR_NO_MEMORY;

// L4019-4023: XML_Char 数组乘法保护
#if UINT_MAX >= SIZE_MAX
if ((unsigned)(len + EXPAND_SPARE) > (size_t)(-1) / sizeof(XML_Char)) {
  return XML_ERROR_NO_MEMORY;
}
#endif
```

**状态**: ✅ 修复完整

---

## 全局安全特性

| 机制 | 描述 |
|------|------|
| 显式溢出守护 | 每个整数运算前均有 `if (x > INT_MAX - y)` 检查 |
| `accountingDiffTolerated()` | 递归实体展开放大系数限制 (默认 100倍) |
| `XML_SetBillionLaughsAttackProtectionMaximumAmplification()` | 可配置的实体展开上限 |
| malloc 返回值检查 | 所有 MALLOC/REALLOC 返回 NULL 均处理 |
| 解析状态机 | `prologProcessor`/`contentProcessor` 防止非法状态转换 |

---

## 分析结论

**初步判断**: libexpat 2.6.4 在历史 CVE 修复方面非常彻底，显式的溢出守护注释遍布代码。

**新漏洞候选**（低概率，GLM 扫描后验证）：
1. `accountingDiffTolerated` 的实体计数器可能在极端嵌套下绕过
2. `processXmlDecl` 的版本字符串解析边界（极小字符串）
3. 超长命名空间分隔符位置的边界条件

**预期 FP 模式**（GLM 会误报）：
- 已有溢出守护的乘法/加法操作
- realloc 后旧指针使用（expat 有完整的 fixup 逻辑）
- `len` 变量的负值假设（len 从 URI 扫描获得，非负）

---

*GLM 扫描将在 API 配额恢复后执行，结果写入 vulnerability_report.md*
