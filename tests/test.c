#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

static FILE* temp_file = NULL;

int init_suite1(void)
{
  if (NULL == (temp_file = fopen("temp.txt", "w+"))) {
    return -1;
  }
  else {
    return 0;
  }
}

int clean_suite1(void)
{
   if (0 != fclose(temp_file)) {
     return -1;
   }
   else {
     temp_file = NULL;
     return 0;
   }
}

void testFPRINTF(void)
{
  int i1 = 10;

  if (NULL != temp_file) {
    CU_ASSERT(0 == fprintf(temp_file, ""));
    CU_ASSERT(2 == fprintf(temp_file, "Q\n"));
    CU_ASSERT(7 == fprintf(temp_file, "i1 = %d", i1));
  }
}

void testFREAD(void)
{
  unsigned char buffer[20];

  if (NULL != temp_file) {
    rewind(temp_file);
    CU_ASSERT(9 == fread(buffer, sizeof(unsigned char), 20, temp_file));
    CU_ASSERT(0 == strncmp(buffer, "Q\ni1 = 10", 9));
  }
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

  if ((NULL == CU_add_test(pSuite, "test of fprintf()", testFPRINTF)) ||
      (NULL == CU_add_test(pSuite, "test of fread()", testFREAD)))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
