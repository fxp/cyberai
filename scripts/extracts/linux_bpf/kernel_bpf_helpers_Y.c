// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 25/30



/**
 * bpf_lookup_system_key - lookup a key by a system-defined ID
 * @id: key ID
 *
 * Obtain a bpf_key structure with a key pointer set to the passed key ID.
 * The key pointer is marked as invalid, to prevent bpf_key_put() from
 * attempting to decrement the key reference count on that pointer. The key
 * pointer set in such way is currently understood only by
 * verify_pkcs7_signature().
 *
 * Set *id* to one of the values defined in include/linux/verification.h:
 * 0 for the primary keyring (immutable keyring of system keys);
 * VERIFY_USE_SECONDARY_KEYRING for both the primary and secondary keyring
 * (where keys can be added only if they are vouched for by existing keys
 * in those keyrings); VERIFY_USE_PLATFORM_KEYRING for the platform
 * keyring (primarily used by the integrity subsystem to verify a kexec'ed
 * kerned image and, possibly, the initramfs signature).
 *
 * Return: a bpf_key pointer with an invalid key pointer set from the
 *         pre-determined ID on success, a NULL pointer otherwise
 */
__bpf_kfunc struct bpf_key *bpf_lookup_system_key(u64 id)
{
	struct bpf_key *bkey;

	if (system_keyring_id_check(id) < 0)
		return NULL;

	bkey = kmalloc_obj(*bkey, GFP_ATOMIC);
	if (!bkey)
		return NULL;

	bkey->key = (struct key *)(unsigned long)id;
	bkey->has_ref = false;

	return bkey;
}

/**
 * bpf_key_put - decrement key reference count if key is valid and free bpf_key
 * @bkey: bpf_key structure
 *
 * Decrement the reference count of the key inside *bkey*, if the pointer
 * is valid, and free *bkey*.
 */
__bpf_kfunc void bpf_key_put(struct bpf_key *bkey)
{
	if (bkey->has_ref)
		key_put(bkey->key);

	kfree(bkey);
}

/**
 * bpf_verify_pkcs7_signature - verify a PKCS#7 signature
 * @data_p: data to verify
 * @sig_p: signature of the data
 * @trusted_keyring: keyring with keys trusted for signature verification
 *
 * Verify the PKCS#7 signature *sig_ptr* against the supplied *data_ptr*
 * with keys in a keyring referenced by *trusted_keyring*.
 *
 * Return: 0 on success, a negative value on error.
 */
__bpf_kfunc int bpf_verify_pkcs7_signature(struct bpf_dynptr *data_p,
			       struct bpf_dynptr *sig_p,
			       struct bpf_key *trusted_keyring)
{
#ifdef CONFIG_SYSTEM_DATA_VERIFICATION
	struct bpf_dynptr_kern *data_ptr = (struct bpf_dynptr_kern *)data_p;
	struct bpf_dynptr_kern *sig_ptr = (struct bpf_dynptr_kern *)sig_p;
	const void *data, *sig;
	u32 data_len, sig_len;
	int ret;

	if (trusted_keyring->has_ref) {
		/*
		 * Do the permission check deferred in bpf_lookup_user_key().
		 * See bpf_lookup_user_key() for more details.
		 *
		 * A call to key_task_permission() here would be redundant, as
		 * it is already done by keyring_search() called by
		 * find_asymmetric_key().
		 */
		ret = key_validate(trusted_keyring->key);
		if (ret < 0)
			return ret;
	}

	data_len = __bpf_dynptr_size(data_ptr);
	data = __bpf_dynptr_data(data_ptr, data_len);
	sig_len = __bpf_dynptr_size(sig_ptr);
	sig = __bpf_dynptr_data(sig_ptr, sig_len);

	return verify_pkcs7_signature(data, data_len, sig, sig_len,
				      trusted_keyring->key,
				      VERIFYING_BPF_SIGNATURE, NULL,
				      NULL);
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_SYSTEM_DATA_VERIFICATION */
}
#endif /* CONFIG_KEYS */

typedef int (*bpf_task_work_callback_t)(struct bpf_map *map, void *key, void *value);

enum bpf_task_work_state {
	/* bpf_task_work is ready to be used */
	BPF_TW_STANDBY = 0,
	/* irq work scheduling in progress */
	BPF_TW_PENDING,
	/* task work scheduling in progress */
	BPF_TW_SCHEDULING,
	/* task work is scheduled successfully */
	BPF_TW_SCHEDULED,
	/* callback is running */
	BPF_TW_RUNNING,
	/* associated BPF map value is deleted */
	BPF_TW_FREED,
};

struct bpf_task_work_ctx {
	enum bpf_task_work_state state;
	refcount_t refcnt;
	struct callback_head work;
	struct irq_work irq_work;
	/* bpf_prog that schedules task work */
	struct bpf_prog *prog;
	/* task for which callback is scheduled */
	struct task_struct *task;
	/* the map and map value associated with this context */
	struct bpf_map *map;
	void *map_val;
	enum task_work_notify_mode mode;
	bpf_task_work_callback_t callback_fn;
	struct rcu_head rcu;
} __aligned(8);

/* Actual type for struct bpf_task_work */
struct bpf_task_work_kern {
	struct bpf_task_work_ctx *ctx;
};

static void bpf_task_work_ctx_reset(struct bpf_task_work_ctx *ctx)
{
	if (ctx->prog) {
		bpf_prog_put(ctx->prog);
		ctx->prog = NULL;
	}
	if (ctx->task) {
		bpf_task_release(ctx->task);
		ctx->task = NULL;
	}
}

static bool bpf_task_work_ctx_tryget(struct bpf_task_work_ctx *ctx)
{
	return refcount_inc_not_zero(&ctx->refcnt);
}

static void bpf_task_work_destroy(struct irq_work *irq_work)
{
	struct bpf_task_work_ctx *ctx = container_of(irq_work, struct bpf_task_work_ctx, irq_work);

	bpf_task_work_ctx_reset(ctx);
	kfree_rcu(ctx, rcu);
}

static void bpf_task_work_ctx_put(struct bpf_task_work_ctx *ctx)
{
	if (!refcount_dec_and_test(&ctx->refcnt))
		return;