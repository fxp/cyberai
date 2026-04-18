// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 43/43



	ret = show_delegatable_files(cgroup_base_files, buf + ret,
				     PAGE_SIZE - ret, NULL);
	if (cgroup_psi_enabled())
		ret += show_delegatable_files(cgroup_psi_files, buf + ret,
					      PAGE_SIZE - ret, NULL);

	for_each_subsys(ss, ssid)
		ret += show_delegatable_files(ss->dfl_cftypes, buf + ret,
					      PAGE_SIZE - ret,
					      cgroup_subsys_name[ssid]);

	return ret;
}
static struct kobj_attribute cgroup_delegate_attr = __ATTR_RO(delegate);

static ssize_t features_show(struct kobject *kobj, struct kobj_attribute *attr,
			     char *buf)
{
	return snprintf(buf, PAGE_SIZE,
			"nsdelegate\n"
			"favordynmods\n"
			"memory_localevents\n"
			"memory_recursiveprot\n"
			"memory_hugetlb_accounting\n"
			"pids_localevents\n");
}
static struct kobj_attribute cgroup_features_attr = __ATTR_RO(features);

static struct attribute *cgroup_sysfs_attrs[] = {
	&cgroup_delegate_attr.attr,
	&cgroup_features_attr.attr,
	NULL,
};

static const struct attribute_group cgroup_sysfs_attr_group = {
	.attrs = cgroup_sysfs_attrs,
	.name = "cgroup",
};

static int __init cgroup_sysfs_init(void)
{
	return sysfs_create_group(kernel_kobj, &cgroup_sysfs_attr_group);
}
subsys_initcall(cgroup_sysfs_init);

#endif /* CONFIG_SYSFS */
