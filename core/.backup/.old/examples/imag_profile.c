/* Reports whether the declared IMAG dimension closes under Cayley-Dickson.
 *
 * Build it twice to see the point:
 *   -DFC_IMAG_TRIFECTAS=4  ->  13 elements, as FOLLOW-CORE.md literally lists.
 *                              Does NOT close; init refuses.
 *   -DFC_IMAG_TRIFECTAS=5  ->  16 elements, sedenions. Closes.
 *
 * Exit status is 0 when the profile closes and 2 when it does not, so this is
 * usable as a build-time gate rather than only as a printout.
 */

#include "fc.h"

#include <stdio.h>

int main(void) {
  fc_status s = fc_imag_init();
  printf("FC_IMAG_TRIFECTAS=%d -> %d basis elements: %s\n",
         FC_IMAG_TRIFECTAS, FC_IMAG_COUNT, fc_strerror(s));
  if (s != FC_OK) {
    printf("  Hamilton/Cayley-Dickson closes only at 1, 2, 4, 8, 16.\n"
           "  %d is not among them, so no consistent multiplication table\n"
           "  exists over these units and BASE cannot be a permutation group\n"
           "  over them. Build with -DFC_IMAG_TRIFECTAS=5.\n", FC_IMAG_COUNT);
    return 2;
  }
  return 0;
}
