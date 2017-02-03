//Encodes Base64
#include "base64encode.h"

int Base64Encode(char **dest, unsigned char *src, unsigned int slen){
	if (src == NULL || slen <= 0) return 0;
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;
	b64 = BIO_new(BIO_f_base64());
        if (!b64) return 0;
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);
        if (!bio) return 0;
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	if (BIO_write(bio, src, slen) <= 0) {
      if (bio) BIO_free_all(bio);
      return 0;
  }

	if (1 != BIO_flush(bio)) {
      if (bio) BIO_free_all(bio);
      return 0;
  }

	BIO_get_mem_ptr(bio, &bufferPtr);

	BIO_set_close(bio, BIO_NOCLOSE);

	if (bio) BIO_free_all(bio);

	*dest = (char *)malloc((*bufferPtr).length + 1);

	if (*dest == NULL) return 0;

	strncpy(*dest, (*bufferPtr).data, (*bufferPtr).length + 1);

	return ((*bufferPtr).length + 1);
}
