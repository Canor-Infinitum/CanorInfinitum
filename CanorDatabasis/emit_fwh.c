/* Preset simplexes. Same lines as tool/manifold_fwh.py.
 *
 *   emit_fwh OUTDIR [flow_order] [meta_order] [ring_max] [--action]
 *
 * Defaults: Flow orders 1..12 on 5 trifectas, MetaFlow orders 1..9 on
 * R,G,B,A with ring indexes 1..96. --action rewrites the congruence file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TR[5] = {"ijk", "lmn", "opq", "rst", "efg"};

static void word(const char *alphabet, int base, int length, int index, char *out) {
  int p;
  for (p = length - 1; p >= 0; p--) {
    out[p] = alphabet[index % base];
    index /= base;
  }
  out[length] = 0;
}

static void sanitized(const char *text, int ring, char *out, size_t cap) {
  size_t n = 0;
  int i;
  out[0] = 0;
  for (i = 0; text[i]; i++) {
    int wrote;
    if (n && n + 2 < cap) {
      out[n++] = ',';
      out[n++] = ' ';
    }
    wrote = snprintf(out + n, cap - n, "%c_%d", text[i], ring);
    if (wrote < 0 || (size_t)wrote >= cap - n) return;
    n += (size_t)wrote;
  }
}

static void write_flows(FILE *fp, int order_max) {
  int t, n, i;
  char compact[16], san[128];
  for (t = 0; t < 5; t++) {
    for (n = 1; n <= order_max; n++) {
      int count = 1, k;
      for (k = 0; k < n; k++) count *= 3;
      for (i = 0; i < count; i++) {
        word(TR[t], 3, n, i, compact);
        sanitized(compact, t + 1, san, sizeof san);
        fprintf(fp, "Flow.%d.%d.%d : %s | %s\n", t + 1, n, i, compact, san);
      }
    }
  }
}

static void write_meta(FILE *fp, int order_max, int ring_max) {
  int n, i, ring;
  char compact[16], san[160];
  for (n = 1; n <= order_max; n++) {
    int count = 1, k;
    for (k = 0; k < n; k++) count *= 4;
    for (i = 0; i < count; i++) {
      word("RGBA", 4, n, i, compact);
      for (ring = 1; ring <= ring_max; ring++) {
        sanitized(compact, ring, san, sizeof san);
        fprintf(fp, "MetaFlow.%s.%d : %s | %s\n", compact, ring, compact, san);
      }
    }
  }
}

static void write_actions(FILE *fp) {
  int m, n, o;
  for (m = 1; m <= 288; m++)
    for (n = 1; n <= 288; n++)
      for (o = 1; o <= 288; o++)
        fprintf(fp, "Action.%d.%d.%d : %d = %d (mod %d)\n", m, n, o, m, n, o);
}

static FILE *open_out(const char *dir, const char *name) {
  char path[512];
  FILE *fp;
  snprintf(path, sizeof path, "%s/%s", dir, name);
  fp = fopen(path, "wb");
  if (fp) setvbuf(fp, 0, _IOFBF, 8 * 1024 * 1024);
  return fp;
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : "CanorDatabasis";
  int flow_order = argc > 2 ? atoi(argv[2]) : 12;
  int meta_order = argc > 3 ? atoi(argv[3]) : 9;
  int rings = argc > 4 ? atoi(argv[4]) : 96;
  int action = 0;
  int a;
  FILE *fp;
  for (a = 1; a < argc; a++)
    if (strcmp(argv[a], "--action") == 0) action = 1;
  if (flow_order < 1 || flow_order > 12 || meta_order < 1 || meta_order > 9 || rings < 1 || rings > 96)
    return 2;
  fp = open_out(dir, "flow.fwh");
  if (!fp) return 1;
  write_flows(fp, flow_order);
  fclose(fp);
  fp = open_out(dir, "metaflow.fwh");
  if (!fp) return 1;
  write_meta(fp, meta_order, rings);
  fclose(fp);
  if (action) {
    fp = open_out(dir, "action.fwh");
    if (!fp) return 1;
    write_actions(fp);
    fclose(fp);
  }
  return 0;
}
