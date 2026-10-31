#ifndef CANOR_REPO_INDEX_H
#define CANOR_REPO_INDEX_H
/* Content-addressed slots. The key is 32 bytes. Canor.md CanorRepositorium. */
#include <stddef.h>
#include <stdint.h>
#define CANOR_REPO_KEY 32
#define CANOR_REPO_CAP 32
typedef struct {
  uint8_t key[CANOR_REPO_KEY];
  uint64_t length;
  int used;
} canor_repo_slot;
typedef struct {
  canor_repo_slot slot[CANOR_REPO_CAP];
} canor_repo_index;
void canor_repo_init(canor_repo_index *idx);
int canor_repo_put(canor_repo_index *idx, const uint8_t key[CANOR_REPO_KEY], uint64_t length);
int canor_repo_find(const canor_repo_index *idx, const uint8_t key[CANOR_REPO_KEY]);
#endif
