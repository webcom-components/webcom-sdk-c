#ifndef SHA1_H
#define SHA1_H

/*
   SHA-1 in C
   By Steve Reid <steve@edmweb.com>
   100% Public Domain
 */

#include <stdint.h>

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} wc_SHA1_CTX;

void wc_SHA1Transform(
    uint32_t state[5],
    const unsigned char buffer[64]
    );

void wc_SHA1Init(
    wc_SHA1_CTX * context
    );

void wc_SHA1Update(
    wc_SHA1_CTX * context,
    const unsigned char *data,
    uint32_t len
    );

void wc_SHA1Final(
    unsigned char digest[20],
    wc_SHA1_CTX * context
    );

void wc_SHA1(
    char *hash_out,
    const char *str,
    int len);

#endif /* SHA1_H */
