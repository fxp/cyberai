// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 33/37



	if (old_map)
		bpf_map_put(old_map);
out_put:
	bpf_map_put(new_map);
	return ret;
}

#define BPF_LINK_UPDATE_LAST_FIELD link_update.old_prog_fd

static int link_update(union bpf_attr *attr)
{
	struct bpf_prog *old_prog = NULL, *new_prog;
	struct bpf_link *link;
	u32 flags;
	int ret;

	if (CHECK_ATTR(BPF_LINK_UPDATE))
		return -EINVAL;

	flags = attr->link_update.flags;
	if (flags & ~BPF_F_REPLACE)
		return -EINVAL;

	link = bpf_link_get_from_fd(attr->link_update.link_fd);
	if (IS_ERR(link))
		return PTR_ERR(link);

	if (link->ops->update_map) {
		ret = link_update_map(link, attr);
		goto out_put_link;
	}

	new_prog = bpf_prog_get(attr->link_update.new_prog_fd);
	if (IS_ERR(new_prog)) {
		ret = PTR_ERR(new_prog);
		goto out_put_link;
	}

	if (flags & BPF_F_REPLACE) {
		old_prog = bpf_prog_get(attr->link_update.old_prog_fd);
		if (IS_ERR(old_prog)) {
			ret = PTR_ERR(old_prog);
			old_prog = NULL;
			goto out_put_progs;
		}
	} else if (attr->link_update.old_prog_fd) {
		ret = -EINVAL;
		goto out_put_progs;
	}

	if (link->ops->update_prog)
		ret = link->ops->update_prog(link, new_prog, old_prog);
	else
		ret = -EINVAL;

out_put_progs:
	if (old_prog)
		bpf_prog_put(old_prog);
	if (ret)
		bpf_prog_put(new_prog);
out_put_link:
	bpf_link_put_direct(link);
	return ret;
}

#define BPF_LINK_DETACH_LAST_FIELD link_detach.link_fd

static int link_detach(union bpf_attr *attr)
{
	struct bpf_link *link;
	int ret;

	if (CHECK_ATTR(BPF_LINK_DETACH))
		return -EINVAL;

	link = bpf_link_get_from_fd(attr->link_detach.link_fd);
	if (IS_ERR(link))
		return PTR_ERR(link);

	if (link->ops->detach)
		ret = link->ops->detach(link);
	else
		ret = -EOPNOTSUPP;

	bpf_link_put_direct(link);
	return ret;
}

struct bpf_link *bpf_link_inc_not_zero(struct bpf_link *link)
{
	return atomic64_fetch_add_unless(&link->refcnt, 1, 0) ? link : ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL(bpf_link_inc_not_zero);

struct bpf_link *bpf_link_by_id(u32 id)
{
	struct bpf_link *link;

	if (!id)
		return ERR_PTR(-ENOENT);

	spin_lock_bh(&link_idr_lock);
	/* before link is "settled", ID is 0, pretend it doesn't exist yet */
	link = idr_find(&link_idr, id);
	if (link) {
		if (link->id)
			link = bpf_link_inc_not_zero(link);
		else
			link = ERR_PTR(-EAGAIN);
	} else {
		link = ERR_PTR(-ENOENT);
	}
	spin_unlock_bh(&link_idr_lock);
	return link;
}

struct bpf_link *bpf_link_get_curr_or_next(u32 *id)
{
	struct bpf_link *link;

	spin_lock_bh(&link_idr_lock);
again:
	link = idr_get_next(&link_idr, id);
	if (link) {
		link = bpf_link_inc_not_zero(link);
		if (IS_ERR(link)) {
			(*id)++;
			goto again;
		}
	}
	spin_unlock_bh(&link_idr_lock);

	return link;
}

#define BPF_LINK_GET_FD_BY_ID_LAST_FIELD link_id

static int bpf_link_get_fd_by_id(const union bpf_attr *attr)
{
	struct bpf_link *link;
	u32 id = attr->link_id;
	int fd;

	if (CHECK_ATTR(BPF_LINK_GET_FD_BY_ID))
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	link = bpf_link_by_id(id);
	if (IS_ERR(link))
		return PTR_ERR(link);

	fd = bpf_link_new_fd(link);
	if (fd < 0)
		bpf_link_put_direct(link);

	return fd;
}

DEFINE_MUTEX(bpf_stats_enabled_mutex);

static int bpf_stats_release(struct inode *inode, struct file *file)
{
	mutex_lock(&bpf_stats_enabled_mutex);
	static_key_slow_dec(&bpf_stats_enabled_key.key);
	mutex_unlock(&bpf_stats_enabled_mutex);
	return 0;
}

static const struct file_operations bpf_stats_fops = {
	.release = bpf_stats_release,
};

static int bpf_enable_runtime_stats(void)
{
	int fd;

	mutex_lock(&bpf_stats_enabled_mutex);

	/* Set a very high limit to avoid overflow */
	if (static_key_count(&bpf_stats_enabled_key.key) > INT_MAX / 2) {
		mutex_unlock(&bpf_stats_enabled_mutex);
		return -EBUSY;
	}

	fd = anon_inode_getfd("bpf-stats", &bpf_stats_fops, NULL, O_CLOEXEC);
	if (fd >= 0)
		static_key_slow_inc(&bpf_stats_enabled_key.key);

	mutex_unlock(&bpf_stats_enabled_mutex);
	return fd;
}

#define BPF_ENABLE_STATS_LAST_FIELD enable_stats.type

static int bpf_enable_stats(union bpf_attr *attr)
{

	if (CHECK_ATTR(BPF_ENABLE_STATS))
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	switch (attr->enable_stats.type) {
	case BPF_STATS_RUN_TIME:
		return bpf_enable_runtime_stats();
	default:
		break;
	}
	return -EINVAL;
}

#define BPF_ITER_CREATE_LAST_FIELD iter_create.flags

static int bpf_iter_create(union bpf_attr *attr)
{
	struct bpf_link *link;
	int err;

	if (CHECK_ATTR(BPF_ITER_CREATE))
		return -EINVAL;

	if (attr->iter_create.flags)
		return -EINVAL;

	link = bpf_link_get_from_fd(attr->iter_create.link_fd);
	if (IS_ERR(link))
		return PTR_ERR(link);

	err = bpf_iter_new_fd(link);
	bpf_link_put_direct(link);

	return err;
}

#define BPF_PROG_BIND_MAP_LAST_FIELD prog_bind_map.flags

static int bpf_prog_bind_map(union bpf_attr *attr)
{
	struct bpf_prog *prog;
	struct bpf_map *map;
	struct bpf_map **used_maps_old, **used_maps_new;
	int i, ret = 0;

	if (CHECK_ATTR(BPF_PROG_BIND_MAP))
		return -EINVAL;

	if (attr->prog_bind_map.flags)
		return -EINVAL;