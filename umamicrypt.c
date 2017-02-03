#include "umamicrypt.h"

int Encode(char **dest, char *plaintext, unsigned char *key, unsigned char *iv)
{
  /* Bytes to be encrypted */
  //unsigned char *plaintext = (unsigned char *)input;

  /* Buffer for ciphertext. Ensure the buffer is long enough for the
   * ciphertext which may be longer than the plaintext, dependant on the
   * algorithm and mode
   */
  unsigned char ciphertext[10000];

  int ciphertext_len;

  /* Initialise the library */
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();
  OPENSSL_config(NULL);

  int textlen = strlen(plaintext);
  /* Encrypt the plaintext */
  ciphertext_len = encrypt((unsigned char *)plaintext, textlen, key, iv, ciphertext);
  printf("Ciphertext length is: %d", ciphertext_len);


  int counter;
	printf("Dumping Original String\n");
	printf("%s\n",plaintext);
	printf("\n");

	printf("Dumping Original Bytes\n");
	for (counter=0; counter < textlen; counter++)
	{
		printf("\\x%02x",plaintext[counter]);
	}

	printf("\n");
	printf("\n");
	printf("Dumping AES Encrypted Bytes\n");

	for (counter=0; counter < ciphertext_len; counter++)
	{
		printf("\\x%02x",ciphertext[counter]);
	}

	printf("\n");
	printf("\n");


  //char* base64EncodeOutput;
  int length;
  printf("Encoding: Ciphertext\n");
  length = Base64Encode(dest, ciphertext, ciphertext_len);
  printf("%s", *dest);
  printf("\n");

  /* Clean up */
  EVP_cleanup();
  ERR_free_strings();

  return length;
}
