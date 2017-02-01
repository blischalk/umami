//Decodes Base64

#include "base64decode.h"

unsigned int countDecodedLength(const char *encoded) {
	if (encoded == NULL) return 0;
	unsigned int len = strlen(encoded), padding = 0;

	if (encoded[len - 1] == '=' && encoded[len - 2] == '=')
		padding = 2;
	else if (encoded[len - 1] == '=')
		padding = 1;

	return (len * 3) / 4 - padding;
}

int Base64Decode(char **dest, char *src){
	if (src == NULL) return 0;
	unsigned int dlen = 0;
	BIO *bio, *b64;

	unsigned int decode_length = countDecodedLength(src);

	*dest = (char *)malloc(decode_length + 1);
	if (*dest == NULL) return 0;

	bio = BIO_new_mem_buf((char*)src, -1);
        if (!bio) return 0;

	b64 = BIO_new(BIO_f_base64());
        if (!b64 ) return 0;

	bio = BIO_push(b64, bio);
        if (!bio) return 0;

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	dlen = BIO_read(bio, *dest, strlen(src));

	if (dlen != decode_length){
		if (dest){
			free(*dest);
			dest = NULL;
		}
		if (bio) BIO_free_all(bio);
		return 0;
	}

        if (bio) BIO_free_all(bio);

        *(*dest + decode_length) = '\0';

	return 1;
}

