// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 24/30



/**
 * bpf_strstr - Find the first substring in a string
 * @s1__ign: The string to be searched
 * @s2__ign: The string to search for
 *
 * Return:
 * * >=0      - Index of the first character of the first occurrence of @s2__ign
 *              within @s1__ign
 * * %-ENOENT - @s2__ign is not a substring of @s1__ign
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of the strings is too large
 * * %-ERANGE - One of the strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strstr(const char *s1__ign, const char *s2__ign)
{
	return __bpf_strnstr(s1__ign, s2__ign, XATTR_SIZE_MAX, false);
}

/**
 * bpf_strcasestr - Find the first substring in a string, ignoring the case of
 *                  the characters
 * @s1__ign: The string to be searched
 * @s2__ign: The string to search for
 *
 * Return:
 * * >=0      - Index of the first character of the first occurrence of @s2__ign
 *              within @s1__ign
 * * %-ENOENT - @s2__ign is not a substring of @s1__ign
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of the strings is too large
 * * %-ERANGE - One of the strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strcasestr(const char *s1__ign, const char *s2__ign)
{
	return __bpf_strnstr(s1__ign, s2__ign, XATTR_SIZE_MAX, true);
}

/**
 * bpf_strnstr - Find the first substring in a length-limited string
 * @s1__ign: The string to be searched
 * @s2__ign: The string to search for
 * @len: the maximum number of characters to search
 *
 * Return:
 * * >=0      - Index of the first character of the first occurrence of @s2__ign
 *              within the first @len characters of @s1__ign
 * * %-ENOENT - @s2__ign not found in the first @len characters of @s1__ign
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of the strings is too large
 * * %-ERANGE - One of the strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strnstr(const char *s1__ign, const char *s2__ign,
			    size_t len)
{
	return __bpf_strnstr(s1__ign, s2__ign, len, false);
}

/**
 * bpf_strncasestr - Find the first substring in a length-limited string,
 *                   ignoring the case of the characters
 * @s1__ign: The string to be searched
 * @s2__ign: The string to search for
 * @len: the maximum number of characters to search
 *
 * Return:
 * * >=0      - Index of the first character of the first occurrence of @s2__ign
 *              within the first @len characters of @s1__ign
 * * %-ENOENT - @s2__ign not found in the first @len characters of @s1__ign
 * * %-EFAULT - Cannot read one of the strings
 * * %-E2BIG  - One of the strings is too large
 * * %-ERANGE - One of the strings is outside of kernel address space
 */
__bpf_kfunc int bpf_strncasestr(const char *s1__ign, const char *s2__ign,
				size_t len)
{
	return __bpf_strnstr(s1__ign, s2__ign, len, true);
}

#ifdef CONFIG_KEYS
/**
 * bpf_lookup_user_key - lookup a key by its serial
 * @serial: key handle serial number
 * @flags: lookup-specific flags
 *
 * Search a key with a given *serial* and the provided *flags*.
 * If found, increment the reference count of the key by one, and
 * return it in the bpf_key structure.
 *
 * The bpf_key structure must be passed to bpf_key_put() when done
 * with it, so that the key reference count is decremented and the
 * bpf_key structure is freed.
 *
 * Permission checks are deferred to the time the key is used by
 * one of the available key-specific kfuncs.
 *
 * Set *flags* with KEY_LOOKUP_CREATE, to attempt creating a requested
 * special keyring (e.g. session keyring), if it doesn't yet exist.
 * Set *flags* with KEY_LOOKUP_PARTIAL, to lookup a key without waiting
 * for the key construction, and to retrieve uninstantiated keys (keys
 * without data attached to them).
 *
 * Return: a bpf_key pointer with a valid key pointer if the key is found, a
 *         NULL pointer otherwise.
 */
__bpf_kfunc struct bpf_key *bpf_lookup_user_key(s32 serial, u64 flags)
{
	key_ref_t key_ref;
	struct bpf_key *bkey;

	if (flags & ~KEY_LOOKUP_ALL)
		return NULL;

	/*
	 * Permission check is deferred until the key is used, as the
	 * intent of the caller is unknown here.
	 */
	key_ref = lookup_user_key(serial, flags, KEY_DEFER_PERM_CHECK);
	if (IS_ERR(key_ref))
		return NULL;

	bkey = kmalloc_obj(*bkey);
	if (!bkey) {
		key_put(key_ref_to_ptr(key_ref));
		return NULL;
	}

	bkey->key = key_ref_to_ptr(key_ref);
	bkey->has_ref = true;

	return bkey;
}