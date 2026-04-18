// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 21/43



/**
 * cgroup_apply_control - apply control mask updates to the subtree
 * @cgrp: root of the target subtree
 *
 * subsystems can be enabled and disabled in a subtree using the following
 * steps.
 *
 * 1. Call cgroup_save_control() to stash the current state.
 * 2. Update ->subtree_control masks in the subtree as desired.
 * 3. Call cgroup_apply_control() to apply the changes.
 * 4. Optionally perform other related operations.
 * 5. Call cgroup_finalize_control() to finish up.
 *
 * This function implements step 3 and propagates the mask changes
 * throughout @cgrp's subtree, updates csses accordingly and perform
 * process migrations.
 */
static int cgroup_apply_control(struct cgroup *cgrp)
{
	int ret;

	cgroup_propagate_control(cgrp);

	ret = cgroup_apply_control_enable(cgrp);
	if (ret)
		return ret;

	/*
	 * At this point, cgroup_e_css_by_mask() results reflect the new csses
	 * making the following cgroup_update_dfl_csses() properly update
	 * css associations of all tasks in the subtree.
	 */
	return cgroup_update_dfl_csses(cgrp);
}

/**
 * cgroup_finalize_control - finalize control mask update
 * @cgrp: root of the target subtree
 * @ret: the result of the update
 *
 * Finalize control mask update.  See cgroup_apply_control() for more info.
 */
static void cgroup_finalize_control(struct cgroup *cgrp, int ret)
{
	if (ret) {
		cgroup_restore_control(cgrp);
		cgroup_propagate_control(cgrp);
	}

	cgroup_apply_control_disable(cgrp);
}

static int cgroup_vet_subtree_control_enable(struct cgroup *cgrp, u32 enable)
{
	u32 domain_enable = enable & ~cgrp_dfl_threaded_ss_mask;

	/* if nothing is getting enabled, nothing to worry about */
	if (!enable)
		return 0;

	/* can @cgrp host any resources? */
	if (!cgroup_is_valid_domain(cgrp->dom_cgrp))
		return -EOPNOTSUPP;

	/* mixables don't care */
	if (cgroup_is_mixable(cgrp))
		return 0;

	if (domain_enable) {
		/* can't enable domain controllers inside a thread subtree */
		if (cgroup_is_thread_root(cgrp) || cgroup_is_threaded(cgrp))
			return -EOPNOTSUPP;
	} else {
		/*
		 * Threaded controllers can handle internal competitions
		 * and are always allowed inside a (prospective) thread
		 * subtree.
		 */
		if (cgroup_can_be_thread_root(cgrp) || cgroup_is_threaded(cgrp))
			return 0;
	}

	/*
	 * Controllers can't be enabled for a cgroup with tasks to avoid
	 * child cgroups competing against tasks.
	 */
	if (cgroup_has_tasks(cgrp))
		return -EBUSY;

	return 0;
}

/* change the enabled child controllers for a cgroup in the default hierarchy */
static ssize_t cgroup_subtree_control_write(struct kernfs_open_file *of,
					    char *buf, size_t nbytes,
					    loff_t off)
{
	u32 enable = 0, disable = 0;
	struct cgroup *cgrp, *child;
	struct cgroup_subsys *ss;
	char *tok;
	int ssid, ret;

	/*
	 * Parse input - space separated list of subsystem names prefixed
	 * with either + or -.
	 */
	buf = strstrip(buf);
	while ((tok = strsep(&buf, " "))) {
		if (tok[0] == '\0')
			continue;
		do_each_subsys_mask(ss, ssid, ~cgrp_dfl_inhibit_ss_mask) {
			if (!cgroup_ssid_enabled(ssid) ||
			    strcmp(tok + 1, ss->name))
				continue;

			if (*tok == '+') {
				enable |= 1 << ssid;
				disable &= ~(1 << ssid);
			} else if (*tok == '-') {
				disable |= 1 << ssid;
				enable &= ~(1 << ssid);
			} else {
				return -EINVAL;
			}
			break;
		} while_each_subsys_mask();
		if (ssid == CGROUP_SUBSYS_COUNT)
			return -EINVAL;
	}

	cgrp = cgroup_kn_lock_live(of->kn, true);
	if (!cgrp)
		return -ENODEV;

	for_each_subsys(ss, ssid) {
		if (enable & (1 << ssid)) {
			if (cgrp->subtree_control & (1 << ssid)) {
				enable &= ~(1 << ssid);
				continue;
			}

			if (!(cgroup_control(cgrp) & (1 << ssid))) {
				ret = -ENOENT;
				goto out_unlock;
			}
		} else if (disable & (1 << ssid)) {
			if (!(cgrp->subtree_control & (1 << ssid))) {
				disable &= ~(1 << ssid);
				continue;
			}

			/* a child has it enabled? */
			cgroup_for_each_live_child(child, cgrp) {
				if (child->subtree_control & (1 << ssid)) {
					ret = -EBUSY;
					goto out_unlock;
				}
			}
		}
	}

	if (!enable && !disable) {
		ret = 0;
		goto out_unlock;
	}

	ret = cgroup_vet_subtree_control_enable(cgrp, enable);
	if (ret)
		goto out_unlock;

	/* save and update control masks and prepare csses */
	cgroup_save_control(cgrp);

	cgrp->subtree_control |= enable;
	cgrp->subtree_control &= ~disable;

	ret = cgroup_apply_control(cgrp);
	cgroup_finalize_control(cgrp, ret);
	if (ret)
		goto out_unlock;

	kernfs_activate(cgrp->kn);
out_unlock:
	cgroup_kn_unlock(of->kn);
	return ret ?: nbytes;
}