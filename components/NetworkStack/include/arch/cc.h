/* components/NetworkStack/include/arch/cc.h */
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* seL4 on x86_64 uses Little Endian */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/* Type formatters for printf() debugging */
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "zu"

/* * CRITICAL: Struct Packing
 * Network packets arrive as raw bytes. We must tell GCC to pack structs
 * tightly without adding padding, otherwise IP/TCP headers will be misaligned!
 */
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* Diagnostic Output (Route lwIP warnings to the seL4 console) */
#define LWIP_PLATFORM_DIAG(x)   do { printf x; } while(0)
#define LWIP_PLATFORM_ASSERT(x) do { printf("lwIP ASSERT: \"%s\" failed at line %d in %s\n", x, __LINE__, __FILE__); abort(); } while(0)

#endif /* LWIP_ARCH_CC_H */
