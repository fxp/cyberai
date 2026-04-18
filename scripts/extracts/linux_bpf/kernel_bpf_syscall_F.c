// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 6/37



			if (!btf_is_kernel(field->kptr.btf)) {
				pointee_struct_meta = btf_find_struct_meta(field->kptr.btf,
									   field->kptr.btf_id);
				__bpf_obj_drop_impl(xchgd_field, pointee_struct_meta ?
								 pointee_struct_meta->record : NULL,
								 fields[i].type == BPF_KPTR_PERCPU);
			} else {
				field->kptr.dtor(xchgd_field);
			}
			break;
		case BPF_UPTR:
			/* The caller ensured that no one is using the uptr */
			unpin_uptr_kaddr(*(void **)field_ptr);
			break;
		case BPF_LIST_HEAD:
			if (WARN_ON_ONCE(rec->spin_lock_off < 0))
				continue;
			bpf_list_head_free(field, field_ptr, obj + rec->spin_lock_off);
			break;
		case BPF_RB_ROOT:
			if (WARN_ON_ONCE(rec->spin_lock_off < 0))
				continue;
			bpf_rb_root_free(field, field_ptr, obj + rec->spin_lock_off);
			break;
		case BPF_LIST_NODE:
		case BPF_RB_NODE:
		case BPF_REFCOUNT:
			break;
		default:
			WARN_ON_ONCE(1);
			continue;
		}
	}
}

static void bpf_map_free(struct bpf_map *map)
{
	struct btf_record *rec = map->record;
	struct btf *btf = map->btf;

	/* implementation dependent freeing. Disabling migration to simplify
	 * the free of values or special fields allocated from bpf memory
	 * allocator.
	 */
	kfree(map->excl_prog_sha);
	migrate_disable();
	map->ops->map_free(map);
	migrate_enable();

	/* Delay freeing of btf_record for maps, as map_free
	 * callback usually needs access to them. It is better to do it here
	 * than require each callback to do the free itself manually.
	 *
	 * Note that the btf_record stashed in map->inner_map_meta->record was
	 * already freed using the map_free callback for map in map case which
	 * eventually calls bpf_map_free_meta, since inner_map_meta is only a
	 * template bpf_map struct used during verification.
	 */
	btf_record_free(rec);
	/* Delay freeing of btf for maps, as map_free callback may need
	 * struct_meta info which will be freed with btf_put().
	 */
	btf_put(btf);
}

/* called from workqueue */
static void bpf_map_free_deferred(struct work_struct *work)
{
	struct bpf_map *map = container_of(work, struct bpf_map, work);

	security_bpf_map_free(map);
	bpf_map_release_memcg(map);
	bpf_map_owner_free(map);
	bpf_map_free(map);
}

static void bpf_map_put_uref(struct bpf_map *map)
{
	if (atomic64_dec_and_test(&map->usercnt)) {
		if (map->ops->map_release_uref)
			map->ops->map_release_uref(map);
	}
}

static void bpf_map_free_in_work(struct bpf_map *map)
{
	INIT_WORK(&map->work, bpf_map_free_deferred);
	/* Avoid spawning kworkers, since they all might contend
	 * for the same mutex like slab_mutex.
	 */
	queue_work(system_dfl_wq, &map->work);
}

static void bpf_map_free_rcu_gp(struct rcu_head *rcu)
{
	bpf_map_free_in_work(container_of(rcu, struct bpf_map, rcu));
}

/* decrement map refcnt and schedule it for freeing via workqueue
 * (underlying map implementation ops->map_free() might sleep)
 */
void bpf_map_put(struct bpf_map *map)
{
	if (atomic64_dec_and_test(&map->refcnt)) {
		/* bpf_map_free_id() must be called first */
		bpf_map_free_id(map);

		WARN_ON_ONCE(atomic64_read(&map->sleepable_refcnt));
		/* RCU tasks trace grace period implies RCU grace period. */
		if (READ_ONCE(map->free_after_mult_rcu_gp))
			call_rcu_tasks_trace(&map->rcu, bpf_map_free_rcu_gp);
		else if (READ_ONCE(map->free_after_rcu_gp))
			call_rcu(&map->rcu, bpf_map_free_rcu_gp);
		else
			bpf_map_free_in_work(map);
	}
}
EXPORT_SYMBOL_GPL(bpf_map_put);

void bpf_map_put_with_uref(struct bpf_map *map)
{
	bpf_map_put_uref(map);
	bpf_map_put(map);
}

static int bpf_map_release(struct inode *inode, struct file *filp)
{
	struct bpf_map *map = filp->private_data;

	if (map->ops->map_release)
		map->ops->map_release(map, filp);

	bpf_map_put_with_uref(map);
	return 0;
}

static fmode_t map_get_sys_perms(struct bpf_map *map, struct fd f)
{
	fmode_t mode = fd_file(f)->f_mode;

	/* Our file permissions may have been overridden by global
	 * map permissions facing syscall side.
	 */
	if (READ_ONCE(map->frozen))
		mode &= ~FMODE_CAN_WRITE;
	return mode;
}

#ifdef CONFIG_PROC_FS
/* Show the memory usage of a bpf map */
static u64 bpf_map_memory_usage(const struct bpf_map *map)
{
	return map->ops->map_mem_usage(map);
}

static void bpf_map_show_fdinfo(struct seq_file *m, struct file *filp)
{
	struct bpf_map *map = filp->private_data;
	u32 type = 0, jited = 0;

	spin_lock(&map->owner_lock);
	if (map->owner) {
		type  = map->owner->type;
		jited = map->owner->jited;
	}
	spin_unlock(&map->owner_lock);