// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 29/30



BTF_KFUNCS_START(common_btf_ids)
BTF_ID_FLAGS(func, bpf_cast_to_kern_ctx, KF_FASTCALL)
BTF_ID_FLAGS(func, bpf_rdonly_cast, KF_FASTCALL)
BTF_ID_FLAGS(func, bpf_rcu_read_lock)
BTF_ID_FLAGS(func, bpf_rcu_read_unlock)
BTF_ID_FLAGS(func, bpf_dynptr_slice, KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_dynptr_slice_rdwr, KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_iter_num_new, KF_ITER_NEW)
BTF_ID_FLAGS(func, bpf_iter_num_next, KF_ITER_NEXT | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_iter_num_destroy, KF_ITER_DESTROY)
BTF_ID_FLAGS(func, bpf_iter_task_vma_new, KF_ITER_NEW | KF_RCU)
BTF_ID_FLAGS(func, bpf_iter_task_vma_next, KF_ITER_NEXT | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_iter_task_vma_destroy, KF_ITER_DESTROY)
#ifdef CONFIG_CGROUPS
BTF_ID_FLAGS(func, bpf_iter_css_task_new, KF_ITER_NEW)
BTF_ID_FLAGS(func, bpf_iter_css_task_next, KF_ITER_NEXT | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_iter_css_task_destroy, KF_ITER_DESTROY)
BTF_ID_FLAGS(func, bpf_iter_css_new, KF_ITER_NEW | KF_RCU_PROTECTED)
BTF_ID_FLAGS(func, bpf_iter_css_next, KF_ITER_NEXT | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_iter_css_destroy, KF_ITER_DESTROY)
#endif
BTF_ID_FLAGS(func, bpf_iter_task_new, KF_ITER_NEW | KF_RCU_PROTECTED)
BTF_ID_FLAGS(func, bpf_iter_task_next, KF_ITER_NEXT | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_iter_task_destroy, KF_ITER_DESTROY)
BTF_ID_FLAGS(func, bpf_dynptr_adjust)
BTF_ID_FLAGS(func, bpf_dynptr_is_null)
BTF_ID_FLAGS(func, bpf_dynptr_is_rdonly)
BTF_ID_FLAGS(func, bpf_dynptr_size)
BTF_ID_FLAGS(func, bpf_dynptr_clone)
BTF_ID_FLAGS(func, bpf_dynptr_copy)
BTF_ID_FLAGS(func, bpf_dynptr_memset)
#ifdef CONFIG_NET
BTF_ID_FLAGS(func, bpf_modify_return_test_tp)
#endif
BTF_ID_FLAGS(func, bpf_wq_init)
BTF_ID_FLAGS(func, bpf_wq_set_callback, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_wq_start)
BTF_ID_FLAGS(func, bpf_preempt_disable)
BTF_ID_FLAGS(func, bpf_preempt_enable)
BTF_ID_FLAGS(func, bpf_iter_bits_new, KF_ITER_NEW)
BTF_ID_FLAGS(func, bpf_iter_bits_next, KF_ITER_NEXT | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_iter_bits_destroy, KF_ITER_DESTROY)
BTF_ID_FLAGS(func, bpf_copy_from_user_str, KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_copy_from_user_task_str, KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_get_kmem_cache)
BTF_ID_FLAGS(func, bpf_iter_kmem_cache_new, KF_ITER_NEW | KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_iter_kmem_cache_next, KF_ITER_NEXT | KF_RET_NULL | KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_iter_kmem_cache_destroy, KF_ITER_DESTROY | KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_local_irq_save)
BTF_ID_FLAGS(func, bpf_local_irq_restore)
#ifdef CONFIG_BPF_EVENTS
BTF_ID_FLAGS(func, bpf_probe_read_user_dynptr)
BTF_ID_FLAGS(func, bpf_probe_read_kernel_dynptr)
BTF_ID_FLAGS(func, bpf_probe_read_user_str_dynptr)
BTF_ID_FLAGS(func, bpf_probe_read_kernel_str_dynptr)
BTF_ID_FLAGS(func, bpf_copy_from_user_dynptr, KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_copy_from_user_str_dynptr, KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_copy_from_user_task_dynptr, KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_copy_from_user_task_str_dynptr, KF_SLEEPABLE)
#endif
#ifdef CONFIG_DMA_SHARED_BUFFER
BTF_ID_FLAGS(func, bpf_iter_dmabuf_new, KF_ITER_NEW | KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_iter_dmabuf_next, KF_ITER_NEXT | KF_RET_NULL | KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_iter_dmabuf_destroy, KF_ITER_DESTROY | KF_SLEEPABLE)
#endif
BTF_ID_FLAGS(func, __bpf_trap)
BTF_ID_FLAGS(func, bpf_strcmp);
BTF_ID_FLAGS(func, bpf_strcasecmp);
BTF_ID_FLAGS(func, bpf_strncasecmp);
BTF_ID_FLAGS(func, bpf_strchr);
BTF_ID_FLAGS(func, bpf_strchrnul);
BTF_ID_FLAGS(func, bpf_strnchr);
BTF_ID_FLAGS(func, bpf_strrchr);
BTF_ID_FLAGS(func, bpf_strlen);
BTF_ID_FLAGS(func, bpf_strnlen);
BTF_ID_FLAGS(func, bpf_strspn);
BTF_ID_FLAGS(func, bpf_strcspn);
BTF_ID_FLAGS(func, bpf_strstr);
BTF_ID_FLAGS(func, bpf_strcasestr);
BTF_ID_FLAGS(func, bpf_strnstr);
BTF_ID_FLAGS(func, bpf_strncasestr);
#if defined(CONFIG_BPF_LSM) && defined(CONFIG_CGROUPS)
BTF_ID_FLAGS(func, bpf_cgroup_read_xattr, KF_RCU)
#endif
BTF_ID_FLAGS(func, bpf_stream_vprintk, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_stream_print_stack, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_task_work_schedule_signal, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_task_work_schedule_resume, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_dynptr_from_file)
BTF_ID_FLAGS(func, bpf_dynptr_file_discard)
BTF_ID_FLAGS(func, bpf_timer_cancel_async)
BTF_KFUNCS_END(common_btf_ids)

static const struct btf_kfunc_id_set common_kfunc_set = {
	.owner = THIS_MODULE,
	.set   = &common_btf_ids,
};

static int __init kfunc_init(void)
{
	int ret;
	const struct btf_id_dtor_kfunc generic_dtors[] = {
		{
			.btf_id       = generic_dtor_ids[0],
			.kfunc_btf_id = generic_dtor_ids[1]
		},
#ifdef CONFIG_CGROUPS
		{
			.btf_id       = generic_dtor_ids[2],
			.kfunc_btf_id = generic_dtor_ids[3]
		},
#endif
	};