#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdint.h>
int Base64Encode(const unsigned char* buffer, size_t length, char** b64text);
