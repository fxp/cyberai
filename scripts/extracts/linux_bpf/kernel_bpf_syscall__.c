// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 31/37



	if (fd_file(f)->f_op == &bpf_prog_fops)
		return bpf_prog_get_info_by_fd(fd_file(f), fd_file(f)->private_data, attr,
					      uattr);
	else if (fd_file(f)->f_op == &bpf_map_fops)
		return bpf_map_get_info_by_fd(fd_file(f), fd_file(f)->private_data, attr,
					     uattr);
	else if (fd_file(f)->f_op == &btf_fops)
		return bpf_btf_get_info_by_fd(fd_file(f), fd_file(f)->private_data, attr, uattr);
	else if (fd_file(f)->f_op == &bpf_link_fops || fd_file(f)->f_op == &bpf_link_fops_poll)
		return bpf_link_get_info_by_fd(fd_file(f), fd_file(f)->private_data,
					      attr, uattr);
	else if (fd_file(f)->f_op == &bpf_token_fops)
		return token_get_info_by_fd(fd_file(f), fd_file(f)->private_data,
					    attr, uattr);
	return -EINVAL;
}

#define BPF_BTF_LOAD_LAST_FIELD btf_token_fd

static int bpf_btf_load(const union bpf_attr *attr, bpfptr_t uattr, __u32 uattr_size)
{
	struct bpf_token *token = NULL;

	if (CHECK_ATTR(BPF_BTF_LOAD))
		return -EINVAL;

	if (attr->btf_flags & ~BPF_F_TOKEN_FD)
		return -EINVAL;

	if (attr->btf_flags & BPF_F_TOKEN_FD) {
		token = bpf_token_get_from_fd(attr->btf_token_fd);
		if (IS_ERR(token))
			return PTR_ERR(token);
		if (!bpf_token_allow_cmd(token, BPF_BTF_LOAD)) {
			bpf_token_put(token);
			token = NULL;
		}
	}

	if (!bpf_token_capable(token, CAP_BPF)) {
		bpf_token_put(token);
		return -EPERM;
	}

	bpf_token_put(token);

	return btf_new_fd(attr, uattr, uattr_size);
}

#define BPF_BTF_GET_FD_BY_ID_LAST_FIELD fd_by_id_token_fd

static int bpf_btf_get_fd_by_id(const union bpf_attr *attr)
{
	struct bpf_token *token = NULL;

	if (CHECK_ATTR(BPF_BTF_GET_FD_BY_ID))
		return -EINVAL;

	if (attr->open_flags & ~BPF_F_TOKEN_FD)
		return -EINVAL;

	if (attr->open_flags & BPF_F_TOKEN_FD) {
		token = bpf_token_get_from_fd(attr->fd_by_id_token_fd);
		if (IS_ERR(token))
			return PTR_ERR(token);
		if (!bpf_token_allow_cmd(token, BPF_BTF_GET_FD_BY_ID)) {
			bpf_token_put(token);
			token = NULL;
		}
	}

	if (!bpf_token_capable(token, CAP_SYS_ADMIN)) {
		bpf_token_put(token);
		return -EPERM;
	}

	bpf_token_put(token);

	return btf_get_fd_by_id(attr->btf_id);
}

static int bpf_task_fd_query_copy(const union bpf_attr *attr,
				    union bpf_attr __user *uattr,
				    u32 prog_id, u32 fd_type,
				    const char *buf, u64 probe_offset,
				    u64 probe_addr)
{
	char __user *ubuf = u64_to_user_ptr(attr->task_fd_query.buf);
	u32 len = buf ? strlen(buf) : 0, input_len;
	int err = 0;

	if (put_user(len, &uattr->task_fd_query.buf_len))
		return -EFAULT;
	input_len = attr->task_fd_query.buf_len;
	if (input_len && ubuf) {
		if (!len) {
			/* nothing to copy, just make ubuf NULL terminated */
			char zero = '\0';

			if (put_user(zero, ubuf))
				return -EFAULT;
		} else {
			err = bpf_copy_to_user(ubuf, buf, input_len, len);
			if (err == -EFAULT)
				return err;
		}
	}

	if (put_user(prog_id, &uattr->task_fd_query.prog_id) ||
	    put_user(fd_type, &uattr->task_fd_query.fd_type) ||
	    put_user(probe_offset, &uattr->task_fd_query.probe_offset) ||
	    put_user(probe_addr, &uattr->task_fd_query.probe_addr))
		return -EFAULT;

	return err;
}

#define BPF_TASK_FD_QUERY_LAST_FIELD task_fd_query.probe_addr

static int bpf_task_fd_query(const union bpf_attr *attr,
			     union bpf_attr __user *uattr)
{
	pid_t pid = attr->task_fd_query.pid;
	u32 fd = attr->task_fd_query.fd;
	const struct perf_event *event;
	struct task_struct *task;
	struct file *file;
	int err;

	if (CHECK_ATTR(BPF_TASK_FD_QUERY))
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (attr->task_fd_query.flags != 0)
		return -EINVAL;

	rcu_read_lock();
	task = get_pid_task(find_vpid(pid), PIDTYPE_PID);
	rcu_read_unlock();
	if (!task)
		return -ENOENT;

	err = 0;
	file = fget_task(task, fd);
	put_task_struct(task);
	if (!file)
		return -EBADF;

	if (file->f_op == &bpf_link_fops || file->f_op == &bpf_link_fops_poll) {
		struct bpf_link *link = file->private_data;

		if (link->ops == &bpf_raw_tp_link_lops) {
			struct bpf_raw_tp_link *raw_tp =
				container_of(link, struct bpf_raw_tp_link, link);
			struct bpf_raw_event_map *btp = raw_tp->btp;

			err = bpf_task_fd_query_copy(attr, uattr,
						     raw_tp->link.prog->aux->id,
						     BPF_FD_TYPE_RAW_TRACEPOINT,
						     btp->tp->name, 0, 0);
			goto put_file;
		}
		goto out_not_supp;
	}

	event = perf_get_event(file);
	if (!IS_ERR(event)) {
		u64 probe_offset, probe_addr;
		u32 prog_id, fd_type;
		const char *buf;

		err = bpf_get_perf_event_info(event, &prog_id, &fd_type,
					      &buf, &probe_offset,
					      &probe_addr, NULL);
		if (!err)
			err = bpf_task_fd_query_copy(attr, uattr, prog_id,
						     fd_type, buf,
						     probe_offset,
						     probe_addr);
		goto put_file;
	}

out_not_supp:
	err = -ENOTSUPP;
put_file:
	fput(file);
	return err;
}

#define BPF_MAP_BATCH_LAST_FIELD batch.flags

#define BPF_DO_BATCH(fn, ...)			\
	do {					\
		if (!fn) {			\
			err = -ENOTSUPP;	\
			goto err_put;		\
		}				\
		err = fn(__VA_ARGS__);		\
	} while (0)