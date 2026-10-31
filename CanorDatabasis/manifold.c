#include "manifold.h"
#include "../CanorLingua/lex.h"
#include <string.h>

static int fact_u64(unsigned n, uint64_t *out) {
  uint64_t a = 1;
  unsigned i;
  for (i = 2; i <= n; i++) {
    if (a > UINT64_MAX / i) return -1;
    a *= i;
  }
  *out = a;
  return 0;
}

static int add1(uint64_t v, uint64_t *out) {
  if (v == UINT64_MAX) return -1;
  *out = v + 1;
  return 0;
}

int canordb_population(canordb_order order, int n, canordb_pop *out) {
  uint64_t f, inner, bang, value;
  if (!out || n < 1 || n > CANORDB_DIM_COUNT) return -1;
  if (order < CANORDB_GENERAL || order > CANORDB_ARBITRARY) return -1;
  out->exact = 0;
  out->value = 0;
  out->log2_milli = 0;
  if (order == CANORDB_GENERAL) {
    out->exact = 1;
    out->value = (uint64_t)(2 * n + 2);
    return 0;
  }
  if (order == CANORDB_SPECIFIC) {
    if (fact_u64((unsigned)n, &f) == 0 && f <= (UINT64_MAX - 2) / 2) {
      out->exact = 1;
      out->value = 2 * f + 2;
      return 0;
    }
    out->log2_milli = 1000 + canor_log2_factorial_milli((uint64_t)n);
    return 0;
  }
  if (order == CANORDB_DISTINCTIVE) {
    if (fact_u64((unsigned)(2 * n + 1), &bang) == 0 && add1(bang, &value) == 0) {
      out->exact = 1;
      out->value = value;
      return 0;
    }
    out->log2_milli = canor_log2_factorial_milli((uint64_t)(2 * n + 1));
    return 0;
  }
  if (fact_u64((unsigned)n, &f) == 0 && f <= (UINT64_MAX - 1) / 2) {
    inner = 2 * f + 1;
    if (inner <= 20 && fact_u64((unsigned)inner, &bang) == 0 && add1(bang, &value) == 0) {
      out->exact = 1;
      out->value = value;
      return 0;
    }
    if (inner <= 100000ull) {
      out->log2_milli = canor_log2_factorial_milli(inner);
      return 0;
    }
  }
  out->log2_milli = canor_kapprox_milli(CANOR_K_ARBITRARY, (uint64_t)n);
  return 0;
}

static void fill_lehmer(canor_lehmer *code, int levels, uint64_t seed) {
  int i;
  canor_lehmer_zero(code, levels);
  for (i = 1; i < levels; i++) {
    uint64_t mix = seed + (uint64_t)i * UINT64_C(0x9E3779B97F4A7C15);
    code->d[i] = (uint8_t)(mix % (uint64_t)(i + 1));
  }
}

int canordb_flow_address(int dim, canordb_order order, int n, canordb_flow *out) {
  int levels;
  uint64_t seed;
  if (!out || dim < 1 || dim > CANORDB_DIM_COUNT || n < 1 || n > CANORDB_DIM_COUNT) return -1;
  if (order < CANORDB_GENERAL || order > CANORDB_ARBITRARY) return -1;
  levels = n > CANOR_LEVELS_MAX ? CANOR_LEVELS_MAX : n;
  seed = ((uint64_t)dim << 16) ^ ((uint64_t)order << 8) ^ (uint64_t)n;
  out->dim = dim;
  out->order = order;
  out->n = n;
  fill_lehmer(&out->code, levels, seed);
  return canor_lehmer_valid(&out->code) ? 0 : -1;
}

int canordb_flow_cmp(const canordb_flow *a, const canordb_flow *b) {
  if (!a || !b) return 0;
  if (a->dim != b->dim) return a->dim < b->dim ? -1 : 1;
  if (a->order != b->order) return a->order < b->order ? -1 : 1;
  if (a->n != b->n) return a->n < b->n ? -1 : 1;
  return canor_lehmer_cmp(&a->code, &b->code);
}

static int rgba_of(uint8_t rank, uint8_t rgba[4]) {
  uint8_t pool[4] = {0, 1, 2, 3};
  int i;
  if (rank >= 24) return -1;
  for (i = 0; i < 4; i++) {
    int left = 4 - i;
    int pick = rank % left;
    rgba[i] = pool[pick];
    memmove(&pool[pick], &pool[pick + 1], (size_t)(left - 1 - pick));
    rank = (uint8_t)(rank / left);
  }
  return 0;
}

static int support_of(int stage, canordb_chart *out) {
  uint64_t s = 1;
  int i;
  for (i = 0; i < stage; i++) {
    if (s > UINT64_MAX / 4) {
      out->exact_support = 0;
      out->support = 0;
      out->support_log2_milli = (int64_t)stage * canor_log2_milli(4);
      return 0;
    }
    s *= 4;
  }
  out->exact_support = 1;
  out->support = s;
  out->support_log2_milli = 0;
  return 0;
}

int canordb_chart_stage(int stage, uint8_t rgba_rank, canordb_chart *out) {
  if (!out || stage < 2 || stage > 15) return -1;
  if (rgba_of(rgba_rank, out->rgba) != 0) return -1;
  out->stage = stage;
  out->dimension = 1u << stage;
  out->rgba_rank = rgba_rank;
  return support_of(stage, out);
}

int canordb_chart_dimension(uint32_t dimension, uint8_t rgba_rank, canordb_chart *out) {
  int stage = 0;
  uint32_t d = dimension;
  if (dimension < 4 || dimension > 32768) return -1;
  while (d > 1) {
    if (d & 1u) return -1;
    d >>= 1;
    stage++;
  }
  return canordb_chart_stage(stage, rgba_rank, out);
}

int canordb_chart_for_n(int n, uint8_t rgba_rank, canordb_chart *out) {
  int stage = 2;
  uint32_t dim = 4;
  if (n < 1) return -1;
  while (dim < (uint32_t)n && stage < 15) {
    stage++;
    dim <<= 1;
  }
  return canordb_chart_stage(stage, rgba_rank, out);
}

static uint8_t rank_of(const char *text) {
  uint32_t h = 0;
  const unsigned char *p = (const unsigned char *)text;
  if (!text) return 0;
  for (; *p; p++) h = h * 131u + *p;
  return (uint8_t)(h % 24u);
}

static int token_count(const char *text, int *matched_dim) {
  size_t i = 0, n = 0;
  int count = 0;
  int found = 0;
  if (!text) return 0;
  n = strlen(text);
  while (i < n) {
    size_t tok = 0;
    while (i < n && (text[i] == ' ' || text[i] == '\t' || text[i] == ',')) i++;
    if (i >= n) break;
    if (canor_lex_name(text + i, n - i, &tok) != 0) {
      i++;
      continue;
    }
    count++;
    if (!found) {
      char buf[64];
      size_t copy = tok < sizeof buf - 1 ? tok : sizeof buf - 1;
      memcpy(buf, text + i, copy);
      buf[copy] = 0;
      found = canordb_dim_lookup(buf);
    }
    i += tok;
  }
  *matched_dim = found;
  return count;
}

int canordb_infer(const char *text, canordb_inference *out) {
  int matched = 0;
  int count;
  uint64_t fold = 0;
  const unsigned char *p;
  if (!out) return -1;
  count = token_count(text ? text : "", &matched);
  if (count < 1) count = 1;
  if (count > CANORDB_DIM_COUNT) count = CANORDB_DIM_COUNT;
  if (!matched) {
    for (p = (const unsigned char *)(text ? text : ""); *p; p++)
      fold = fold * 131u + *p;
    matched = (int)(fold % 288u) + 1;
  }
  out->dim = matched;
  out->n = count;
  out->order = (canordb_order)(count % 4);
  {
    canordb_flow flow;
    if (canordb_flow_address(out->dim, out->order, out->n, &flow) != 0) return -1;
    out->code = flow.code;
  }
  return canordb_chart_for_n(out->n, rank_of(text ? text : ""), &out->chart);
}

static int unit_of(char c) {
  const char *letters = "ijklmnopqrstefg";
  const char *hit;
  if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
  hit = strchr(letters, c);
  if (!hit) return -1;
  return (int)(hit - letters);
}

int canordb_strategy_cmp(const char *spec, const canordb_inference *a, const canordb_inference *b) {
  int idx[3];
  int got = 0;
  const char *s;
  int k;
  if (!a || !b) return 0;
  if (!spec) return canor_lehmer_cmp(&a->code, &b->code);
  for (s = spec; *s && got < 3; s++) {
    if ((s[0] == 'd' || s[0] == 'D') && s[1] == '_' && s[2]) {
      int u = unit_of(s[2]);
      if (u >= 0) idx[got++] = u;
      s += 2;
    }
  }
  if (got < 3) return canor_lehmer_cmp(&a->code, &b->code);
  for (k = 0; k < 3; k++) {
    int ia = idx[k] < a->code.levels ? a->code.d[idx[k]] : 0;
    int ib = idx[k] < b->code.levels ? b->code.d[idx[k]] : 0;
    if (ia != ib) return ia < ib ? -1 : 1;
  }
  return 0;
}
