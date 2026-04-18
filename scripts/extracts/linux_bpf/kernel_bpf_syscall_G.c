// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 7/37



	seq_printf(m,
		   "map_type:\t%u\n"
		   "key_size:\t%u\n"
		   "value_size:\t%u\n"
		   "max_entries:\t%u\n"
		   "map_flags:\t%#x\n"
		   "map_extra:\t%#llx\n"
		   "memlock:\t%llu\n"
		   "map_id:\t%u\n"
		   "frozen:\t%u\n",
		   map->map_type,
		   map->key_size,
		   map->value_size,
		   map->max_entries,
		   map->map_flags,
		   (unsigned long long)map->map_extra,
		   bpf_map_memory_usage(map),
		   map->id,
		   READ_ONCE(map->frozen));
	if (type) {
		seq_printf(m, "owner_prog_type:\t%u\n", type);
		seq_printf(m, "owner_jited:\t%u\n", jited);
	}
}
#endif

static ssize_t bpf_dummy_read(struct file *filp, char __user *buf, size_t siz,
			      loff_t *ppos)
{
	/* We need this handler such that alloc_file() enables
	 * f_mode with FMODE_CAN_READ.
	 */
	return -EINVAL;
}

static ssize_t bpf_dummy_write(struct file *filp, const char __user *buf,
			       size_t siz, loff_t *ppos)
{
	/* We need this handler such that alloc_file() enables
	 * f_mode with FMODE_CAN_WRITE.
	 */
	return -EINVAL;
}

/* called for any extra memory-mapped regions (except initial) */
static void bpf_map_mmap_open(struct vm_area_struct *vma)
{
	struct bpf_map *map = vma->vm_file->private_data;

	if (vma->vm_flags & VM_MAYWRITE)
		bpf_map_write_active_inc(map);
}

/* called for all unmapped memory region (including initial) */
static void bpf_map_mmap_close(struct vm_area_struct *vma)
{
	struct bpf_map *map = vma->vm_file->private_data;

	if (vma->vm_flags & VM_MAYWRITE)
		bpf_map_write_active_dec(map);
}

static const struct vm_operations_struct bpf_map_default_vmops = {
	.open		= bpf_map_mmap_open,
	.close		= bpf_map_mmap_close,
};

static int bpf_map_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct bpf_map *map = filp->private_data;
	int err = 0;

	if (!map->ops->map_mmap || !IS_ERR_OR_NULL(map->record))
		return -ENOTSUPP;

	if (!(vma->vm_flags & VM_SHARED))
		return -EINVAL;

	mutex_lock(&map->freeze_mutex);

	if (vma->vm_flags & VM_WRITE) {
		if (map->frozen) {
			err = -EPERM;
			goto out;
		}
		/* map is meant to be read-only, so do not allow mapping as
		 * writable, because it's possible to leak a writable page
		 * reference and allows user-space to still modify it after
		 * freezing, while verifier will assume contents do not change
		 */
		if (map->map_flags & BPF_F_RDONLY_PROG) {
			err = -EACCES;
			goto out;
		}
		bpf_map_write_active_inc(map);
	}
out:
	mutex_unlock(&map->freeze_mutex);
	if (err)
		return err;

	/* set default open/close callbacks */
	vma->vm_ops = &bpf_map_default_vmops;
	vma->vm_private_data = map;
	vm_flags_clear(vma, VM_MAYEXEC);
	/* If mapping is read-only, then disallow potentially re-mapping with
	 * PROT_WRITE by dropping VM_MAYWRITE flag. This VM_MAYWRITE clearing
	 * means that as far as BPF map's memory-mapped VMAs are concerned,
	 * VM_WRITE and VM_MAYWRITE and equivalent, if one of them is set,
	 * both should be set, so we can forget about VM_MAYWRITE and always
	 * check just VM_WRITE
	 */
	if (!(vma->vm_flags & VM_WRITE))
		vm_flags_clear(vma, VM_MAYWRITE);

	err = map->ops->map_mmap(map, vma);
	if (err) {
		if (vma->vm_flags & VM_WRITE)
			bpf_map_write_active_dec(map);
	}

	return err;
}

static __poll_t bpf_map_poll(struct file *filp, struct poll_table_struct *pts)
{
	struct bpf_map *map = filp->private_data;

	if (map->ops->map_poll)
		return map->ops->map_poll(map, filp, pts);

	return EPOLLERR;
}

static unsigned long bpf_get_unmapped_area(struct file *filp, unsigned long addr,
					   unsigned long len, unsigned long pgoff,
					   unsigned long flags)
{
	struct bpf_map *map = filp->private_data;

	if (map->ops->map_get_unmapped_area)
		return map->ops->map_get_unmapped_area(filp, addr, len, pgoff, flags);
#ifdef CONFIG_MMU
	return mm_get_unmapped_area(filp, addr, len, pgoff, flags);
#else
	return addr;
#endif
}

const struct file_operations bpf_map_fops = {
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= bpf_map_show_fdinfo,
#endif
	.release	= bpf_map_release,
	.read		= bpf_dummy_read,
	.write		= bpf_dummy_write,
	.mmap		= bpf_map_mmap,
	.poll		= bpf_map_poll,
	.get_unmapped_area = bpf_get_unmapped_area,
};

int bpf_map_new_fd(struct bpf_map *map, int flags)
{
	int ret;

	ret = security_bpf_map(map, OPEN_FMODE(flags));
	if (ret < 0)
		return ret;

	return anon_inode_getfd("bpf-map", &bpf_map_fops, map,
				flags | O_CLOEXEC);
}

int bpf_get_file_flag(int flags)
{
	if ((flags & BPF_F_RDONLY) && (flags & BPF_F_WRONLY))
		return -EINVAL;
	if (flags & BPF_F_RDONLY)
		return O_RDONLY;
	if (flags & BPF_F_WRONLY)
		return O_WRONLY;
	return O_RDWR;
}

/* helper macro to check that unused fields 'union bpf_attr' are zero */
#define CHECK_ATTR(CMD) \
	memchr_inv((void *) &attr->CMD##_LAST_FIELD + \
		   sizeof(attr->CMD##_LAST_FIELD), 0, \
		   sizeof(*attr) - \
		   offsetof(union bpf_attr, CMD##_LAST_FIELD) - \
		   sizeof(attr->CMD##_LAST_FIELD)) != NULL