#include "index.h"
#include <string.h>
void canor_repo_init(canor_repo_index *idx) {
  if (idx) memset(idx, 0, sizeof *idx);
}
int canor_repo_find(const canor_repo_index *idx, const uint8_t key[CANOR_REPO_KEY]) {
  int i;
  if (!idx || !key) return -1;
  for (i = 0; i < CANOR_REPO_CAP; i++)
    if (idx->slot[i].used && memcmp(idx->slot[i].key, key, CANOR_REPO_KEY) == 0) return i;
  return -1;
}
int canor_repo_put(canor_repo_index *idx, const uint8_t key[CANOR_REPO_KEY], uint64_t length) {
  int i;
  if (!idx || !key || length == 0) return -1;
  if (canor_repo_find(idx, key) >= 0) return 1;
  for (i = 0; i < CANOR_REPO_CAP; i++) {
    if (!idx->slot[i].used) {
      memcpy(idx->slot[i].key, key, CANOR_REPO_KEY);
      idx->slot[i].length = length;
      idx->slot[i].used = 1;
      return 0;
    }
  }
  return -1;
}
