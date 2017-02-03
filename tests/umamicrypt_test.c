/**
 * gcc umamicrypt_test.c -o umamicrypt_test ../aesencrypt.o ../base64encode.o ../umamicrypt.o -lcunit -lcurses -lcrypto -loauth -lcurl
 **/

#include "../umamicrypt.h"
#include <CUnit/Basic.h>


#define KEY "01234567890123456789012345678901"
#define IV "01234567890123456"

int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}


void testENCODE(void)
{
    char *input = "some string";
    char *output;
    int length;
    length = Encode(&output, input, (unsigned char *)KEY, (unsigned char *)IV);
    CU_ASSERT(output != input);
    CU_ASSERT(*(output+length) == '\00');
}

int main()
{
  CU_pSuite pSuite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  pSuite = CU_add_suite("Suite_1", init_suite1, clean_suite1);
  if (NULL == pSuite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if ((NULL == CU_add_test(pSuite, "test of Encode()", testENCODE)))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
