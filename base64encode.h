#ifndef BASE64ENCODE_H
#define BASE64ENCODE_H
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdint.h>
#include <string.h>
int Base64Encode(char **dest, unsigned char *src, unsigned int slen);

#endif /* BASE64ENCODE_H */
