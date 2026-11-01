#ifndef CANOR_FW_H
#define CANOR_FW_H

/* A parser for the FollowLang subset that Master-Strategies are written in.
 *
 * SCOPE, STATED UP FRONT. This parses the ATOM LAYER exactly - sigil heads,
 * dotted paths, templates, the two definition operators, and the five
 * lifecycle faces. It does NOT parse the expression layer: a face's contents
 * are captured as balanced text and then scanned lexically for the references
 * it makes.
 *
 * That is a deliberate first increment rather than a shortcut, and the split
 * falls in a defensible place: every load-time check the Master-Strategy design
 * calls for - operator arity, undeclared references, growth monotonicity,
 * reference cycles, holonomic propagation, ::Entire violations - is answerable
 * from the atom structure plus a lexical scan. None of them needs an expression
 * tree. Building one now would be speculative work in service of no current
 * check, and it would be the part most likely to disagree with the canonical
 * grammar in ways nothing here would catch.
 *
 * WHAT THE TWO OPERATORS MEAN, since the parser depends on it:
 *
 *     :   a VALUE   - an Expr, non-nullable
 *     :=  a SCHEMA  - lifecycle faces, each optional, possibly empty
 *
 *         {construction} [serializable] (parameters) {destruction} <|observable|>
 *
 * Construction and destruction are both StructureGrouping, so a lone `{...}`
 * binds greedily to construction. Destruction-only is therefore inexpressible,
 * which is intended: nothing is retired that was never constructed.
 *
 * The observable is the TAGGED directive form. BRA and LESSTHAN are both "<",
 * KET and GREATERTHAN are both ">", and comparisons chain - so a bare
 * <observable> is genuinely ambiguous with `a < b > c`, and `<|` is the only
 * unambiguous opening.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FW_MAX_ATOMS    512
#define FW_MAX_TMPL     4
#define FW_MAX_PATH     6
#define FW_FACE_MAX     512
#define FW_NAME_MAX     48

typedef enum {
  FW_SIG_NONE = 0,
  FW_SIG_TYPE,      /* &  namespace / Master-Strategy */
  FW_SIG_FUNC,      /* %  declared operator           */
  FW_SIG_VAR,       /* $  equation / differential     */
  FW_SIG_SYM        /* @  declared constant           */
} fw_sigil;

const char *fw_sigil_name(fw_sigil s);

/* The five faces, in the order the schema fixes them. */
typedef enum {
  FW_FACE_CONSTRUCT = 0,
  FW_FACE_WIRE,          /* [serializable] - where an equation body lives */
  FW_FACE_PARAMS,
  FW_FACE_DESTRUCT,
  FW_FACE_OBSERVE,
  FW_FACE_N
} fw_face;

const char *fw_face_name(fw_face f);

typedef struct {
  char name[FW_NAME_MAX];
  char arg[FW_FACE_MAX];
  bool has_arg;
} fw_template;

typedef struct {
  fw_sigil sigil;
  /* The dotted head, kept both split and joined: split for lookup, joined for
   * reporting, so a diagnostic can quote what the author wrote. */
  char seg[FW_MAX_PATH][FW_NAME_MAX];
  int  nseg;
  char head[FW_NAME_MAX * 2];
  /* A second sigil head on the same atom, e.g. `&Master.M $cd.di`. */
  fw_sigil sigil2;
  char seg2[FW_MAX_PATH][FW_NAME_MAX];
  int  nseg2;
  char head2[FW_NAME_MAX * 2];

  fw_template tmpl[FW_MAX_TMPL];
  int ntmpl;

  bool is_schema;                 /* := rather than : */
  char face[FW_FACE_N][FW_FACE_MAX];
  bool has_face[FW_FACE_N];
  char value[FW_FACE_MAX];        /* when `:` */

  int line;
} fw_atom;

typedef struct {
  fw_atom a[FW_MAX_ATOMS];
  int n;
  char err[256];
  int  err_line;
} fw_doc;

/* Parses `text`. On failure returns false with doc->err and doc->err_line set;
 * a partial parse is left in place so a caller can report what was understood
 * before the failure rather than only that one occurred. */
bool fw_parse(const char *text, fw_doc *doc);

/* Reads a whole file into caller-placed `buf` of `cap` bytes, NUL-terminated
 * (AGENTS.md invariant 2: no heap in the core). CANOR_ERR_ARGS on bad input,
 * CANOR_ERR_NOT_FOUND when the file cannot be opened or read, CANOR_ERR_FULL
 * when the file plus terminator exceeds `cap` - never a truncated read. */
canor_status fw_read_file(const char *path, char *buf, size_t cap, size_t *len);

/* --- lookup ----------------------------------------------------------------- */

/* Finds an atom by sigil and head, e.g. (FW_SIG_VAR, "di.2"). Matches against
 * the SECOND head when the atom carries two, since `&Master.M $cd.di` is
 * declaring `cd.di`, not `Master.M`. */
const fw_atom *fw_find(const fw_doc *d, fw_sigil s, const char *head);

/* Scans a face for references of a given sigil, appending each name found.
 * Returns how many were written. This is the lexical scan the header describes:
 * enough for every declared check, and not a substitute for an expression
 * tree. */
int fw_scan_refs(const char *face, fw_sigil s, char out[][FW_NAME_MAX], int cap);

/* Reads a template argument by template name, e.g. tmpl "Op" arg "class".
 * Returns NULL when absent. `buf` must hold FW_NAME_MAX. */
const char *fw_tmpl_field(const fw_atom *a, const char *tmpl,
                          const char *field, char *buf);
bool fw_has_tmpl(const fw_atom *a, const char *tmpl);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_FW_H */
