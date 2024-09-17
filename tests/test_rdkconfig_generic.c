#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <CUnit/Basic.h>
#include "../src/rdkconfig_generic.h"

void test_rdkconfig_get(void)
{
    uint8_t *temp=NULL;
	size_t tempSize=0;
    int result = rdkconfig_get(&temp, &tempSize,NULL);
    CU_ASSERT_EQUAL(result, RDKCONFIG_FAIL);
    result = rdkconfig_get(&temp, &tempSize,"/tmp/.cfgStaticxpki");
    CU_ASSERT_EQUAL(result, RDKCONFIG_OK);     
}

void test_rdkconfig_set(void)
{
    int result = rdkconfig_set(NULL,NULL,0);
    CU_ASSERT_EQUAL(result, RDKCONFIG_OK);     
}

void test_rdkconfig_free(void)
{
    int result = rdkconfig_free(NULL,0);
    CU_ASSERT_EQUAL(result, RDKCONFIG_FAIL);
    char *ptr=NULL;
    ptr = strdup("hello");
    result = rdkconfig_free(&ptr,5);
    CU_ASSERT_EQUAL(result, RDKCONFIG_OK);      
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "test rdkconfig_get", test_rdkconfig_get);
    CU_add_test( *suite, "test rdkconfig_set", test_rdkconfig_set);
    CU_add_test( *suite, "test rdkconfig_free", test_rdkconfig_free);
}

int main( int argc, char *argv[] )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    (void ) argc;
    (void ) argv;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}