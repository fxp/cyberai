// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 14/30



/**
 * bpf_percpu_obj_new() - allocate a percpu object described by program BTF
 * @local_type_id__k: type ID in program BTF
 * @meta: verifier-supplied struct metadata
 *
 * Allocate a percpu object of the type identified by @local_type_id__k. BPF
 * programs can use bpf_core_type_id_local() to provide @local_type_id__k.
 * The verifier rewrites @meta; BPF programs do not set it.
 *
 * Return: Pointer to the allocated percpu object, or %NULL on failure.
 */
__bpf_kfunc void *bpf_percpu_obj_new(u64 local_type_id__k, struct btf_struct_meta *meta)
{
	u64 size = local_type_id__k;

	/* The verifier has ensured that meta must be NULL */
	return bpf_mem_alloc(&bpf_global_percpu_ma, size);
}

__bpf_kfunc void *bpf_percpu_obj_new_impl(u64 local_type_id__k, void *meta__ign)
{
	return bpf_percpu_obj_new(local_type_id__k, meta__ign);
}

/* Must be called under migrate_disable(), as required by bpf_mem_free */
void __bpf_obj_drop_impl(void *p, const struct btf_record *rec, bool percpu)
{
	struct bpf_mem_alloc *ma;

	if (rec && rec->refcount_off >= 0 &&
	    !refcount_dec_and_test((refcount_t *)(p + rec->refcount_off))) {
		/* Object is refcounted and refcount_dec didn't result in 0
		 * refcount. Return without freeing the object
		 */
		return;
	}

	if (rec)
		bpf_obj_free_fields(rec, p);

	if (percpu)
		ma = &bpf_global_percpu_ma;
	else
		ma = &bpf_global_ma;
	bpf_mem_free_rcu(ma, p);
}

/**
 * bpf_obj_drop() - drop a previously allocated object
 * @p__alloc: object to free
 * @meta: verifier-supplied struct metadata
 *
 * Destroy special fields in @p__alloc as needed and free the object. The
 * verifier rewrites @meta; BPF programs do not set it.
 */
__bpf_kfunc void bpf_obj_drop(void *p__alloc, struct btf_struct_meta *meta)
{
	void *p = p__alloc;

	__bpf_obj_drop_impl(p, meta ? meta->record : NULL, false);
}

__bpf_kfunc void bpf_obj_drop_impl(void *p__alloc, void *meta__ign)
{
	return bpf_obj_drop(p__alloc, meta__ign);
}

/**
 * bpf_percpu_obj_drop() - drop a previously allocated percpu object
 * @p__alloc: percpu object to free
 * @meta: verifier-supplied struct metadata
 *
 * Free @p__alloc. The verifier rewrites @meta; BPF programs do not set it.
 */
__bpf_kfunc void bpf_percpu_obj_drop(void *p__alloc, struct btf_struct_meta *meta)
{
	/* The verifier has ensured that meta must be NULL */
	bpf_mem_free_rcu(&bpf_global_percpu_ma, p__alloc);
}

__bpf_kfunc void bpf_percpu_obj_drop_impl(void *p__alloc, void *meta__ign)
{
	bpf_percpu_obj_drop(p__alloc, meta__ign);
}

/**
 * bpf_refcount_acquire() - turn a local kptr into an owning reference
 * @p__refcounted_kptr: non-owning local kptr
 * @meta: verifier-supplied struct metadata
 *
 * Increment the refcount for @p__refcounted_kptr. The verifier rewrites
 * @meta; BPF programs do not set it.
 *
 * Return: Owning reference to @p__refcounted_kptr, or %NULL on failure.
 */
__bpf_kfunc void *bpf_refcount_acquire(void *p__refcounted_kptr, struct btf_struct_meta *meta)
{
	struct bpf_refcount *ref;

	/* Could just cast directly to refcount_t *, but need some code using
	 * bpf_refcount type so that it is emitted in vmlinux BTF
	 */
	ref = (struct bpf_refcount *)(p__refcounted_kptr + meta->record->refcount_off);
	if (!refcount_inc_not_zero((refcount_t *)ref))
		return NULL;

	/* Verifier strips KF_RET_NULL if input is owned ref, see is_kfunc_ret_null
	 * in verifier.c
	 */
	return (void *)p__refcounted_kptr;
}

__bpf_kfunc void *bpf_refcount_acquire_impl(void *p__refcounted_kptr, void *meta__ign)
{
	return bpf_refcount_acquire(p__refcounted_kptr, meta__ign);
}

static int __bpf_list_add(struct bpf_list_node_kern *node,
			  struct bpf_list_head *head,
			  bool tail, struct btf_record *rec, u64 off)
{
	struct list_head *n = &node->list_head, *h = (void *)head;

	/* If list_head was 0-initialized by map, bpf_obj_init_field wasn't
	 * called on its fields, so init here
	 */
	if (unlikely(!h->next))
		INIT_LIST_HEAD(h);

	/* node->owner != NULL implies !list_empty(n), no need to separately
	 * check the latter
	 */
	if (cmpxchg(&node->owner, NULL, BPF_PTR_POISON)) {
		/* Only called from BPF prog, no need to migrate_disable */
		__bpf_obj_drop_impl((void *)n - off, rec, false);
		return -EINVAL;
	}

	tail ? list_add_tail(n, h) : list_add(n, h);
	WRITE_ONCE(node->owner, head);

	return 0;
}

/**
 * bpf_list_push_front() - add a node to the front of a BPF linked list
 * @head: list head
 * @node: node to insert
 * @meta: verifier-supplied struct metadata
 * @off: verifier-supplied offset of @node within the containing object
 *
 * Insert @node at the front of @head. The verifier rewrites @meta and @off;
 * BPF programs do not set them.
 *
 * Return: 0 on success, or %-EINVAL if @node is already linked.
 */
__bpf_kfunc int bpf_list_push_front(struct bpf_list_head *head,
				    struct bpf_list_node *node,
				    struct btf_struct_meta *meta,
				    u64 off)
{
	struct bpf_list_node_kern *n = (void *)node;