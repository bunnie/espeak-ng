/* Compatibility shim for <endian.h>
 *
 * "License": Public Domain
 * "Original": https://gist.github.com/panzi/6856583
 *
 * I, Mathias Panzenböck, place this file hereby into the public domain. Use it
 * at your own risk for whatever you like. In case there are jurisdictions that
 * don't support putting things in the public domain you can also consider it
 * to be "dual licensed" under the BSD, MIT and Apache licenses, if you want
 * to. This code is trivial anyway. Consider it an example on how to get the
 * endian conversion functions on different platforms.
 *
 * Modifications also in the Public Domain and dual licensed under BSD, MIT and
 * Apache licenses (using the terms outlined above):
 *
 * Copyright (C) 2016 Reece H. Dunn
 */

#ifndef ENDIAN_H_COMPAT_SHIM
#define ENDIAN_H_COMPAT_SHIM

#if defined(HAVE_ENDIAN_H)
#	pragma GCC system_header // Silence "warning: #include_next is a GCC extension"
#	include_next <endian.h>
#elif defined(HAVE_SYS_ENDIAN_H)
#	include <sys/endian.h>
#	if !defined(be16toh)
#		define be16toh(x) betoh16(x)
#	endif
#	if !defined(le16toh)
#		define le16toh(x) letoh16(x)
#	endif
#	if !defined(be32toh)
#		define be32toh(x) betoh32(x)
#	endif
#	if !defined(le32toh)
#		define le32toh(x) letoh32(x)
#	endif
#	if !defined(be64toh)
#		define be64toh(x) betoh64(x)
#	endif
#	if !defined(le64toh)
#		define le64toh(x) letoh64(x)
#	endif
#elif defined(__APPLE__)
#	include <libkern/OSByteOrder.h>

#	define htobe16(x) OSSwapHostToBigInt16(x)
#	define htole16(x) OSSwapHostToLittleInt16(x)
#	define be16toh(x) OSSwapBigToHostInt16(x)
#	define le16toh(x) OSSwapLittleToHostInt16(x)
 
#	define htobe32(x) OSSwapHostToBigInt32(x)
#	define htole32(x) OSSwapHostToLittleInt32(x)
#	define be32toh(x) OSSwapBigToHostInt32(x)
#	define le32toh(x) OSSwapLittleToHostInt32(x)
 
#	define htobe64(x) OSSwapHostToBigInt64(x)
#	define htole64(x) OSSwapHostToLittleInt64(x)
#	define be64toh(x) OSSwapBigToHostInt64(x)
#	define le64toh(x) OSSwapLittleToHostInt64(x)

#	define __BYTE_ORDER    BYTE_ORDER
#	define __BIG_ENDIAN    BIG_ENDIAN
#	define __LITTLE_ENDIAN LITTLE_ENDIAN
#	define __PDP_ENDIAN    PDP_ENDIAN
#elif defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
#	if BYTE_ORDER == LITTLE_ENDIAN
#		include <winsock2.h>

#		define htobe16(x) htons(x)
#		define htole16(x) (x)
#		define be16toh(x) ntohs(x)
#		define le16toh(x) (x)
 
#		define htobe32(x) htonl(x)
#		define htole32(x) (x)
#		define be32toh(x) ntohl(x)
#		define le32toh(x) (x)
 
#		define htobe64(x) htonll(x)
#		define htole64(x) (x)
#		define be64toh(x) ntohll(x)
#		define le64toh(x) (x)
#	elif BYTE_ORDER == BIG_ENDIAN /* that would be xbox 360 */
#		define htobe16(x) (x)
#		define htole16(x) __builtin_bswap16(x)
#		define be16toh(x) (x)
#		define le16toh(x) __builtin_bswap16(x)
 
#		define htobe32(x) (x)
#		define htole32(x) __builtin_bswap32(x)
#		define be32toh(x) (x)
#		define le32toh(x) __builtin_bswap32(x)
 
#		define htobe64(x) (x)
#		define htole64(x) __builtin_bswap64(x)
#		define be64toh(x) (x)
#		define le64toh(x) __builtin_bswap64(x)
#	else
#		error byte order not supported
#	endif

#	define __BYTE_ORDER    BYTE_ORDER
#	define __BIG_ENDIAN    BIG_ENDIAN
#	define __LITTLE_ENDIAN LITTLE_ENDIAN
#	define __PDP_ENDIAN    PDP_ENDIAN
#elif defined(EMBEDDED)

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define _QUAD_HIGHWORD  1
#define _QUAD_LOWWORD   0
#else
#define _QUAD_HIGHWORD  0
#define _QUAD_LOWWORD   1
#endif

#if __BSD_VISIBLE
#define LITTLE_ENDIAN   _LITTLE_ENDIAN
#define BIG_ENDIAN      _BIG_ENDIAN
#define PDP_ENDIAN      _PDP_ENDIAN
#define BYTE_ORDER      _BYTE_ORDER
#endif

#ifdef __GNUC__
#define __bswap16(_x)   __builtin_bswap16(_x)
#define __bswap32(_x)   __builtin_bswap32(_x)
#define __bswap64(_x)   __builtin_bswap64(_x)
#else /* __GNUC__ */
static __inline __uint16_t
__bswap16(__uint16_t _x)
{

        return ((__uint16_t)((_x >> 8) | ((_x << 8) & 0xff00)));
}

static __inline __uint32_t
__bswap32(__uint32_t _x)
{

        return ((__uint32_t)((_x >> 24) | ((_x >> 8) & 0xff00) |
            ((_x << 8) & 0xff0000) | ((_x << 24) & 0xff000000)));
}

static __inline __uint64_t
__bswap64(__uint64_t _x)
{

        return ((__uint64_t)((_x >> 56) | ((_x >> 40) & 0xff00) |
            ((_x >> 24) & 0xff0000) | ((_x >> 8) & 0xff000000) |
            ((_x << 8) & ((__uint64_t)0xff << 32)) |
            ((_x << 24) & ((__uint64_t)0xff << 40)) |
            ((_x << 40) & ((__uint64_t)0xff << 48)) | ((_x << 56))));
}
#endif /* !__GNUC__ */

#ifndef __machine_host_to_from_network_defined
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define __htonl(_x)     __bswap32(_x)
#define __htons(_x)     __bswap16(_x)
#define __ntohl(_x)     __bswap32(_x)
#define __ntohs(_x)     __bswap16(_x)
#else
#define __htonl(_x)     ((__uint32_t)(_x))
#define __htons(_x)     ((__uint16_t)(_x))
#define __ntohl(_x)     ((__uint32_t)(_x))
#define __ntohs(_x)     ((__uint16_t)(_x))
#endif
#endif /* __machine_host_to_from_network_defined */

#else
#	error platform not supported
#endif
#endif
