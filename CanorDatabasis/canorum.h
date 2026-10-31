#ifndef CANORDB_CANORUM_H
#define CANORDB_CANORUM_H
/* Canorum: protocol, waveform length, and interface slot. */
#include <stdint.h>
typedef struct {
  uint32_t protocol;
  uint32_t waveform_len;
  uint16_t interface_id;
} canordb_canorum;
int canordb_canorum_make(canordb_canorum *c, uint32_t protocol, uint32_t waveform_len,
                         uint16_t interface_id);
#endif
