#ifndef CANOR_LEDGER_H
#define CANOR_LEDGER_H
/* Records a finished permutohedron address so another agent does not recompute it.
 * Address is a Lehmer code of at most 96 digits. Canor.md CanorInstinctivus. */
#include <stddef.h>
#include <stdint.h>
#define CANOR_LEDGER_LEVELS 96
#define CANOR_LEDGER_CAP 64
typedef struct {
  uint8_t digit[CANOR_LEDGER_LEVELS];
  uint8_t levels;
  uint8_t done;
} canor_ledger_task;
typedef struct {
  canor_ledger_task task[CANOR_LEDGER_CAP];
  int count;
} canor_ledger;
void canor_ledger_init(canor_ledger *L);
/* 0 stored, 1 already present, -1 rejected. */
int canor_ledger_claim(canor_ledger *L, const uint8_t *digit, uint8_t levels);
#endif
