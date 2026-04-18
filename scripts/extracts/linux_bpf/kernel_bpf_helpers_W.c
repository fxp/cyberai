// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 23/30



/**
 * bpf_strnlen - Calculate the length of a length-limited string
 * @s__ign: The string
 * @count: The maximum number of characters to count
 *
 * Return:
 * * >=0      - The length of @s__ign
 * * %-EFAULT - Cannot read @s__ign
 * * %-E2BIG  - @s__ign is too large
 * * %-ERANGE - @s__ign is outside of kernel address space
 */
__bpf_kfunc int bpf_strnlen(const char *s__ign, size_t count)
{
	char c;
	int i;

	if (!copy_from_kernel_nofault_allowed(s__ign, 1))
		return -ERANGE;

	guard(pagefault)();
	for (i = 0; i < count && i < XATTR_SIZE_MAX; i++) {
		__get_kernel_nofault(&c, s__ign, char, err_out);
		if (c == '\0')
			return i;
		s__ign++;
	}
	return i == XATTR_SIZE_MAX ? -E2BIG : i;
err_out:
	return -EFAULT;
}

/**
 * bpf_strlen - Calculate the length of a string
 * @s__ign: The string
 *
 * Return:
 * * >=0      - The length of @s__ign
 * * %-EFAULT - Cannot read @s__ign
 * * %-E2BIG  - @s__ign is too large
 * * %-ERANGE - @s__ign is outside of kernel address space
 */
__bpf_kfunc int bpf_strlen(const char *s__ign)
{
	return bpf_strnlen(s__ign, XATTR_SIZE_MAX);
}

/**
 * bpf_strspn - Calculate the length of the initial substring of @s__ign which
 *              only contains letters in @accept__ign
 * @s__ign: The string to be searched
 * @accept__ign: The string to search for
 *
 * Return:
 * * >=0      - The length of the initial substring of @s__ign which only
 *              contains letters from @accept__ign
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of the strings is too large
 * * %-ERANGE - One of the strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strspn(const char *s__ign, const char *accept__ign)
{
	char cs, ca;
	int i, j;

	if (!copy_from_kernel_nofault_allowed(s__ign, 1) ||
	    !copy_from_kernel_nofault_allowed(accept__ign, 1)) {
		return -ERANGE;
	}

	guard(pagefault)();
	for (i = 0; i < XATTR_SIZE_MAX; i++) {
		__get_kernel_nofault(&cs, s__ign, char, err_out);
		if (cs == '\0')
			return i;
		for (j = 0; j < XATTR_SIZE_MAX; j++) {
			__get_kernel_nofault(&ca, accept__ign + j, char, err_out);
			if (cs == ca || ca == '\0')
				break;
		}
		if (j == XATTR_SIZE_MAX)
			return -E2BIG;
		if (ca == '\0')
			return i;
		s__ign++;
	}
	return -E2BIG;
err_out:
	return -EFAULT;
}

/**
 * bpf_strcspn - Calculate the length of the initial substring of @s__ign which
 *               does not contain letters in @reject__ign
 * @s__ign: The string to be searched
 * @reject__ign: The string to search for
 *
 * Return:
 * * >=0      - The length of the initial substring of @s__ign which does not
 *              contain letters from @reject__ign
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of the strings is too large
 * * %-ERANGE - One of the strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strcspn(const char *s__ign, const char *reject__ign)
{
	char cs, cr;
	int i, j;

	if (!copy_from_kernel_nofault_allowed(s__ign, 1) ||
	    !copy_from_kernel_nofault_allowed(reject__ign, 1)) {
		return -ERANGE;
	}

	guard(pagefault)();
	for (i = 0; i < XATTR_SIZE_MAX; i++) {
		__get_kernel_nofault(&cs, s__ign, char, err_out);
		if (cs == '\0')
			return i;
		for (j = 0; j < XATTR_SIZE_MAX; j++) {
			__get_kernel_nofault(&cr, reject__ign + j, char, err_out);
			if (cs == cr || cr == '\0')
				break;
		}
		if (j == XATTR_SIZE_MAX)
			return -E2BIG;
		if (cr != '\0')
			return i;
		s__ign++;
	}
	return -E2BIG;
err_out:
	return -EFAULT;
}

static int __bpf_strnstr(const char *s1, const char *s2, size_t len,
			 bool ignore_case)
{
	char c1, c2;
	int i, j;

	if (!copy_from_kernel_nofault_allowed(s1, 1) ||
	    !copy_from_kernel_nofault_allowed(s2, 1)) {
		return -ERANGE;
	}

	guard(pagefault)();
	for (i = 0; i < XATTR_SIZE_MAX; i++) {
		for (j = 0; i + j <= len && j < XATTR_SIZE_MAX; j++) {
			__get_kernel_nofault(&c2, s2 + j, char, err_out);
			if (c2 == '\0')
				return i;
			/*
			 * We allow reading an extra byte from s2 (note the
			 * `i + j <= len` above) to cover the case when s2 is
			 * a suffix of the first len chars of s1.
			 */
			if (i + j == len)
				break;
			__get_kernel_nofault(&c1, s1 + j, char, err_out);

			if (ignore_case) {
				c1 = tolower(c1);
				c2 = tolower(c2);
			}

			if (c1 == '\0')
				return -ENOENT;
			if (c1 != c2)
				break;
		}
		if (j == XATTR_SIZE_MAX)
			return -E2BIG;
		if (i + j == len)
			return -ENOENT;
		s1++;
	}
	return -E2BIG;
err_out:
	return -EFAULT;
}