// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 35/55



#define BPF_MAP_TYPE(_id, _ops)
#define BPF_LINK_TYPE(_id, _name)
static union {
	struct bpf_ctx_convert {
#define BPF_PROG_TYPE(_id, _name, prog_ctx_type, kern_ctx_type) \
	prog_ctx_type _id##_prog; \
	kern_ctx_type _id##_kern;
#include <linux/bpf_types.h>
#undef BPF_PROG_TYPE
	} *__t;
	/* 't' is written once under lock. Read many times. */
	const struct btf_type *t;
} bpf_ctx_convert;
enum {
#define BPF_PROG_TYPE(_id, _name, prog_ctx_type, kern_ctx_type) \
	__ctx_convert##_id,
#include <linux/bpf_types.h>
#undef BPF_PROG_TYPE
	__ctx_convert_unused, /* to avoid empty enum in extreme .config */
};
static u8 bpf_ctx_convert_map[] = {
#define BPF_PROG_TYPE(_id, _name, prog_ctx_type, kern_ctx_type) \
	[_id] = __ctx_convert##_id,
#include <linux/bpf_types.h>
#undef BPF_PROG_TYPE
	0, /* avoid empty array */
};
#undef BPF_MAP_TYPE
#undef BPF_LINK_TYPE

static const struct btf_type *find_canonical_prog_ctx_type(enum bpf_prog_type prog_type)
{
	const struct btf_type *conv_struct;
	const struct btf_member *ctx_type;

	conv_struct = bpf_ctx_convert.t;
	if (!conv_struct)
		return NULL;
	/* prog_type is valid bpf program type. No need for bounds check. */
	ctx_type = btf_type_member(conv_struct) + bpf_ctx_convert_map[prog_type] * 2;
	/* ctx_type is a pointer to prog_ctx_type in vmlinux.
	 * Like 'struct __sk_buff'
	 */
	return btf_type_by_id(btf_vmlinux, ctx_type->type);
}

static int find_kern_ctx_type_id(enum bpf_prog_type prog_type)
{
	const struct btf_type *conv_struct;
	const struct btf_member *ctx_type;

	conv_struct = bpf_ctx_convert.t;
	if (!conv_struct)
		return -EFAULT;
	/* prog_type is valid bpf program type. No need for bounds check. */
	ctx_type = btf_type_member(conv_struct) + bpf_ctx_convert_map[prog_type] * 2 + 1;
	/* ctx_type is a pointer to prog_ctx_type in vmlinux.
	 * Like 'struct sk_buff'
	 */
	return ctx_type->type;
}

bool btf_is_projection_of(const char *pname, const char *tname)
{
	if (strcmp(pname, "__sk_buff") == 0 && strcmp(tname, "sk_buff") == 0)
		return true;
	if (strcmp(pname, "xdp_md") == 0 && strcmp(tname, "xdp_buff") == 0)
		return true;
	return false;
}

bool btf_is_prog_ctx_type(struct bpf_verifier_log *log, const struct btf *btf,
			  const struct btf_type *t, enum bpf_prog_type prog_type,
			  int arg)
{
	const struct btf_type *ctx_type;
	const char *tname, *ctx_tname;

	t = btf_type_by_id(btf, t->type);

	/* KPROBE programs allow bpf_user_pt_regs_t typedef, which we need to
	 * check before we skip all the typedef below.
	 */
	if (prog_type == BPF_PROG_TYPE_KPROBE) {
		while (btf_type_is_modifier(t) && !btf_type_is_typedef(t))
			t = btf_type_by_id(btf, t->type);

		if (btf_type_is_typedef(t)) {
			tname = btf_name_by_offset(btf, t->name_off);
			if (tname && strcmp(tname, "bpf_user_pt_regs_t") == 0)
				return true;
		}
	}

	while (btf_type_is_modifier(t))
		t = btf_type_by_id(btf, t->type);
	if (!btf_type_is_struct(t)) {
		/* Only pointer to struct is supported for now.
		 * That means that BPF_PROG_TYPE_TRACEPOINT with BTF
		 * is not supported yet.
		 * BPF_PROG_TYPE_RAW_TRACEPOINT is fine.
		 */
		return false;
	}
	tname = btf_name_by_offset(btf, t->name_off);
	if (!tname) {
		bpf_log(log, "arg#%d struct doesn't have a name\n", arg);
		return false;
	}

	ctx_type = find_canonical_prog_ctx_type(prog_type);
	if (!ctx_type) {
		bpf_log(log, "btf_vmlinux is malformed\n");
		/* should not happen */
		return false;
	}
again:
	ctx_tname = btf_name_by_offset(btf_vmlinux, ctx_type->name_off);
	if (!ctx_tname) {
		/* should not happen */
		bpf_log(log, "Please fix kernel include/linux/bpf_types.h\n");
		return false;
	}
	/* program types without named context types work only with arg:ctx tag */
	if (ctx_tname[0] == '\0')
		return false;
	/* only compare that prog's ctx type name is the same as
	 * kernel expects. No need to compare field by field.
	 * It's ok for bpf prog to do:
	 * struct __sk_buff {};
	 * int socket_filter_bpf_prog(struct __sk_buff *skb)
	 * { // no fields of skb are ever used }
	 */
	if (btf_is_projection_of(ctx_tname, tname))
		return true;
	if (strcmp(ctx_tname, tname)) {
		/* bpf_user_pt_regs_t is a typedef, so resolve it to
		 * underlying struct and check name again
		 */
		if (!btf_type_is_modifier(ctx_type))
			return false;
		while (btf_type_is_modifier(ctx_type))
			ctx_type = btf_type_by_id(btf_vmlinux, ctx_type->type);
		goto again;
	}
	return true;
}

/* forward declarations for arch-specific underlying types of
 * bpf_user_pt_regs_t; this avoids the need for arch-specific #ifdef
 * compilation guards below for BPF_PROG_TYPE_PERF_EVENT checks, but still
 * works correctly with __builtin_types_compatible_p() on respective
 * architectures
 */
struct user_regs_struct;
struct user_pt_regs;