/*
 * addons.h
 *
 *  Created on: 1 ���� 2015
 *      Author: Daniel
 */

#ifndef HEAVYHITTERS_ADDONS_H_
#define HEAVYHITTERS_ADDONS_H_

#include <stdlib.h>

void *
memmem(const void *l, size_t l_len, const void *s, size_t s_len)
{
	register char *cur, *last;
	const char *cl = (const char *)l;
	const char *cs = (const char *)s;

	/* we need something to compare */
	if (l_len == 0 || s_len == 0)
		return NULL;

	/* "s" must be smaller or equal to "l" */
	if (l_len < s_len)
		return NULL;

	/* special case where s_len == 1 */
	if (s_len == 1)
		return memchr((void*) l, (int)*cs, l_len);

	/* the last position where its possible to find "s" in "l" */
	last = (char *)cl + l_len - s_len;

	for (cur = (char *)cl; cur <= last; cur++)
		if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
			return cur;

	return NULL;
}


//
//
///*
// * The memmem() function finds the start of the first occurrence of the
// * substring 'needle' of length 'nlen' in the memory area 'haystack' of
// * length 'hlen'.
// *
// * The return value is a pointer to the beginning of the sub-string, or
// * NULL if the substring is not found.
// */
//void *memmem(const void *haystack, size_t hlen, const void *needle, size_t nlen)
//{
//    int needle_first;
//    const void *p = haystack;
//    size_t plen = hlen;
//
//    if (!nlen)
//        return NULL;
//
//    needle_first = *(unsigned char *)needle;
//
//    while (plen >= nlen && (p = memchr(p, needle_first, plen - nlen + 1)))
//    {
//        if (!memcmp(p, needle, nlen))
//            return (void *)p;
//
//        p+=1;
//        plen = (size_t) hlen - (p - haystack);
//    }
//
//    return NULL;
//}


#endif /* HEAVYHITTERS_ADDONS_H_ */
