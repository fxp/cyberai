// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 17/20



/**
 * bpf_prog_array_delete_safe_at() - Replaces the program at the given
 *                                   index into the program array with
 *                                   a dummy no-op program.
 * @array: a bpf_prog_array
 * @index: the index of the program to replace
 *
 * Skips over dummy programs, by not counting them, when calculating
 * the position of the program to replace.
 *
 * Return:
 * * 0		- Success
 * * -EINVAL	- Invalid index value. Must be a non-negative integer.
 * * -ENOENT	- Index out of range
 */
int bpf_prog_array_delete_safe_at(struct bpf_prog_array *array, int index)
{
	return bpf_prog_array_update_at(array, index, &dummy_bpf_prog.prog);
}

/**
 * bpf_prog_array_update_at() - Updates the program at the given index
 *                              into the program array.
 * @array: a bpf_prog_array
 * @index: the index of the program to update
 * @prog: the program to insert into the array
 *
 * Skips over dummy programs, by not counting them, when calculating
 * the position of the program to update.
 *
 * Return:
 * * 0		- Success
 * * -EINVAL	- Invalid index value. Must be a non-negative integer.
 * * -ENOENT	- Index out of range
 */
int bpf_prog_array_update_at(struct bpf_prog_array *array, int index,
			     struct bpf_prog *prog)
{
	struct bpf_prog_array_item *item;

	if (unlikely(index < 0))
		return -EINVAL;

	for (item = array->items; item->prog; item++) {
		if (item->prog == &dummy_bpf_prog.prog)
			continue;
		if (!index) {
			WRITE_ONCE(item->prog, prog);
			return 0;
		}
		index--;
	}
	return -ENOENT;
}

int bpf_prog_array_copy(struct bpf_prog_array *old_array,
			struct bpf_prog *exclude_prog,
			struct bpf_prog *include_prog,
			u64 bpf_cookie,
			struct bpf_prog_array **new_array)
{
	int new_prog_cnt, carry_prog_cnt = 0;
	struct bpf_prog_array_item *existing, *new;
	struct bpf_prog_array *array;
	bool found_exclude = false;

	/* Figure out how many existing progs we need to carry over to
	 * the new array.
	 */
	if (old_array) {
		existing = old_array->items;
		for (; existing->prog; existing++) {
			if (existing->prog == exclude_prog) {
				found_exclude = true;
				continue;
			}
			if (existing->prog != &dummy_bpf_prog.prog)
				carry_prog_cnt++;
			if (existing->prog == include_prog)
				return -EEXIST;
		}
	}

	if (exclude_prog && !found_exclude)
		return -ENOENT;

	/* How many progs (not NULL) will be in the new array? */
	new_prog_cnt = carry_prog_cnt;
	if (include_prog)
		new_prog_cnt += 1;

	/* Do we have any prog (not NULL) in the new array? */
	if (!new_prog_cnt) {
		*new_array = NULL;
		return 0;
	}

	/* +1 as the end of prog_array is marked with NULL */
	array = bpf_prog_array_alloc(new_prog_cnt + 1, GFP_KERNEL);
	if (!array)
		return -ENOMEM;
	new = array->items;

	/* Fill in the new prog array */
	if (carry_prog_cnt) {
		existing = old_array->items;
		for (; existing->prog; existing++) {
			if (existing->prog == exclude_prog ||
			    existing->prog == &dummy_bpf_prog.prog)
				continue;

			new->prog = existing->prog;
			new->bpf_cookie = existing->bpf_cookie;
			new++;
		}
	}
	if (include_prog) {
		new->prog = include_prog;
		new->bpf_cookie = bpf_cookie;
		new++;
	}
	new->prog = NULL;
	*new_array = array;
	return 0;
}

int bpf_prog_array_copy_info(struct bpf_prog_array *array,
			     u32 *prog_ids, u32 request_cnt,
			     u32 *prog_cnt)
{
	u32 cnt = 0;

	if (array)
		cnt = bpf_prog_array_length(array);

	*prog_cnt = cnt;

	/* return early if user requested only program count or nothing to copy */
	if (!request_cnt || !cnt)
		return 0;

	/* this function is called under trace/bpf_trace.c: bpf_event_mutex */
	return bpf_prog_array_copy_core(array, prog_ids, request_cnt) ? -ENOSPC
								     : 0;
}

void __bpf_free_used_maps(struct bpf_prog_aux *aux,
			  struct bpf_map **used_maps, u32 len)
{
	struct bpf_map *map;
	bool sleepable;
	u32 i;

	sleepable = aux->prog->sleepable;
	for (i = 0; i < len; i++) {
		map = used_maps[i];
		if (map->ops->map_poke_untrack)
			map->ops->map_poke_untrack(map, aux);
		if (sleepable)
			atomic64_dec(&map->sleepable_refcnt);
		bpf_map_put(map);
	}
}

static void bpf_free_used_maps(struct bpf_prog_aux *aux)
{
	__bpf_free_used_maps(aux, aux->used_maps, aux->used_map_cnt);
	kfree(aux->used_maps);
}

void __bpf_free_used_btfs(struct btf_mod_pair *used_btfs, u32 len)
{
#ifdef CONFIG_BPF_SYSCALL
	struct btf_mod_pair *btf_mod;
	u32 i;

	for (i = 0; i < len; i++) {
		btf_mod = &used_btfs[i];
		if (btf_mod->module)
			module_put(btf_mod->module);
		btf_put(btf_mod->btf);
	}
#endif
}

static void bpf_free_used_btfs(struct bpf_prog_aux *aux)
{
	__bpf_free_used_btfs(aux->used_btfs, aux->used_btf_cnt);
	kfree(aux->used_btfs);
}

static void bpf_prog_free_deferred(struct work_struct *work)
{
	struct bpf_prog_aux *aux;
	int i;