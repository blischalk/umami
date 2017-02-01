#ifndef BASE64DECODE_H
#define BASE64DECODE_H

#include <stdio.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <assert.h>
int Base64Decode(char **dest, char *src);

#endif /* BASE64DECODE_H */
