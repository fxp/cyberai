// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 20/43



/**
 * cgroup_save_control - save control masks and dom_cgrp of a subtree
 * @cgrp: root of the target subtree
 *
 * Save ->subtree_control, ->subtree_ss_mask and ->dom_cgrp to the
 * respective old_ prefixed fields for @cgrp's subtree including @cgrp
 * itself.
 */
static void cgroup_save_control(struct cgroup *cgrp)
{
	struct cgroup *dsct;
	struct cgroup_subsys_state *d_css;

	cgroup_for_each_live_descendant_pre(dsct, d_css, cgrp) {
		dsct->old_subtree_control = dsct->subtree_control;
		dsct->old_subtree_ss_mask = dsct->subtree_ss_mask;
		dsct->old_dom_cgrp = dsct->dom_cgrp;
	}
}

/**
 * cgroup_propagate_control - refresh control masks of a subtree
 * @cgrp: root of the target subtree
 *
 * For @cgrp and its subtree, ensure ->subtree_ss_mask matches
 * ->subtree_control and propagate controller availability through the
 * subtree so that descendants don't have unavailable controllers enabled.
 */
static void cgroup_propagate_control(struct cgroup *cgrp)
{
	struct cgroup *dsct;
	struct cgroup_subsys_state *d_css;

	cgroup_for_each_live_descendant_pre(dsct, d_css, cgrp) {
		dsct->subtree_control &= cgroup_control(dsct);
		dsct->subtree_ss_mask =
			cgroup_calc_subtree_ss_mask(dsct->subtree_control,
						    cgroup_ss_mask(dsct));
	}
}

/**
 * cgroup_restore_control - restore control masks and dom_cgrp of a subtree
 * @cgrp: root of the target subtree
 *
 * Restore ->subtree_control, ->subtree_ss_mask and ->dom_cgrp from the
 * respective old_ prefixed fields for @cgrp's subtree including @cgrp
 * itself.
 */
static void cgroup_restore_control(struct cgroup *cgrp)
{
	struct cgroup *dsct;
	struct cgroup_subsys_state *d_css;

	cgroup_for_each_live_descendant_post(dsct, d_css, cgrp) {
		dsct->subtree_control = dsct->old_subtree_control;
		dsct->subtree_ss_mask = dsct->old_subtree_ss_mask;
		dsct->dom_cgrp = dsct->old_dom_cgrp;
	}
}

static bool css_visible(struct cgroup_subsys_state *css)
{
	struct cgroup_subsys *ss = css->ss;
	struct cgroup *cgrp = css->cgroup;

	if (cgroup_control(cgrp) & (1 << ss->id))
		return true;
	if (!(cgroup_ss_mask(cgrp) & (1 << ss->id)))
		return false;
	return cgroup_on_dfl(cgrp) && ss->implicit_on_dfl;
}

/**
 * cgroup_apply_control_enable - enable or show csses according to control
 * @cgrp: root of the target subtree
 *
 * Walk @cgrp's subtree and create new csses or make the existing ones
 * visible.  A css is created invisible if it's being implicitly enabled
 * through dependency.  An invisible css is made visible when the userland
 * explicitly enables it.
 *
 * Returns 0 on success, -errno on failure.  On failure, csses which have
 * been processed already aren't cleaned up.  The caller is responsible for
 * cleaning up with cgroup_apply_control_disable().
 */
static int cgroup_apply_control_enable(struct cgroup *cgrp)
{
	struct cgroup *dsct;
	struct cgroup_subsys_state *d_css;
	struct cgroup_subsys *ss;
	int ssid, ret;

	cgroup_for_each_live_descendant_pre(dsct, d_css, cgrp) {
		for_each_subsys(ss, ssid) {
			struct cgroup_subsys_state *css = cgroup_css(dsct, ss);

			if (!(cgroup_ss_mask(dsct) & (1 << ss->id)))
				continue;

			if (!css) {
				css = css_create(dsct, ss);
				if (IS_ERR(css))
					return PTR_ERR(css);
			}

			WARN_ON_ONCE(percpu_ref_is_dying(&css->refcnt));

			if (css_visible(css)) {
				ret = css_populate_dir(css);
				if (ret)
					return ret;
			}
		}
	}

	return 0;
}

/**
 * cgroup_apply_control_disable - kill or hide csses according to control
 * @cgrp: root of the target subtree
 *
 * Walk @cgrp's subtree and kill and hide csses so that they match
 * cgroup_ss_mask() and cgroup_visible_mask().
 *
 * A css is hidden when the userland requests it to be disabled while other
 * subsystems are still depending on it.  The css must not actively control
 * resources and be in the vanilla state if it's made visible again later.
 * Controllers which may be depended upon should provide ->css_reset() for
 * this purpose.
 */
static void cgroup_apply_control_disable(struct cgroup *cgrp)
{
	struct cgroup *dsct;
	struct cgroup_subsys_state *d_css;
	struct cgroup_subsys *ss;
	int ssid;

	cgroup_for_each_live_descendant_post(dsct, d_css, cgrp) {
		for_each_subsys(ss, ssid) {
			struct cgroup_subsys_state *css = cgroup_css(dsct, ss);

			if (!css)
				continue;

			WARN_ON_ONCE(percpu_ref_is_dying(&css->refcnt));

			if (css->parent &&
			    !(cgroup_ss_mask(dsct) & (1 << ss->id))) {
				kill_css(css);
			} else if (!css_visible(css)) {
				css_clear_dir(css);
				if (ss->css_reset)
					ss->css_reset(css);
			}
		}
	}
}