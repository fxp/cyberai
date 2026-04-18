// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 46/55



			kern_type_id = btf_get_ptr_to_btf_id(log, i, btf, t);
			if (kern_type_id < 0)
				return kern_type_id;

			sub->args[i].arg_type = ARG_PTR_TO_BTF_ID | PTR_TRUSTED;
			if (tags & ARG_TAG_NULLABLE)
				sub->args[i].arg_type |= PTR_MAYBE_NULL;
			sub->args[i].btf_id = kern_type_id;
			continue;
		}
		if (tags & ARG_TAG_UNTRUSTED) {
			struct btf *vmlinux_btf;
			int kern_type_id;

			if (tags & ~ARG_TAG_UNTRUSTED) {
				bpf_log(log, "arg#%d untrusted cannot be combined with any other tags\n", i);
				return -EINVAL;
			}

			ref_t = btf_type_skip_modifiers(btf, t->type, NULL);
			if (btf_type_is_void(ref_t) || btf_type_is_primitive(ref_t)) {
				sub->args[i].arg_type = ARG_PTR_TO_MEM | MEM_RDONLY | PTR_UNTRUSTED;
				sub->args[i].mem_size = 0;
				continue;
			}

			kern_type_id = btf_get_ptr_to_btf_id(log, i, btf, t);
			if (kern_type_id < 0)
				return kern_type_id;

			vmlinux_btf = bpf_get_btf_vmlinux();
			ref_t = btf_type_by_id(vmlinux_btf, kern_type_id);
			if (!btf_type_is_struct(ref_t)) {
				tname = __btf_name_by_offset(vmlinux_btf, t->name_off);
				bpf_log(log, "arg#%d has type %s '%s', but only struct or primitive types are allowed\n",
					i, btf_type_str(ref_t), tname);
				return -EINVAL;
			}
			sub->args[i].arg_type = ARG_PTR_TO_BTF_ID | PTR_UNTRUSTED;
			sub->args[i].btf_id = kern_type_id;
			continue;
		}
		if (tags & ARG_TAG_ARENA) {
			if (tags & ~ARG_TAG_ARENA) {
				bpf_log(log, "arg#%d arena cannot be combined with any other tags\n", i);
				return -EINVAL;
			}
			sub->args[i].arg_type = ARG_PTR_TO_ARENA;
			continue;
		}
		if (is_global) { /* generic user data pointer */
			u32 mem_size;

			if (tags & ARG_TAG_NULLABLE) {
				bpf_log(log, "arg#%d has invalid combination of tags\n", i);
				return -EINVAL;
			}

			t = btf_type_skip_modifiers(btf, t->type, NULL);
			ref_t = btf_resolve_size(btf, t, &mem_size);
			if (IS_ERR(ref_t)) {
				bpf_log(log, "arg#%d reference type('%s %s') size cannot be determined: %ld\n",
					i, btf_type_str(t), btf_name_by_offset(btf, t->name_off),
					PTR_ERR(ref_t));
				return -EINVAL;
			}

			sub->args[i].arg_type = ARG_PTR_TO_MEM | PTR_MAYBE_NULL;
			if (tags & ARG_TAG_NONNULL)
				sub->args[i].arg_type &= ~PTR_MAYBE_NULL;
			sub->args[i].mem_size = mem_size;
			continue;
		}

skip_pointer:
		if (tags) {
			bpf_log(log, "arg#%d has pointer tag, but is not a pointer type\n", i);
			return -EINVAL;
		}
		if (btf_type_is_int(t) || btf_is_any_enum(t)) {
			sub->args[i].arg_type = ARG_ANYTHING;
			continue;
		}
		if (!is_global)
			return -EINVAL;
		bpf_log(log, "Arg#%d type %s in %s() is not supported yet.\n",
			i, btf_type_str(t), tname);
		return -EINVAL;
	}

	sub->arg_cnt = nargs;
	sub->args_cached = true;

	return 0;
}

static void btf_type_show(const struct btf *btf, u32 type_id, void *obj,
			  struct btf_show *show)
{
	const struct btf_type *t = btf_type_by_id(btf, type_id);

	show->btf = btf;
	memset(&show->state, 0, sizeof(show->state));
	memset(&show->obj, 0, sizeof(show->obj));

	btf_type_ops(t)->show(btf, t, type_id, obj, 0, show);
}

__printf(2, 0) static void btf_seq_show(struct btf_show *show, const char *fmt,
					va_list args)
{
	seq_vprintf((struct seq_file *)show->target, fmt, args);
}

int btf_type_seq_show_flags(const struct btf *btf, u32 type_id,
			    void *obj, struct seq_file *m, u64 flags)
{
	struct btf_show sseq;

	sseq.target = m;
	sseq.showfn = btf_seq_show;
	sseq.flags = flags;

	btf_type_show(btf, type_id, obj, &sseq);

	return sseq.state.status;
}

void btf_type_seq_show(const struct btf *btf, u32 type_id, void *obj,
		       struct seq_file *m)
{
	(void) btf_type_seq_show_flags(btf, type_id, obj, m,
				       BTF_SHOW_NONAME | BTF_SHOW_COMPACT |
				       BTF_SHOW_ZERO | BTF_SHOW_UNSAFE);
}

struct btf_show_snprintf {
	struct btf_show show;
	int len_left;		/* space left in string */
	int len;		/* length we would have written */
};

__printf(2, 0) static void btf_snprintf_show(struct btf_show *show, const char *fmt,
					     va_list args)
{
	struct btf_show_snprintf *ssnprintf = (struct btf_show_snprintf *)show;
	int len;

	len = vsnprintf(show->target, ssnprintf->len_left, fmt, args);

	if (len < 0) {
		ssnprintf->len_left = 0;
		ssnprintf->len = len;
	} else if (len >= ssnprintf->len_left) {
		/* no space, drive on to get length we would have written */
		ssnprintf->len_left = 0;
		ssnprintf->len += len;
	} else {
		ssnprintf->len_left -= len;
		ssnprintf->len += len;
		show->target += len;
	}
}

int btf_type_snprintf_show(const struct btf *btf, u32 type_id, void *obj,
			   char *buf, int len, u64 flags)
{
	struct btf_show_snprintf ssnprintf;

	ssnprintf.show.target = buf;
	ssnprintf.show.flags = flags;
	ssnprintf.show.showfn = btf_snprintf_show;
	ssnprintf.len_left = len;
	ssnprintf.len = 0;

	btf_type_show(btf, type_id, obj, (struct btf_show *)&ssnprintf);

	/* If we encountered an error, return it. */
	if (ssnprintf.show.state.status)
		return ssnprintf.show.state.status;