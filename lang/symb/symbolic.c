#include "symb.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int fail(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  return 1;
}

static int cmd_range(const char *text) {
  canor_interval span;
  if (symb_range(text, &span) != CANOR_OK) return fail("range rejected");
  printf("%lld..%lld\n", (long long)span.lo.num, (long long)span.hi.num);
  return 0;
}

static int cmd_numeric(const char *op, const char *xs, const char *ys) {
  canor_real a, b, c;
  a.val = canor_rat(atoll(xs), 1);
  b.val = canor_rat(atoll(ys), 1);
  if (strcmp(op, "add") == 0) {
    if (symb_numeric_add(a, b, &c) != CANOR_OK) return fail("add");
  } else if (strcmp(op, "mul") == 0) {
    if (symb_numeric_mul(a, b, &c) != CANOR_OK) return fail("mul");
  } else return fail("numeric op");
  printf("%lld\n", (long long)c.val.num);
  return 0;
}

static int cmd_interval(const char *a, const char *b) {
  canor_interval ia, ib, out;
  if (symb_range(a, &ia) != CANOR_OK || symb_range(b, &ib) != CANOR_OK) return fail("interval");
  if (symb_interval_add(ia, ib, &out) != CANOR_OK) return fail("interval add");
  printf("%lld..%lld\n", (long long)out.lo.num, (long long)out.hi.num);
  return 0;
}

static int cmd_logical(const char *op, const char *xs, const char *ys) {
  bool bit;
  if (strcmp(op, "not") == 0) {
    if (symb_logical_not(atoi(xs) != 0, &bit) != CANOR_OK) return fail("not");
    printf("%d\n", bit ? 1 : 0);
    return 0;
  }
  if (strcmp(op, "and") == 0) symb_logical_and(atoi(xs) != 0, atoi(ys) != 0, &bit);
  else if (strcmp(op, "or") == 0) symb_logical_or(atoi(xs) != 0, atoi(ys) != 0, &bit);
  else if (strcmp(op, "cmp") == 0) {
    canor_real a, b; int cmp;
    a.val = canor_rat(atoll(xs), 1); b.val = canor_rat(atoll(ys), 1);
    if (symb_logical_cmp(a, b, &cmp) != CANOR_OK) return fail("cmp");
    printf("%d\n", cmp);
    return 0;
  } else return fail("logical op");
  printf("%d\n", bit ? 1 : 0);
  return 0;
}

static int cmd_func(const char *body, const char *xs) {
  canor_real x, y;
  x.val = canor_rat(atoll(xs), 1);
  if (symb_func_apply(body, x, &y) != CANOR_OK) return fail("func");
  printf("%lld\n", (long long)y.val.num);
  return 0;
}

static int cmd_parametric(const char *name) {
  symb_geometry g;
  if (symb_parametric(name, &g) != CANOR_OK) return fail("parametric");
  printf("%d\n", (int)g);
  return 0;
}

static int cmd_tensorial(const char *symmetry, const char *group) {
  canor_tensorial t;
  if (symb_tensorial_make(symmetry, (size_t)atoll(group), &t) != CANOR_OK) return fail("tensorial");
  printf("%s %llu\n", symmetry, (unsigned long long)t.group_id);
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) return fail("symbolic <range|numeric|interval|logical|func|parametric|tensorial|tensor>");
  if (strcmp(argv[1], "--self-test") == 0 || strcmp(argv[1], "tensor") == 0) {
    int a = symb_selftest();
    int b = symb_tensor_selftest();
    if (a || b) {
      fprintf(stderr, "self-test %d %d\n", a, b);
      return 1;
    }
    printf("symbolic self-test ok\n");
    return 0;
  }
  if (strcmp(argv[1], "range") == 0 && argc == 3) return cmd_range(argv[2]);
  if (strcmp(argv[1], "numeric") == 0 && argc == 5) return cmd_numeric(argv[2], argv[3], argv[4]);
  if (strcmp(argv[1], "interval") == 0 && argc == 4) return cmd_interval(argv[2], argv[3]);
  if (strcmp(argv[1], "logical") == 0 && argc >= 4) return cmd_logical(argv[2], argv[3], argc > 4 ? argv[4] : "0");
  if (strcmp(argv[1], "func") == 0 && argc == 4) return cmd_func(argv[2], argv[3]);
  if (strcmp(argv[1], "parametric") == 0 && argc == 3) return cmd_parametric(argv[2]);
  if (strcmp(argv[1], "tensorial") == 0 && argc == 4) return cmd_tensorial(argv[2], argv[3]);
  return fail("usage");
}
