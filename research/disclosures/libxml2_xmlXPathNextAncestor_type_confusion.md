# libxml2 — `xmlXPathNextAncestor` namespace "type confusion" — ❌ FALSE POSITIVE (refuted by code semantics, 2026-05-20)

> **VERDICT: NOT A BUG.** The flagged cast of `xmlNsPtr` → `xmlNodePtr` in
> `xmlXPathNextAncestor` is **intentional, documented libxml2 design**, not a
> type confusion. For XPath namespace nodes, libxml2 deliberately repurposes
> the `xmlNs.next` field to hold a pointer to the **parent `xmlNode`**. The
> source says so directly (comment above `xmlXPathNodeSetCreate`):
>
> > "Namespace node in libxml don't match the XPath semantic. In a node set
> > the namespace nodes are duplicated and the next pointer is set to the
> > parent node in the XPath semantic."
>
> `xmlXPathNodeSetDupNs` implements exactly that: `cur->next = (xmlNsPtr) node;`
> (where `node` is the parent element). So when `xmlXPathNextAncestor` returns
> `ns->next` as the ancestor, it is returning a **valid parent `xmlNode`** — by
> contract, never another `xmlNs`. The matching teardown in
> `xmlXPathNodeSetFreeNs` relies on the same invariant
> (`ns->next->type != XML_NAMESPACE_DECL`).
>
> The model's exploit scenario ("if `ns->next` points to another `xmlNs` whose
> type field is corrupted…") **presupposes a separate, pre-existing
> memory-corruption primitive** that violates this invariant. That is circular:
> if you already have an arbitrary-write primitive corrupting the namespace
> list, you don't need this path. There is **no path from attacker-controlled
> XML input alone** to trigger it. Both the H reasoning
> (`exploitability: needs_specific_setup`) and the J3 cross-check
> (glm-4-plus, PARTIAL, "arbitrary memory read/write unlikely") already
> signalled this; the code-semantics read makes it conclusive.
>
> **Bonus error:** the J5 draft asserted "This is a known issue tracked as
> CVE-2023-39615." That is a **hallucinated attribution** — CVE-2023-39615 is
> an out-of-bounds read in `xmlSAX2StartElement` (`SAX2.c`), unrelated to
> `xmlXPathNextAncestor` or namespace nodes. Verified against NVD on 2026-05-20.
>
> **This finding will NOT be disclosed.** Kept as a documented negative result.

---

## Provenance

- Pipeline A flag: glm-5.1, scan run 2026-05-04, `xpath.c [xmlXPathNextAncestor]`
- H adversarial verify: glm-5.1, CONFIRMED, conf 0.95, `exploitability: needs_specific_setup`
- J3 cross-model: glm-4-plus, **PARTIAL**, code_match=yes, conf 0.8 ("exploitability questionable")
- J5 draft: generated 2026-05-07 by glm-5.1 (asserted CVE-2023-39615 — incorrect)
- Refutation: code-semantics verification against libxml2 `master` `xpath.c` + NVD
  CVE lookup, human review 2026-05-20.

## Why the static extract misled the model (same root cause as libpng)

The Pipeline A extract (`scripts/extracts/libxml2/xpath_axis_A.c`) showed the
`xmlXPathNextAncestor` cast in isolation. The invariant that makes the cast safe
— "XPath namespace nodes are duplicated with `next` = parent element" — is
established in *other* functions (`xmlXPathNodeSetDupNs`, `xmlXPathNodeSetCreate`)
that were not in the extract window. Reading one function out of context, both
GLM models and a naive human read see an unchecked downcast and call it type
confusion. Only reading the surrounding data-structure contract refutes it.

**Lesson (reinforces the libpng lesson):** memory-safety findings on a single
extracted function must be validated against the data-structure invariants
established elsewhere in the translation unit. Prefer whole-file (or
whole-subsystem) grounding before treating a cast/overflow as real.
