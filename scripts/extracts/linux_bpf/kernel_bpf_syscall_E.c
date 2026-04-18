// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 37/37



static int __init bpf_syscall_sysctl_init(void)
{
	register_sysctl_init("kernel", bpf_syscall_table);
	return 0;
}
late_initcall(bpf_syscall_sysctl_init);
#endif /* CONFIG_SYSCTL */
