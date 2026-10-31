#ifndef CANOR_TEST_H
#define CANOR_TEST_H

/* Minimal assertion harness shared by the property tests. A failed check is
 * reported with its location and counted; the test keeps going so one run
 * shows every broken property rather than the first. */

#include <stdint.h>
#include <stdio.h>

static int canor_test_failures = 0;
static int canor_test_checks = 0;

#define CANOR_CHECK(cond)                                                     \
  do {                                                                        \
    ++canor_test_checks;                                                      \
    if (!(cond)) {                                                            \
      ++canor_test_failures;                                                  \
      fprintf(stderr, "%s:%d: CHECK FAILED: %s\n", __FILE__, __LINE__, #cond); \
    }                                                                         \
  } while (0)

#define CANOR_TEST_DONE(name)                                                   \
  (printf("%s: %d checks, %d failed\n", (name), canor_test_checks,              \
          canor_test_failures),                                                 \
   canor_test_failures ? 1 : 0)

/* Deterministic 64-bit LCG (Knuth MMIX constants), so every host draws the
 * same test vectors. */
#define CANOR_TEST_LCG_MUL UINT64_C(6364136223846793005)
#define CANOR_TEST_LCG_ADD UINT64_C(1442695040888963407)
static inline uint64_t canor_test_next(uint64_t *state) {
  *state = *state * CANOR_TEST_LCG_MUL + CANOR_TEST_LCG_ADD;
  return *state;
}

#endif /* CANOR_TEST_H */
