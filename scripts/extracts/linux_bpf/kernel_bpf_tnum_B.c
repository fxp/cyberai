// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/tnum.c
// Segment 2/2



struct tnum tnum_cast(struct tnum a, u8 size)
{
	a.value &= (1ULL << (size * 8)) - 1;
	a.mask &= (1ULL << (size * 8)) - 1;
	return a;
}

bool tnum_is_aligned(struct tnum a, u64 size)
{
	if (!size)
		return true;
	return !((a.value | a.mask) & (size - 1));
}

bool tnum_in(struct tnum a, struct tnum b)
{
	if (b.mask & ~a.mask)
		return false;
	b.value &= ~a.mask;
	return a.value == b.value;
}

int tnum_sbin(char *str, size_t size, struct tnum a)
{
	size_t n;

	for (n = 64; n; n--) {
		if (n < size) {
			if (a.mask & 1)
				str[n - 1] = 'x';
			else if (a.value & 1)
				str[n - 1] = '1';
			else
				str[n - 1] = '0';
		}
		a.mask >>= 1;
		a.value >>= 1;
	}
	str[min(size - 1, (size_t)64)] = 0;
	return 64;
}

struct tnum tnum_subreg(struct tnum a)
{
	return tnum_cast(a, 4);
}

struct tnum tnum_clear_subreg(struct tnum a)
{
	return tnum_lshift(tnum_rshift(a, 32), 32);
}

struct tnum tnum_with_subreg(struct tnum reg, struct tnum subreg)
{
	return tnum_or(tnum_clear_subreg(reg), tnum_subreg(subreg));
}

struct tnum tnum_const_subreg(struct tnum a, u32 value)
{
	return tnum_with_subreg(a, tnum_const(value));
}

struct tnum tnum_bswap16(struct tnum a)
{
	return TNUM(swab16(a.value & 0xFFFF), swab16(a.mask & 0xFFFF));
}

struct tnum tnum_bswap32(struct tnum a)
{
	return TNUM(swab32(a.value & 0xFFFFFFFF), swab32(a.mask & 0xFFFFFFFF));
}

struct tnum tnum_bswap64(struct tnum a)
{
	return TNUM(swab64(a.value), swab64(a.mask));
}

/* Given tnum t, and a number z such that tmin <= z < tmax, where tmin
 * is the smallest member of the t (= t.value) and tmax is the largest
 * member of t (= t.value | t.mask), returns the smallest member of t
 * larger than z.
 *
 * For example,
 * t      = x11100x0
 * z      = 11110001 (241)
 * result = 11110010 (242)
 *
 * Note: if this function is called with z >= tmax, it just returns
 * early with tmax; if this function is called with z < tmin, the
 * algorithm already returns tmin.
 */
u64 tnum_step(struct tnum t, u64 z)
{
	u64 tmax, d, carry_mask, filled, inc;

	tmax = t.value | t.mask;

	/* if z >= largest member of t, return largest member of t */
	if (z >= tmax)
		return tmax;

	/* if z < smallest member of t, return smallest member of t */
	if (z < t.value)
		return t.value;

	/*
	 * Let r be the result tnum member, z = t.value + d.
	 * Every tnum member is t.value | s for some submask s of t.mask,
	 * and since t.value & t.mask == 0, t.value | s == t.value + s.
	 * So r > z becomes s > d where d = z - t.value.
	 *
	 * Find the smallest submask s of t.mask greater than d by
	 * "incrementing d within the mask": fill every non-mask
	 * position with 1 (`filled`) so +1 ripples through the gaps,
	 * then keep only mask bits. `carry_mask` additionally fills
	 * positions below the highest non-mask 1 in d, preventing
	 * it from trapping the carry.
	 */
	d = z - t.value;
	carry_mask = (1ULL << fls64(d & ~t.mask)) - 1;
	filled = d | carry_mask | ~t.mask;
	inc = (filled + 1) & t.mask;
	return t.value | inc;
}
