/* FollowLang atom-layer parser. See canor_fw.h. */

#include "canor_fw.h"

#include <stdio.h>
#include <string.h>

const char *fw_sigil_name(fw_sigil s) {
  switch (s) {
    case FW_SIG_TYPE: return "&type";
    case FW_SIG_FUNC: return "%operator";
    case FW_SIG_VAR:  return "$equation";
    case FW_SIG_SYM:  return "@constant";
    default:          return "?";
  }
}

const char *fw_face_name(fw_face f) {
  switch (f) {
    case FW_FACE_CONSTRUCT: return "construction";
    case FW_FACE_WIRE:      return "serializable";
    case FW_FACE_PARAMS:    return "parameters";
    case FW_FACE_DESTRUCT:  return "destruction";
    case FW_FACE_OBSERVE:   return "observable";
    default:                return "?";
  }
}

canor_status fw_read_file(const char *path, char *buf, size_t cap, size_t *len) {
  FILE *f;
  long n;
  if (!path || !buf || cap == 0) return CANOR_ERR_ARGS;
  f = fopen(path, "rb");
  if (!f) return CANOR_ERR_NOT_FOUND;
  fseek(f, 0, SEEK_END);
  n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) { fclose(f); return CANOR_ERR_NOT_FOUND; }
  /* Refuse rather than truncate: a partial source would parse as a different
   * program (AGENTS.md invariant 3). */
  if ((size_t)n >= cap) { fclose(f); return CANOR_ERR_FULL; }
  if (fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); return CANOR_ERR_NOT_FOUND; }
  buf[n] = 0;
  fclose(f);
  if (len) *len = (size_t)n;
  return CANOR_OK;
}

/* --- scanner ---------------------------------------------------------------- */

typedef struct {
  const char *p;
  int line;
  fw_doc *d;
} fw_scan;

static void fw_fail(fw_scan *s, const char *msg) {
  if (s->d->err[0]) return;                 /* keep the FIRST failure */
  snprintf(s->d->err, sizeof s->d->err, "%s", msg);
  s->d->err_line = s->line;
}

/* Whitespace and (* nesting comments *). The canonical grammar's comments nest,
 * so a naive scan-to-first-`*)` would terminate early on a commented-out block
 * that itself contains a comment - which is exactly what a file full of
 * explanatory prose is likely to contain. */
static void fw_ws(fw_scan *s) {
  for (;;) {
    while (*s->p == ' ' || *s->p == '\t' || *s->p == '\r' || *s->p == '\n') {
      if (*s->p == '\n') ++s->line;
      ++s->p;
    }
    if (s->p[0] == '(' && s->p[1] == '*') {
      int depth = 0;
      for (;;) {
        if (!*s->p) { fw_fail(s, "unterminated comment"); return; }
        if (s->p[0] == '(' && s->p[1] == '*') { ++depth; s->p += 2; continue; }
        if (s->p[0] == '*' && s->p[1] == ')') {
          --depth; s->p += 2;
          if (depth == 0) break;
          continue;
        }
        if (*s->p == '\n') ++s->line;
        ++s->p;
      }
      continue;
    }
    return;
  }
}

static bool fw_ident_ch(char c, bool first) {
  if (c >= 'a' && c <= 'z') return true;
  if (c >= 'A' && c <= 'Z') return true;
  if (c == '_') return true;
  if (!first && c >= '0' && c <= '9') return true;
  return false;
}

/* An identifier, or a bare integer - level paths like `$di.2` need the latter. */
static bool fw_ident(fw_scan *s, char *out, int cap) {
  int k = 0;
  if (*s->p >= '0' && *s->p <= '9') {
    while (*s->p >= '0' && *s->p <= '9' && k < cap - 1) out[k++] = *s->p++;
    out[k] = 0;
    return k > 0;
  }
  if (!fw_ident_ch(*s->p, true)) return false;
  while (fw_ident_ch(*s->p, false) && k < cap - 1) out[k++] = *s->p++;
  out[k] = 0;
  return true;
}

/* Copies a balanced group, excluding the outer delimiters. Nesting is tracked
 * so an inner grouping of the same kind does not close the outer one. */
static bool fw_balanced(fw_scan *s, char open, char close, char *out, int cap) {
  int depth = 0, k = 0;
  if (*s->p != open) return false;
  for (;;) {
    if (!*s->p) { fw_fail(s, "unterminated grouping"); return false; }
    if (*s->p == open) {
      ++depth;
      ++s->p;
      if (depth == 1) continue;             /* drop the outermost opener */
    } else if (*s->p == close) {
      --depth;
      ++s->p;
      if (depth == 0) break;                /* drop the outermost closer */
    } else {
      if (*s->p == '\n') ++s->line;
      ++s->p;
    }
    if (k < cap - 1) out[k++] = s->p[-1];
  }
  out[k] = 0;
  /* trim */
  while (k > 0 && (out[k-1]==' '||out[k-1]=='\n'||out[k-1]=='\t'||out[k-1]=='\r'))
    out[--k] = 0;
  return true;
}

static fw_sigil fw_sigil_of(char c) {
  switch (c) {
    case '&': return FW_SIG_TYPE;
    case '%': return FW_SIG_FUNC;
    case '$': return FW_SIG_VAR;
    case '@': return FW_SIG_SYM;
    default:  return FW_SIG_NONE;
  }
}

/* SIGIL IDENT { DOT IDENT } */
static bool fw_head(fw_scan *s, fw_sigil *sg, char seg[][FW_NAME_MAX],
                    int *nseg, char *joined, int jcap) {
  int n = 0, k = 0;
  *sg = fw_sigil_of(*s->p);
  if (*sg == FW_SIG_NONE) return false;
  ++s->p;
  if (!fw_ident(s, seg[0], FW_NAME_MAX)) { fw_fail(s, "sigil without a name"); return false; }
  n = 1;
  while (*s->p == '.' && n < FW_MAX_PATH) {
    ++s->p;
    if (!fw_ident(s, seg[n], FW_NAME_MAX)) { fw_fail(s, "dot without a name"); return false; }
    ++n;
  }
  *nseg = n;
  for (int i = 0; i < n && k < jcap - 2; ++i) {
    if (i) joined[k++] = '.';
    for (const char *q = seg[i]; *q && k < jcap - 1; ++q) joined[k++] = *q;
  }
  joined[k] = 0;
  return true;
}

bool fw_parse(const char *text, fw_doc *d) {
  fw_scan s;
  if (!text || !d) return false;
  memset(d, 0, sizeof *d);
  s.p = text; s.line = 1; s.d = d;

  for (;;) {
    fw_atom *a;
    fw_ws(&s);
    if (d->err[0]) return false;
    if (!*s.p) break;
    if (d->n >= FW_MAX_ATOMS) { fw_fail(&s, "too many atoms"); return false; }

    a = &d->a[d->n];
    memset(a, 0, sizeof *a);
    a->line = s.line;

    if (!fw_head(&s, &a->sigil, a->seg, &a->nseg, a->head, sizeof a->head)) {
      if (!d->err[0]) fw_fail(&s, "expected a sigil head");
      return false;
    }
    fw_ws(&s);
    /* An optional SECOND head: `&Master.M $cd.di ...` */
    if (fw_sigil_of(*s.p) != FW_SIG_NONE) {
      if (!fw_head(&s, &a->sigil2, a->seg2, &a->nseg2, a->head2, sizeof a->head2))
        return false;
      fw_ws(&s);
    }

    while (s.p[0] == ':' && s.p[1] == ':' && a->ntmpl < FW_MAX_TMPL) {
      fw_template *t = &a->tmpl[a->ntmpl];
      s.p += 2;
      fw_ws(&s);
      if (!fw_ident(&s, t->name, FW_NAME_MAX)) { fw_fail(&s, "template without a name"); return false; }
      if (*s.p == '(') {
        if (!fw_balanced(&s, '(', ')', t->arg, FW_FACE_MAX)) return false;
        t->has_arg = true;
      }
      ++a->ntmpl;
      fw_ws(&s);
    }

    /* `:=` must be tested before `:` - the lexer's own longest-match rule. */
    if (s.p[0] == ':' && s.p[1] == '=') {
      s.p += 2;
      a->is_schema = true;
      fw_ws(&s);
      /* Faces in fixed order, each optional. The two StructureGroupings are
       * distinguished only by position, so a lone `{...}` binds greedily to
       * construction - destruction-only is inexpressible by design. */
      if (*s.p == '{') {
        if (!fw_balanced(&s, '{', '}', a->face[FW_FACE_CONSTRUCT], FW_FACE_MAX)) return false;
        a->has_face[FW_FACE_CONSTRUCT] = true;
        fw_ws(&s);
      }
      if (*s.p == '[') {
        if (!fw_balanced(&s, '[', ']', a->face[FW_FACE_WIRE], FW_FACE_MAX)) return false;
        a->has_face[FW_FACE_WIRE] = true;
        fw_ws(&s);
      }
      if (*s.p == '(') {
        if (!fw_balanced(&s, '(', ')', a->face[FW_FACE_PARAMS], FW_FACE_MAX)) return false;
        a->has_face[FW_FACE_PARAMS] = true;
        fw_ws(&s);
      }
      if (*s.p == '{') {
        if (!fw_balanced(&s, '{', '}', a->face[FW_FACE_DESTRUCT], FW_FACE_MAX)) return false;
        a->has_face[FW_FACE_DESTRUCT] = true;
        fw_ws(&s);
      }
      /* The observable is the TAGGED form only. A bare `<...>` is ambiguous
       * with a comparison chain, so it is not accepted here at all. */
      if (s.p[0] == '<' && s.p[1] == '|') {
        int k = 0;
        s.p += 2;
        while (*s.p && !(s.p[0] == '|' && s.p[1] == '>')) {
          if (*s.p == '\n') ++s.line;
          if (k < FW_FACE_MAX - 1) a->face[FW_FACE_OBSERVE][k++] = *s.p;
          ++s.p;
        }
        if (!*s.p) { fw_fail(&s, "unterminated <| observable |>"); return false; }
        s.p += 2;
        a->face[FW_FACE_OBSERVE][k] = 0;
        a->has_face[FW_FACE_OBSERVE] = true;
        fw_ws(&s);
      } else if (*s.p == '<') {
        fw_fail(&s, "bare <...> is ambiguous with a comparison chain; "
                    "use the tagged form <| ... |>");
        return false;
      }
    } else if (*s.p == ':') {
      int k = 0;
      ++s.p;
      fw_ws(&s);
      while (*s.p && *s.p != ';') {
        if (*s.p == '\n') ++s.line;
        if (k < FW_FACE_MAX - 1) a->value[k++] = *s.p;
        ++s.p;
      }
      a->value[k] = 0;
      while (k > 0 && (a->value[k-1]==' '||a->value[k-1]=='\n'||
                       a->value[k-1]=='\t'||a->value[k-1]=='\r'))
        a->value[--k] = 0;
    } else {
      fw_fail(&s, "expected ':' (a value) or ':=' (a schema)");
      return false;
    }

    fw_ws(&s);
    if (*s.p != ';') { fw_fail(&s, "expected ';' to close the atom"); return false; }
    ++s.p;
    ++d->n;
  }
  return d->err[0] == 0;
}

/* --- lookup ----------------------------------------------------------------- */

const fw_atom *fw_find(const fw_doc *d, fw_sigil s, const char *head) {
  int i;
  if (!d || !head) return NULL;
  for (i = 0; i < d->n; ++i) {
    const fw_atom *a = &d->a[i];
    /* The declared name is the SECOND head when there are two: in
     * `&Master.M $cd.di` the atom declares cd.di, not Master.M. */
    if (a->nseg2 > 0) {
      if (a->sigil2 == s && !strcmp(a->head2, head)) return a;
    } else if (a->sigil == s && !strcmp(a->head, head)) {
      return a;
    }
  }
  return NULL;
}

int fw_scan_refs(const char *face, fw_sigil s, char out[][FW_NAME_MAX], int cap) {
  const char *p = face;
  char want = s == FW_SIG_FUNC ? '%' : s == FW_SIG_VAR ? '$'
            : s == FW_SIG_SYM  ? '@' : '&';
  int n = 0;
  if (!face) return 0;
  while (*p && n < cap) {
    if (*p == want) {
      int k = 0;
      ++p;
      while (fw_ident_ch(*p, k == 0) && k < FW_NAME_MAX - 1) out[n][k++] = *p++;
      /* keep dotted tails, so `$di.2` is not truncated to `di` */
      while (*p == '.' && k < FW_NAME_MAX - 2) {
        int j = k;
        out[n][k++] = '.';
        ++p;
        while ((fw_ident_ch(*p, false) || (*p >= '0' && *p <= '9')) &&
               k < FW_NAME_MAX - 1)
          out[n][k++] = *p++;
        if (k == j + 1) { k = j; break; }     /* a trailing dot is not a segment */
      }
      out[n][k] = 0;
      if (k) ++n;
      continue;
    }
    ++p;
  }
  return n;
}

bool fw_has_tmpl(const fw_atom *a, const char *tmpl) {
  int i;
  if (!a || !tmpl) return false;
  for (i = 0; i < a->ntmpl; ++i) if (!strcmp(a->tmpl[i].name, tmpl)) return true;
  return false;
}

const char *fw_tmpl_field(const fw_atom *a, const char *tmpl,
                          const char *field, char *buf) {
  int i;
  if (!a || !tmpl || !field || !buf) return NULL;
  for (i = 0; i < a->ntmpl; ++i) {
    const char *q;
    if (strcmp(a->tmpl[i].name, tmpl)) continue;
    q = strstr(a->tmpl[i].arg, field);
    if (!q) return NULL;
    q += strlen(field);
    while (*q == ' ' || *q == ':' || *q == '=') ++q;
    {
      int k = 0;
      while (*q && *q != ',' && *q != ')' && k < FW_NAME_MAX - 1) buf[k++] = *q++;
      while (k > 0 && buf[k-1] == ' ') --k;
      buf[k] = 0;
      return buf;
    }
  }
  return NULL;
}
