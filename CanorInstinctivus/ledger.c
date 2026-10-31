#include "ledger.h"
#include <string.h>
void canor_ledger_init(canor_ledger *L) {
  if (L) memset(L, 0, sizeof *L);
}
int canor_ledger_claim(canor_ledger *L, const uint8_t *digit, uint8_t levels) {
  int i, d;
  if (!L || !digit || levels < 1 || levels > CANOR_LEDGER_LEVELS) return -1;
  for (d = 0; d < levels; d++) if (digit[d] > d) return -1;
  for (i = 0; i < L->count; i++) {
    if (L->task[i].levels == levels &&
        memcmp(L->task[i].digit, digit, levels) == 0)
      return 1;
  }
  if (L->count >= CANOR_LEDGER_CAP) return -1;
  memset(L->task[L->count].digit, 0, CANOR_LEDGER_LEVELS);
  memcpy(L->task[L->count].digit, digit, levels);
  L->task[L->count].levels = levels;
  L->task[L->count].done = 1;
  L->count++;
  return 0;
}
