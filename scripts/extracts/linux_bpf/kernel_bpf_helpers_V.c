// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 22/30



/**
 * bpf_strcmp - Compare two strings
 * @s1__ign: One string
 * @s2__ign: Another string
 *
 * Return:
 * * %0       - Strings are equal
 * * %-1      - @s1__ign is smaller
 * * %1       - @s2__ign is smaller
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of strings is too large
 * * %-ERANGE - One of strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strcmp(const char *s1__ign, const char *s2__ign)
{
	return __bpf_strncasecmp(s1__ign, s2__ign, false, XATTR_SIZE_MAX);
}

/**
 * bpf_strcasecmp - Compare two strings, ignoring the case of the characters
 * @s1__ign: One string
 * @s2__ign: Another string
 *
 * Return:
 * * %0       - Strings are equal
 * * %-1      - @s1__ign is smaller
 * * %1       - @s2__ign is smaller
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of strings is too large
 * * %-ERANGE - One of strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strcasecmp(const char *s1__ign, const char *s2__ign)
{
	return __bpf_strncasecmp(s1__ign, s2__ign, true, XATTR_SIZE_MAX);
}

/*
 * bpf_strncasecmp - Compare two length-limited strings, ignoring case
 * @s1__ign: One string
 * @s2__ign: Another string
 * @len: The maximum number of characters to compare
 *
 * Return:
 * * %0       - Strings are equal
 * * %-1      - @s1__ign is smaller
 * * %1       - @s2__ign is smaller
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of strings is too large
 * * %-ERANGE - One of strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strncasecmp(const char *s1__ign, const char *s2__ign, size_t len)
{
	return __bpf_strncasecmp(s1__ign, s2__ign, true, len);
}

/**
 * bpf_strnchr - Find a character in a length limited string
 * @s__ign: The string to be searched
 * @count: The number of characters to be searched
 * @c: The character to search for
 *
 * Note that the %NUL-terminator is considered part of the string, and can
 * be searched for.
 *
 * Return:
 * * >=0      - Index of the first occurrence of @c within @s__ign
 * * %-ENOENT - @c not found in the first @count characters of @s__ign
 * * %-EFAULT - Cannot read @s__ign
 * * %-E2BIG  - @s__ign is too large
 * * %-ERANGE - @s__ign is outside of kernel address space
 */
__bpf_kfunc int bpf_strnchr(const char *s__ign, size_t count, char c)
{
	char sc;
	int i;

	if (!copy_from_kernel_nofault_allowed(s__ign, 1))
		return -ERANGE;

	guard(pagefault)();
	for (i = 0; i < count && i < XATTR_SIZE_MAX; i++) {
		__get_kernel_nofault(&sc, s__ign, char, err_out);
		if (sc == c)
			return i;
		if (sc == '\0')
			return -ENOENT;
		s__ign++;
	}
	return i == XATTR_SIZE_MAX ? -E2BIG : -ENOENT;
err_out:
	return -EFAULT;
}

/**
 * bpf_strchr - Find the first occurrence of a character in a string
 * @s__ign: The string to be searched
 * @c: The character to search for
 *
 * Note that the %NUL-terminator is considered part of the string, and can
 * be searched for.
 *
 * Return:
 * * >=0      - The index of the first occurrence of @c within @s__ign
 * * %-ENOENT - @c not found in @s__ign
 * * %-EFAULT - Cannot read @s__ign
 * * %-E2BIG  - @s__ign is too large
 * * %-ERANGE - @s__ign is outside of kernel address space
 */
__bpf_kfunc int bpf_strchr(const char *s__ign, char c)
{
	return bpf_strnchr(s__ign, XATTR_SIZE_MAX, c);
}

/**
 * bpf_strchrnul - Find and return a character in a string, or end of string
 * @s__ign: The string to be searched
 * @c: The character to search for
 *
 * Return:
 * * >=0      - Index of the first occurrence of @c within @s__ign or index of
 *              the null byte at the end of @s__ign when @c is not found
 * * %-EFAULT - Cannot read @s__ign
 * * %-E2BIG  - @s__ign is too large
 * * %-ERANGE - @s__ign is outside of kernel address space
 */
__bpf_kfunc int bpf_strchrnul(const char *s__ign, char c)
{
	char sc;
	int i;

	if (!copy_from_kernel_nofault_allowed(s__ign, 1))
		return -ERANGE;

	guard(pagefault)();
	for (i = 0; i < XATTR_SIZE_MAX; i++) {
		__get_kernel_nofault(&sc, s__ign, char, err_out);
		if (sc == '\0' || sc == c)
			return i;
		s__ign++;
	}
	return -E2BIG;
err_out:
	return -EFAULT;
}

/**
 * bpf_strrchr - Find the last occurrence of a character in a string
 * @s__ign: The string to be searched
 * @c: The character to search for
 *
 * Return:
 * * >=0      - Index of the last occurrence of @c within @s__ign
 * * %-ENOENT - @c not found in @s__ign
 * * %-EFAULT - Cannot read @s__ign
 * * %-E2BIG  - @s__ign is too large
 * * %-ERANGE - @s__ign is outside of kernel address space
 */
__bpf_kfunc int bpf_strrchr(const char *s__ign, int c)
{
	char sc;
	int i, last = -ENOENT;

	if (!copy_from_kernel_nofault_allowed(s__ign, 1))
		return -ERANGE;

	guard(pagefault)();
	for (i = 0; i < XATTR_SIZE_MAX; i++) {
		__get_kernel_nofault(&sc, s__ign, char, err_out);
		if (sc == c)
			last = i;
		if (sc == '\0')
			return last;
		s__ign++;
	}
	return -E2BIG;
err_out:
	return -EFAULT;
}