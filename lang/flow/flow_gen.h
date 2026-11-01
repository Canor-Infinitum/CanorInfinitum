#ifndef FLOW_GEN_H
#define FLOW_GEN_H

/* Freestanding word session shared by symb_gen.
   Zephyr (ARM Cortex-M, RISC-V): compile flow_gen.c with the symb_gen core.
   No malloc. No FILE. */

#include <stddef.h>
#include <stdint.h>

#define FLOW_MAX_ORDER 288
#define FLOW_LIMBS 40
#define FLOW_MAX_BASES 64
#define FLOW_NAME_MAX 64
#define FLOW_DEC_MAX 512
#define FLOW_LINE_MAX 4096
#define FLOW_HEADER_MAX 1024

typedef struct {
    uint32_t d[FLOW_LIMBS];
} FlowBig;

typedef struct {
    char bases[FLOW_MAX_BASES];
    int n_bases;
    int order;
    int rtl;
    char name[FLOW_NAME_MAX];
    FlowBig cursor;
    FlowBig end;
    FlowBig count;
    int digits[FLOW_MAX_ORDER];
    int active;
} FlowSession;

/* 0 on success, -1 on a rejected argument. Errors are written to err when err != 0. */
int flow_session_begin(FlowSession *s, const char *bases_text, int order,
                       const char *index_dec, const char *count_dec,
                       const char *direction, const char *name,
                       char *err, size_t err_cap);

/* .cah text, including the trailing newline of the last field. */
int flow_session_header(const FlowSession *s, const char *index_dec,
                        char *buf, size_t cap);

/* One @Name::Flow.Order.Index.Count:a b c; line, no newline.
   Returns 1 when a line was written, 0 when the slice is exhausted, -1 on overflow. */
int flow_session_next(FlowSession *s, char *buf, size_t cap);

/* Remainder of a limb integer. 0 on success, -1 if m is 0. */
int flow_big_mod_u32(const FlowBig *a, uint32_t m, uint32_t *out);

/* Decimal text of a limb integer. 0 on success. */
int flow_big_to_dec(const FlowBig *a, char *buf, size_t cap);

/* Decimal cardinality of the current alphabet and order. 0 on success. */
int flow_session_cardinality(const FlowSession *s, char *buf, size_t cap);

/* Drop the current word and move to the next. 1 if a word was current, 0 if the slice is done, -1 on overflow. */
int flow_session_advance(FlowSession *s);

#endif
