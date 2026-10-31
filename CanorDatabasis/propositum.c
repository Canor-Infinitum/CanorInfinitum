#include "propositum.h"
int canordb_propositum_link(canordb_propositum *n, int32_t id, int32_t parent,
                            canordb_propositum_kind kind) {
  if (!n || id < 0) return -1;
  if (kind < CANORDB_ELEM || kind > CANORDB_UNKNOWN) return -1;
  if (parent == id) return -1;
  n->id = id;
  n->parent = parent;
  n->kind = kind;
  return 0;
}
