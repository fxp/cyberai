// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 6/30



			*tmp_buf = raw_args[num_spec];
			tmp_buf++;
			num_spec++;

			continue;
		}

		sizeof_cur_arg = sizeof(int);

		if (fmt[i] == 'l') {
			sizeof_cur_arg = sizeof(long);
			i++;
		}
		if (fmt[i] == 'l') {
			sizeof_cur_arg = sizeof(long long);
			i++;
		}

		if (fmt[i] != 'i' && fmt[i] != 'd' && fmt[i] != 'u' &&
		    fmt[i] != 'x' && fmt[i] != 'X') {
			err = -EINVAL;
			goto out;
		}

		if (tmp_buf)
			cur_arg = raw_args[num_spec];
nocopy_fmt:
		if (tmp_buf) {
			tmp_buf = PTR_ALIGN(tmp_buf, sizeof(u32));
			if (tmp_buf_end - tmp_buf < sizeof_cur_arg) {
				err = -ENOSPC;
				goto out;
			}

			if (sizeof_cur_arg == 8) {
				*(u32 *)tmp_buf = *(u32 *)&cur_arg;
				*(u32 *)(tmp_buf + 4) = *((u32 *)&cur_arg + 1);
			} else {
				*(u32 *)tmp_buf = (u32)(long)cur_arg;
			}
			tmp_buf += sizeof_cur_arg;
		}
		num_spec++;
	}

	err = 0;
out:
	if (err)
		bpf_bprintf_cleanup(data);
	return err;
}

BPF_CALL_5(bpf_snprintf, char *, str, u32, str_size, char *, fmt,
	   const void *, args, u32, data_len)
{
	struct bpf_bprintf_data data = {
		.get_bin_args	= true,
	};
	int err, num_args;

	if (data_len % 8 || data_len > MAX_BPRINTF_VARARGS * 8 ||
	    (data_len && !args))
		return -EINVAL;
	num_args = data_len / 8;

	/* ARG_PTR_TO_CONST_STR guarantees that fmt is zero-terminated so we
	 * can safely give an unbounded size.
	 */
	err = bpf_bprintf_prepare(fmt, UINT_MAX, args, num_args, &data);
	if (err < 0)
		return err;

	err = bstr_printf(str, str_size, fmt, data.bin_args);

	bpf_bprintf_cleanup(&data);

	return err + 1;
}

const struct bpf_func_proto bpf_snprintf_proto = {
	.func		= bpf_snprintf,
	.gpl_only	= true,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_MEM_OR_NULL | MEM_WRITE,
	.arg2_type	= ARG_CONST_SIZE_OR_ZERO,
	.arg3_type	= ARG_PTR_TO_CONST_STR,
	.arg4_type	= ARG_PTR_TO_MEM | PTR_MAYBE_NULL | MEM_RDONLY,
	.arg5_type	= ARG_CONST_SIZE_OR_ZERO,
};

static void *map_key_from_value(struct bpf_map *map, void *value, u32 *arr_idx)
{
	if (map->map_type == BPF_MAP_TYPE_ARRAY) {
		struct bpf_array *array = container_of(map, struct bpf_array, map);

		*arr_idx = ((char *)value - array->value) / array->elem_size;
		return arr_idx;
	}
	return (void *)value - round_up(map->key_size, 8);
}

enum bpf_async_type {
	BPF_ASYNC_TYPE_TIMER = 0,
	BPF_ASYNC_TYPE_WQ,
};

enum bpf_async_op {
	BPF_ASYNC_START,
	BPF_ASYNC_CANCEL
};

struct bpf_async_cmd {
	struct llist_node node;
	u64 nsec;
	u32 mode;
	enum bpf_async_op op;
};

struct bpf_async_cb {
	struct bpf_map *map;
	struct bpf_prog *prog;
	void __rcu *callback_fn;
	void *value;
	struct rcu_head rcu;
	u64 flags;
	struct irq_work worker;
	refcount_t refcnt;
	enum bpf_async_type type;
	struct llist_head async_cmds;
};

/* BPF map elements can contain 'struct bpf_timer'.
 * Such map owns all of its BPF timers.
 * 'struct bpf_timer' is allocated as part of map element allocation
 * and it's zero initialized.
 * That space is used to keep 'struct bpf_async_kern'.
 * bpf_timer_init() allocates 'struct bpf_hrtimer', inits hrtimer, and
 * remembers 'struct bpf_map *' pointer it's part of.
 * bpf_timer_set_callback() increments prog refcnt and assign bpf callback_fn.
 * bpf_timer_start() arms the timer.
 * If user space reference to a map goes to zero at this point
 * ops->map_release_uref callback is responsible for cancelling the timers,
 * freeing their memory, and decrementing prog's refcnts.
 * bpf_timer_cancel() cancels the timer and decrements prog's refcnt.
 * Inner maps can contain bpf timers as well. ops->map_release_uref is
 * freeing the timers when inner map is replaced or deleted by user space.
 */
struct bpf_hrtimer {
	struct bpf_async_cb cb;
	struct hrtimer timer;
	atomic_t cancelling;
};

struct bpf_work {
	struct bpf_async_cb cb;
	struct work_struct work;
};

/* the actual struct hidden inside uapi struct bpf_timer and bpf_wq */
struct bpf_async_kern {
	union {
		struct bpf_async_cb *cb;
		struct bpf_hrtimer *timer;
		struct bpf_work *work;
	};
} __attribute__((aligned(8)));

static DEFINE_PER_CPU(struct bpf_hrtimer *, hrtimer_running);

static void bpf_async_refcount_put(struct bpf_async_cb *cb);

static enum hrtimer_restart bpf_timer_cb(struct hrtimer *hrtimer)
{
	struct bpf_hrtimer *t = container_of(hrtimer, struct bpf_hrtimer, timer);
	struct bpf_map *map = t->cb.map;
	void *value = t->cb.value;
	bpf_callback_t callback_fn;
	void *key;
	u32 idx;

	BTF_TYPE_EMIT(struct bpf_timer);
	callback_fn = rcu_dereference_check(t->cb.callback_fn, rcu_read_lock_bh_held());
	if (!callback_fn)
		goto out;

	/* bpf_timer_cb() runs in hrtimer_run_softirq. It doesn't migrate and
	 * cannot be preempted by another bpf_timer_cb() on the same cpu.
	 * Remember the timer this callback is servicing to prevent
	 * deadlock if callback_fn() calls bpf_timer_cancel() or
	 * bpf_map_delete_elem() on the same timer.
	 */
	this_cpu_write(hrtimer_running, t);

	key = map_key_from_value(map, value, &idx);