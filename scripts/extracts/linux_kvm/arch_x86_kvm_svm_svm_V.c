// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 22/36



	pr_err("VMCB State Save Area:\n");
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "es:",
	       save->es.selector, save->es.attrib,
	       save->es.limit, save->es.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "cs:",
	       save->cs.selector, save->cs.attrib,
	       save->cs.limit, save->cs.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "ss:",
	       save->ss.selector, save->ss.attrib,
	       save->ss.limit, save->ss.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "ds:",
	       save->ds.selector, save->ds.attrib,
	       save->ds.limit, save->ds.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "fs:",
	       save01->fs.selector, save01->fs.attrib,
	       save01->fs.limit, save01->fs.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "gs:",
	       save01->gs.selector, save01->gs.attrib,
	       save01->gs.limit, save01->gs.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "gdtr:",
	       save->gdtr.selector, save->gdtr.attrib,
	       save->gdtr.limit, save->gdtr.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "ldtr:",
	       save01->ldtr.selector, save01->ldtr.attrib,
	       save01->ldtr.limit, save01->ldtr.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "idtr:",
	       save->idtr.selector, save->idtr.attrib,
	       save->idtr.limit, save->idtr.base);
	pr_err("%-5s s: %04x a: %04x l: %08x b: %016llx\n",
	       "tr:",
	       save01->tr.selector, save01->tr.attrib,
	       save01->tr.limit, save01->tr.base);
	pr_err("vmpl: %d   cpl:  %d               efer:          %016llx\n",
	       save->vmpl, save->cpl, save->efer);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "cr0:", save->cr0, "cr2:", save->cr2);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "cr3:", save->cr3, "cr4:", save->cr4);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "dr6:", save->dr6, "dr7:", save->dr7);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "rip:", save->rip, "rflags:", save->rflags);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "rsp:", save->rsp, "rax:", save->rax);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "s_cet:", save->s_cet, "ssp:", save->ssp);
	pr_err("%-15s %016llx\n",
	       "isst_addr:", save->isst_addr);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "star:", save01->star, "lstar:", save01->lstar);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "cstar:", save01->cstar, "sfmask:", save01->sfmask);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "kernel_gs_base:", save01->kernel_gs_base,
	       "sysenter_cs:", save01->sysenter_cs);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "sysenter_esp:", save01->sysenter_esp,
	       "sysenter_eip:", save01->sysenter_eip);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "gpat:", save->g_pat, "dbgctl:", save->dbgctl);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "br_from:", save->br_from, "br_to:", save->br_to);
	pr_err("%-15s %016llx %-13s %016llx\n",
	       "excp_from:", save->last_excp_from,
	       "excp_to:", save->last_excp_to);

	if (is_sev_es_guest(vcpu)) {
		struct sev_es_save_area *vmsa = (struct sev_es_save_area *)save;

		pr_err("%-15s %016llx\n",
		       "sev_features", vmsa->sev_features);

		pr_err("%-15s %016llx %-13s %016llx\n",
		       "pl0_ssp:", vmsa->pl0_ssp, "pl1_ssp:", vmsa->pl1_ssp);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "pl2_ssp:", vmsa->pl2_ssp, "pl3_ssp:", vmsa->pl3_ssp);
		pr_err("%-15s %016llx\n",
		       "u_cet:", vmsa->u_cet);