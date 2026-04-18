// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 127/132



		if (fname)
			ret = btf_find_by_name_kind(btf, fname, BTF_KIND_FUNC);

		if (!fname || ret < 0) {
			bpf_log(log, "Cannot find btf of tracepoint template, fall back to %s%s.\n",
				prefix, tname);
			t = btf_type_by_id(btf, t->type);
			if (!btf_type_is_ptr(t))
				/* should never happen in valid vmlinux build */
				return -EINVAL;
		} else {
			t = btf_type_by_id(btf, ret);
			if (!btf_type_is_func(t))
				/* should never happen in valid vmlinux build */
				return -EINVAL;
		}

		t = btf_type_by_id(btf, t->type);
		if (!btf_type_is_func_proto(t))
			/* should never happen in valid vmlinux build */
			return -EINVAL;

		break;
	case BPF_TRACE_ITER:
		if (!btf_type_is_func(t)) {
			bpf_log(log, "attach_btf_id %u is not a function\n",
				btf_id);
			return -EINVAL;
		}
		t = btf_type_by_id(btf, t->type);
		if (!btf_type_is_func_proto(t))
			return -EINVAL;
		ret = btf_distill_func_proto(log, btf, t, tname, &tgt_info->fmodel);
		if (ret)
			return ret;
		break;
	default:
		if (!prog_extension)
			return -EINVAL;
		fallthrough;
	case BPF_MODIFY_RETURN:
	case BPF_LSM_MAC:
	case BPF_LSM_CGROUP:
	case BPF_TRACE_FENTRY:
	case BPF_TRACE_FEXIT:
	case BPF_TRACE_FSESSION:
		if (prog->expected_attach_type == BPF_TRACE_FSESSION &&
		    !bpf_jit_supports_fsession()) {
			bpf_log(log, "JIT does not support fsession\n");
			return -EOPNOTSUPP;
		}
		if (!btf_type_is_func(t)) {
			bpf_log(log, "attach_btf_id %u is not a function\n",
				btf_id);
			return -EINVAL;
		}
		if (prog_extension &&
		    btf_check_type_match(log, prog, btf, t))
			return -EINVAL;
		t = btf_type_by_id(btf, t->type);
		if (!btf_type_is_func_proto(t))
			return -EINVAL;

		if ((prog->aux->saved_dst_prog_type || prog->aux->saved_dst_attach_type) &&
		    (!tgt_prog || prog->aux->saved_dst_prog_type != tgt_prog->type ||
		     prog->aux->saved_dst_attach_type != tgt_prog->expected_attach_type))
			return -EINVAL;

		if (tgt_prog && conservative)
			t = NULL;

		ret = btf_distill_func_proto(log, btf, t, tname, &tgt_info->fmodel);
		if (ret < 0)
			return ret;

		if (tgt_prog) {
			if (subprog == 0)
				addr = (long) tgt_prog->bpf_func;
			else
				addr = (long) tgt_prog->aux->func[subprog]->bpf_func;
		} else {
			if (btf_is_module(btf)) {
				mod = btf_try_get_module(btf);
				if (mod)
					addr = find_kallsyms_symbol_value(mod, tname);
				else
					addr = 0;
			} else {
				addr = kallsyms_lookup_name(tname);
			}
			if (!addr) {
				module_put(mod);
				bpf_log(log,
					"The address of function %s cannot be found\n",
					tname);
				return -ENOENT;
			}
		}

		if (prog->sleepable) {
			ret = -EINVAL;
			switch (prog->type) {
			case BPF_PROG_TYPE_TRACING:
				if (!check_attach_sleepable(btf_id, addr, tname))
					ret = 0;
				/* fentry/fexit/fmod_ret progs can also be sleepable if they are
				 * in the fmodret id set with the KF_SLEEPABLE flag.
				 */
				else {
					u32 *flags = btf_kfunc_is_modify_return(btf, btf_id,
										prog);

					if (flags && (*flags & KF_SLEEPABLE))
						ret = 0;
				}
				break;
			case BPF_PROG_TYPE_LSM:
				/* LSM progs check that they are attached to bpf_lsm_*() funcs.
				 * Only some of them are sleepable.
				 */
				if (bpf_lsm_is_sleepable_hook(btf_id))
					ret = 0;
				break;
			default:
				break;
			}
			if (ret) {
				module_put(mod);
				bpf_log(log, "%s is not sleepable\n", tname);
				return ret;
			}
		} else if (prog->expected_attach_type == BPF_MODIFY_RETURN) {
			if (tgt_prog) {
				module_put(mod);
				bpf_log(log, "can't modify return codes of BPF programs\n");
				return -EINVAL;
			}
			ret = -EINVAL;
			if (btf_kfunc_is_modify_return(btf, btf_id, prog) ||
			    !check_attach_modify_return(addr, tname))
				ret = 0;
			if (ret) {
				module_put(mod);
				bpf_log(log, "%s() is not modifiable\n", tname);
				return ret;
			}
		}

		break;
	}
	tgt_info->tgt_addr = addr;
	tgt_info->tgt_name = tname;
	tgt_info->tgt_type = t;
	tgt_info->tgt_mod = mod;
	return 0;
}

BTF_SET_START(btf_id_deny)
BTF_ID_UNUSED
#ifdef CONFIG_SMP
BTF_ID(func, ___migrate_enable)
BTF_ID(func, migrate_disable)
BTF_ID(func, migrate_enable)
#endif
#if !defined CONFIG_PREEMPT_RCU && !defined CONFIG_TINY_RCU
BTF_ID(func, rcu_read_unlock_strict)
#endif
#if defined(CONFIG_DEBUG_PREEMPT) || defined(CONFIG_TRACE_PREEMPT_TOGGLE)
BTF_ID(func, preempt_count_add)
BTF_ID(func, preempt_count_sub)
#endif
#ifdef CONFIG_PREEMPT_RCU
BTF_ID(func, __rcu_read_lock)
BTF_ID(func, __rcu_read_unlock)
#endif
BTF_SET_END(btf_id_deny)