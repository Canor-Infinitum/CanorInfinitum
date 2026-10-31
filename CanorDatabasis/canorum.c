#include "canorum.h"
int canordb_canorum_make(canordb_canorum *c, uint32_t protocol, uint32_t waveform_len,
                         uint16_t interface_id) {
  if (!c || protocol == 0 || waveform_len == 0) return -1;
  c->protocol = protocol;
  c->waveform_len = waveform_len;
  c->interface_id = interface_id;
  return 0;
}
