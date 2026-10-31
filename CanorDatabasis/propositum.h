#ifndef CANORDB_PROPOSITUM_H
#define CANORDB_PROPOSITUM_H
/* Aether Propositum: elementary, unique, composite, or unknown tree node. */
#include <stdint.h>
typedef enum {
  CANORDB_ELEM = 1,
  CANORDB_UNIQUE = 2,
  CANORDB_COMPOSITE = 3,
  CANORDB_UNKNOWN = 4
} canordb_propositum_kind;
typedef struct {
  canordb_propositum_kind kind;
  int32_t parent; /* -1 root */
  int32_t id;
} canordb_propositum;
int canordb_propositum_link(canordb_propositum *n, int32_t id, int32_t parent,
                            canordb_propositum_kind kind);
#endif
