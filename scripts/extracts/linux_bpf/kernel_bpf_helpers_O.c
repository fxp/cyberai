// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 15/30



	return __bpf_list_add(n, head, false, meta ? meta->record : NULL, off);
}

__bpf_kfunc int bpf_list_push_front_impl(struct bpf_list_head *head,
					 struct bpf_list_node *node,
					 void *meta__ign, u64 off)
{
	return bpf_list_push_front(head, node, meta__ign, off);
}

/**
 * bpf_list_push_back() - add a node to the back of a BPF linked list
 * @head: list head
 * @node: node to insert
 * @meta: verifier-supplied struct metadata
 * @off: verifier-supplied offset of @node within the containing object
 *
 * Insert @node at the back of @head. The verifier rewrites @meta and @off;
 * BPF programs do not set them.
 *
 * Return: 0 on success, or %-EINVAL if @node is already linked.
 */
__bpf_kfunc int bpf_list_push_back(struct bpf_list_head *head,
				   struct bpf_list_node *node,
				   struct btf_struct_meta *meta,
				   u64 off)
{
	struct bpf_list_node_kern *n = (void *)node;

	return __bpf_list_add(n, head, true, meta ? meta->record : NULL, off);
}

__bpf_kfunc int bpf_list_push_back_impl(struct bpf_list_head *head,
					struct bpf_list_node *node,
					void *meta__ign, u64 off)
{
	return bpf_list_push_back(head, node, meta__ign, off);
}

static struct bpf_list_node *__bpf_list_del(struct bpf_list_head *head, bool tail)
{
	struct list_head *n, *h = (void *)head;
	struct bpf_list_node_kern *node;

	/* If list_head was 0-initialized by map, bpf_obj_init_field wasn't
	 * called on its fields, so init here
	 */
	if (unlikely(!h->next))
		INIT_LIST_HEAD(h);
	if (list_empty(h))
		return NULL;

	n = tail ? h->prev : h->next;
	node = container_of(n, struct bpf_list_node_kern, list_head);
	if (WARN_ON_ONCE(READ_ONCE(node->owner) != head))
		return NULL;

	list_del_init(n);
	WRITE_ONCE(node->owner, NULL);
	return (struct bpf_list_node *)n;
}

__bpf_kfunc struct bpf_list_node *bpf_list_pop_front(struct bpf_list_head *head)
{
	return __bpf_list_del(head, false);
}

__bpf_kfunc struct bpf_list_node *bpf_list_pop_back(struct bpf_list_head *head)
{
	return __bpf_list_del(head, true);
}

__bpf_kfunc struct bpf_list_node *bpf_list_front(struct bpf_list_head *head)
{
	struct list_head *h = (struct list_head *)head;

	if (list_empty(h) || unlikely(!h->next))
		return NULL;

	return (struct bpf_list_node *)h->next;
}

__bpf_kfunc struct bpf_list_node *bpf_list_back(struct bpf_list_head *head)
{
	struct list_head *h = (struct list_head *)head;

	if (list_empty(h) || unlikely(!h->next))
		return NULL;

	return (struct bpf_list_node *)h->prev;
}

__bpf_kfunc struct bpf_rb_node *bpf_rbtree_remove(struct bpf_rb_root *root,
						  struct bpf_rb_node *node)
{
	struct bpf_rb_node_kern *node_internal = (struct bpf_rb_node_kern *)node;
	struct rb_root_cached *r = (struct rb_root_cached *)root;
	struct rb_node *n = &node_internal->rb_node;

	/* node_internal->owner != root implies either RB_EMPTY_NODE(n) or
	 * n is owned by some other tree. No need to check RB_EMPTY_NODE(n)
	 */
	if (READ_ONCE(node_internal->owner) != root)
		return NULL;

	rb_erase_cached(n, r);
	RB_CLEAR_NODE(n);
	WRITE_ONCE(node_internal->owner, NULL);
	return (struct bpf_rb_node *)n;
}

/* Need to copy rbtree_add_cached's logic here because our 'less' is a BPF
 * program
 */
static int __bpf_rbtree_add(struct bpf_rb_root *root,
			    struct bpf_rb_node_kern *node,
			    void *less, struct btf_record *rec, u64 off)
{
	struct rb_node **link = &((struct rb_root_cached *)root)->rb_root.rb_node;
	struct rb_node *parent = NULL, *n = &node->rb_node;
	bpf_callback_t cb = (bpf_callback_t)less;
	bool leftmost = true;

	/* node->owner != NULL implies !RB_EMPTY_NODE(n), no need to separately
	 * check the latter
	 */
	if (cmpxchg(&node->owner, NULL, BPF_PTR_POISON)) {
		/* Only called from BPF prog, no need to migrate_disable */
		__bpf_obj_drop_impl((void *)n - off, rec, false);
		return -EINVAL;
	}

	while (*link) {
		parent = *link;
		if (cb((uintptr_t)node, (uintptr_t)parent, 0, 0, 0)) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = false;
		}
	}

	rb_link_node(n, parent, link);
	rb_insert_color_cached(n, (struct rb_root_cached *)root, leftmost);
	WRITE_ONCE(node->owner, root);
	return 0;
}

/**
 * bpf_rbtree_add() - add a node to a BPF rbtree
 * @root: tree root
 * @node: node to insert
 * @less: comparator used to order nodes
 * @meta: verifier-supplied struct metadata
 * @off: verifier-supplied offset of @node within the containing object
 *
 * Insert @node into @root using @less. The verifier rewrites @meta and @off;
 * BPF programs do not set them.
 *
 * Return: 0 on success, or %-EINVAL if @node is already linked in a tree.
 */
__bpf_kfunc int bpf_rbtree_add(struct bpf_rb_root *root,
			       struct bpf_rb_node *node,
			       bool (less)(struct bpf_rb_node *a, const struct bpf_rb_node *b),
			       struct btf_struct_meta *meta,
			       u64 off)
{
	struct bpf_rb_node_kern *n = (void *)node;

	return __bpf_rbtree_add(root, n, (void *)less, meta ? meta->record : NULL, off);
}