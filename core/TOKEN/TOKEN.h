#ifndef CANOR_CORE_TOKEN_H
#define CANOR_CORE_TOKEN_H
/* TOKEN: one file per CANOR_V1-ALPHA.md core type.
 * The integer implementation is canor_token_eval_fn in canor_core.h.
 * This container is the module boundary the alpha spec requires. */
#include "../include/canor_core.h"
typedef canor_token_eval_fn TOKEN;
#endif

