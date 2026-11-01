#define _CRT_SECURE_NO_WARNINGS
#include "flow_gen.h"
#include "ring_expr.h"
#include "place.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMB_LINE 65536

static int mod_k(void *ctx, uint32_t m, uint32_t *out) {
    return flow_big_mod_u32(&((FlowSession *)ctx)->cursor, m, out);
}

static int mod_c(void *ctx, uint32_t m, uint32_t *out) {
    return flow_big_mod_u32(&((FlowSession *)ctx)->count, m, out);
}

static int append(char *buf, size_t cap, size_t *n, const char *s) {
    while (*s) {
        if (*n + 1 >= cap) return 0;
        buf[(*n)++] = *s++;
    }
    buf[*n] = 0;
    return 1;
}

static int append_int(char *buf, size_t cap, size_t *n, int v) {
    char tmp[16], rev[16];
    int k = 0;
    if (v == 0) return append(buf, cap, n, "0");
    while (v) {
        rev[k++] = (char)('0' + v % 10);
        v /= 10;
    }
    for (int i = 0; i < k; i++) tmp[i] = rev[k - 1 - i];
    tmp[k] = 0;
    return append(buf, cap, n, tmp);
}

static int emit_line(FlowSession *s, const RingProg *prog, const char *kind, char *buf, size_t cap, char *err, size_t err_cap) {
    char index_s[FLOW_DEC_MAX], count_s[FLOW_DEC_MAX], atom[256];
    int64_t vals[8];
    int prev[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    size_t n = 0;
    RingEnv env;
    if (!s->active) return 0;
    if (flow_big_to_dec(&s->cursor, index_s, sizeof index_s) != 0) return -1;
    if (flow_big_to_dec(&s->count, count_s, sizeof count_s) != 0) return -1;
    memset(&env, 0, sizeof env);
    env.n = s->order;
    env.mod_k = mod_k;
    env.mod_c = mod_c;
    env.ctx = s;
    buf[0] = 0;
    if (!append(buf, cap, &n, "@") || !append(buf, cap, &n, s->name) || !append(buf, cap, &n, "::")) return -1;
    if (!append(buf, cap, &n, kind) || !append(buf, cap, &n, ".")) return -1;
    if (!append_int(buf, cap, &n, s->order)) return -1;
    if (!append(buf, cap, &n, ".") || !append(buf, cap, &n, index_s) || !append(buf, cap, &n, ".")) return -1;
    if (!append(buf, cap, &n, count_s) || !append(buf, cap, &n, ":{ ")) return -1;
    for (int i = 0; i < s->order; i++) {
        int src = s->rtl ? s->order - 1 - i : i;
        env.p = i;
        env.a = s->digits[src];
        for (int c = 0; c < 8; c++) env.prev[c] = prev[c];
        if (ring_eval8(prog, &env, vals, err, err_cap) != 0) return -1;
        if (ring_decorate(s->bases[s->digits[src]], vals, atom, sizeof atom) != 0) return -1;
        if (i && !append(buf, cap, &n, ", ")) return -1;
        if (!append(buf, cap, &n, atom)) return -1;
        for (int c = 0; c < 8; c++) {
            if (vals[c] > 2147483647ll) {
                snprintf(err, err_cap, "ring index does not fit");
                return -1;
            }
            prev[c] = (int)vals[c];
        }
    }
    if (!append(buf, cap, &n, " };")) return -1;
    return flow_session_advance(s) < 0 ? -1 : 1;
}

static int self_test(void) {
    FlowSession s;
    RingProg prog;
    char err[160], line[SYMB_LINE];
    if (ring_compile("0,0,0,p+1,p+1,0,0,0", &prog, err, sizeof err) != 0) return 0;
    if (flow_session_begin(&s, "i", 3, "0", "1", "LTR", "flow", err, sizeof err) != 0) return 0;
    if (emit_line(&s, &prog, "Flow", line, sizeof line, err, sizeof err) != 1) return 0;
    if (strcmp(line, "@flow::Flow.3.0.1:{ 1_i_1, 2_i_2, 3_i_3 };") != 0) return 0;
    if (ring_compile("0,0,a+1,0,0,a+1,0,0", &prog, err, sizeof err) != 0) return 0;
    if (flow_session_begin(&s, "R,G,B,A", 4, "0", "1", "LTR", "metaflow", err, sizeof err) != 0) return 0;
    if (emit_line(&s, &prog, "MetaFlow", line, sizeof line, err, sizeof err) != 1) return 0;
    if (strcmp(line, "@metaflow::MetaFlow.4.0.1:{ 1^R^1, 1^R^1, 1^R^1, 1^R^1 };") != 0) return 0;
    if (ring_compile("k+1,0,0,0,0,0,0,0", &prog, err, sizeof err) == 0) return 0;
    printf("symb_gen self-test ok\n");
    return 1;
}

static void usage(void) {
    fprintf(stderr,
            "usage: symb_gen --order N [--flow | --metaflow] [--bases LIST] [--index K]\n"
            "                [--count C|all] [--direction ltr|rtl] [--name NAME]\n"
            "                [--symbolic EXPR] [--dir DIR] [--stdout]\n"
            "  --flow is the default. --metaflow uses bases R,G,B,A and ::MetaFlow.\n"
            "  EXPR is eight channels: LTR prime,dot,caret,under, RTL under,caret,dot,prime\n");
}

int main(int argc, char **argv) {
    const char *bases = 0;
    const char *index_s = "0";
    const char *count_s = "1";
    const char *direction = "LTR";
    const char *name = 0;
    const char *symbolic = "0,0,0,0,0,0,0,0";
    const char *dir_override = 0;
    int order = 0, have_order = 0, do_stdout = 0, do_test = 0;
    int want_flow = 0, want_meta = 0;
    FlowSession session;
    RingProg prog;
    char err[160];
    static char header[8192];
    static char line[SYMB_LINE];
    const char *kind;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *val = (i + 1 < argc) ? argv[i + 1] : 0;
        if (strcmp(a, "--self-test") == 0) do_test = 1;
        else if (strcmp(a, "--stdout") == 0) do_stdout = 1;
        else if (strcmp(a, "--flow") == 0) want_flow = 1;
        else if (strcmp(a, "--metaflow") == 0) want_meta = 1;
        else if (strcmp(a, "--help") == 0) { usage(); return 0; }
        else if (strcmp(a, "--order") == 0 && val) { order = atoi(val); have_order = 1; i++; }
        else if (strcmp(a, "--bases") == 0 && val) { bases = val; i++; }
        else if (strcmp(a, "--index") == 0 && val) { index_s = val; i++; }
        else if (strcmp(a, "--count") == 0 && val) { count_s = val; i++; }
        else if (strcmp(a, "--direction") == 0 && val) { direction = val; i++; }
        else if (strcmp(a, "--name") == 0 && val) { name = val; i++; }
        else if (strcmp(a, "--symbolic") == 0 && val) { symbolic = val; i++; }
        else if (strcmp(a, "--dir") == 0 && val) { dir_override = val; i++; }
        else { fprintf(stderr, "symb_gen: unknown argument %s\n", a); usage(); return 1; }
    }
    if (do_test) return self_test() ? 0 : 1;
    if (want_flow && want_meta) {
        fprintf(stderr, "symb_gen: --flow and --metaflow are mutually exclusive\n");
        return 1;
    }
    if (!have_order) { fprintf(stderr, "symb_gen: --order is required\n"); return 1; }
    kind = want_meta ? "MetaFlow" : "Flow";
    if (!bases) bases = want_meta ? "R,G,B,A" : "i,j,k,l,m,n,o,p,q,r,s,t,e,f,g";
    if (!name) name = want_meta ? "metaflow" : "flow";
    if (ring_compile(symbolic, &prog, err, sizeof err) != 0) {
        fprintf(stderr, "symb_gen: %s\n", err);
        return 1;
    }
    if (flow_session_begin(&session, bases, order, index_s, count_s, direction, name, err, sizeof err) != 0) {
        fprintf(stderr, "symb_gen: %s\n", err);
        return 1;
    }
    {
        char card_s[FLOW_DEC_MAX], count_out[FLOW_DEC_MAX];
        size_t n = 0;
        if (flow_session_cardinality(&session, card_s, sizeof card_s) != 0) return 1;
        if (flow_big_to_dec(&session.count, count_out, sizeof count_out) != 0) return 1;
        header[0] = 0;
        if (!append(header, sizeof header, &n, "@") || !append(header, sizeof header, &n, name)) return 1;
        if (!append(header, sizeof header, &n, "::") || !append(header, sizeof header, &n, kind)) return 1;
        if (!append(header, sizeof header, &n, ".") || !append_int(header, sizeof header, &n, order)) return 1;
        if (!append(header, sizeof header, &n, ".") || !append(header, sizeof header, &n, index_s)) return 1;
        if (!append(header, sizeof header, &n, ".") || !append(header, sizeof header, &n, count_out)) return 1;
        if (!append(header, sizeof header, &n, ":{\n    $bases:{ ")) return 1;
        for (int i = 0; i < session.n_bases; i++) {
            if (i && !append(header, sizeof header, &n, ", ")) return 1;
            if (!append(header, sizeof header, &n, (char[2]){session.bases[i], 0})) return 1;
        }
        if (!append(header, sizeof header, &n, " };\n    $direction:\"")) return 1;
        if (!append(header, sizeof header, &n, session.rtl ? "RTL" : "LTR")) return 1;
        if (!append(header, sizeof header, &n, "\";\n    $symbolic:\"")) return 1;
        for (const char *p = symbolic; *p; p++) {
            if ((*p == '\\' || *p == '"') && !append(header, sizeof header, &n, "\\")) return 1;
            if (!append(header, sizeof header, &n, (char[2]){*p, 0})) return 1;
        }
        if (!append(header, sizeof header, &n, "\";\n    $cardinality:")) return 1;
        if (!append(header, sizeof header, &n, card_s)) return 1;
        if (!append(header, sizeof header, &n, ";\n};\n")) return 1;
    }
    {
        char placed[1024], path[1200];
        FILE *fp;
        if (place_tool(placed, sizeof placed, name, dir_override) != 0) {
            fprintf(stderr, "symb_gen: cannot choose an output directory\n");
            return 1;
        }
        snprintf(path, sizeof path, "%s\\%s.cah", placed, name);
        fp = fopen(path, "wb");
        if (!fp) { fprintf(stderr, "symb_gen: cannot write %s\n", path); return 1; }
        fwrite(header, 1, strlen(header), fp);
        fclose(fp);
        snprintf(path, sizeof path, "%s\\%s.cas", placed, name);
        fp = fopen(path, "wb");
        if (!fp) { fprintf(stderr, "symb_gen: cannot write %s\n", path); return 1; }
        for (;;) {
            int rc = emit_line(&session, &prog, kind, line, sizeof line, err, sizeof err);
            if (rc == 0) break;
            if (rc < 0) { fclose(fp); fprintf(stderr, "symb_gen: %s\n", err[0] ? err : "line overflow"); return 1; }
            fwrite(line, 1, strlen(line), fp);
            fputc('\n', fp);
        }
        fclose(fp);
    }
    if (do_stdout) {
        if (flow_session_begin(&session, bases, order, index_s, count_s, direction, name, err, sizeof err) != 0) return 1;
        for (;;) {
            int rc = emit_line(&session, &prog, kind, line, sizeof line, err, sizeof err);
            if (rc == 0) break;
            if (rc < 0) { fprintf(stderr, "symb_gen: %s\n", err[0] ? err : "line overflow"); return 1; }
            fwrite(line, 1, strlen(line), stdout);
            fputc('\n', stdout);
        }
    }
    return 0;
}
