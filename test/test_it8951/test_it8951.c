#include <stdio.h>
#include "unity.h"
#include "unity_fixture.h"
#include "it8951.h"
#include "test_it8951.h"

void setUp(void) {
    printf("testing");
}

void tearDown(void) {

}

void test_send_command() {
    TEST_ASSERT_EQUAL(0, 0);
}

int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_send_command);
    return UNITY_END();
}

// This is required for the ESP-IDF framework
void app_main() {
    runUnityTests();
}
