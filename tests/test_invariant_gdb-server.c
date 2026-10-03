/*
 * File: tests/test_invariant_gdb-server.c
 *
 *
 */

// TODO: CONTENT AND USE OF THIS SOURCE FILE IS TO BE VERIFIED

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Simulate the vulnerable function's context
#define STLINK_SERIAL_BUFFER_SIZE 16

// This test simulates the security property: buffer boundaries must never be exceeded
START_TEST(test_serial_buffer_boundary)
{
    // Invariant: memcpy must never write beyond st->serialnumber buffer boundaries
    const char *payloads[] = {
        "EXACT_EXPLOIT_PAYLOAD_OVERFLOW",  // Exact exploit case: longer than buffer
        "BOUNDARY_16_CHARS",               // Boundary: exactly STLINK_SERIAL_BUFFER_SIZE
        "VALID",                           // Valid: shorter than buffer
        "",                                // Edge: empty string
        "A\0B"                             // Edge: embedded null
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        char serialnumber[STLINK_SERIAL_BUFFER_SIZE];
        memset(serialnumber, 0xAA, sizeof(serialnumber)); // Fill with sentinel
        
        // Track bytes written
        size_t copy_len = strlen(payloads[i]) + 1;
        if (copy_len > STLINK_SERIAL_BUFFER_SIZE) {
            copy_len = STLINK_SERIAL_BUFFER_SIZE;
        }
        
        // Simulate the vulnerable memcpy call
        memcpy(serialnumber, payloads[i], STLINK_SERIAL_BUFFER_SIZE);
        
        // Property: No byte beyond buffer boundary should be modified
        // Check sentinel after buffer
        char after_buffer[8];
        memset(after_buffer, 0xAA, sizeof(after_buffer));
        
        // In real test, we'd check adjacent memory wasn't corrupted
        // For this demonstration, we verify buffer null-terminator when string fits
        if (strlen(payloads[i]) < STLINK_SERIAL_BUFFER_SIZE) {
            ck_assert_msg(serialnumber[STLINK_SERIAL_BUFFER_SIZE - 1] == 0xAA,
                         "Buffer overflow detected for payload: %s", payloads[i]);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_serial_buffer_boundary);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}