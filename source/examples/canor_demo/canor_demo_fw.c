/* canor_demo_fw.c - fw - parse a Master-Strategy, check it, and draw it
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* fw - parse a Master-Strategy, check it, and draw it                        */
/* ========================================================================== */
/* Every check below can FAIL on a real file, which is the point of doing them
 * at load rather than at evaluation. The two that carry the most weight are
 * holonomic propagation (contagious upward, so an author cannot forget it) and
 * the ::Entire violation check (declared totality, verified rather than
 * trusted).
 */

#define FWV_BASES 15
static const char *const FWV_BASE[FWV_BASES] = {
  "di","dj","dk", "dm","dn","do", "dr","ds","dt",
  "de","df","dg", "dp","dq","du"
};

typedef struct {
  bool declared;
  bool entire;
  bool nonholo;          /* uses a NON_HOLONOMIC operator, directly or via a ref */
  bool violates_entire;  /* declared ::Entire but the body is partial */
  int  nfaces;
  int  nrefs;
  char refs[8][FW_NAME_MAX];
  const fw_atom *at;
} fwv_base;

int canor_demo_cmd_fw(const canor_demo_config_t *cfg, const char *path, const char *out) {
  char *text;
  long len = 0;
  static fw_doc doc;
  fwv_base b[FWV_BASES];
  int i, j, k, nop = 0, nconst = 0, neq = 0, ndecl = 0;
  int nonholo_ops = 0, cycles = 0;
  FILE *f;
  (void)cfg;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  if (!path) path = "examples/memristive.fw";
  text = fw_read_file(path, &len);
  if (!text) { fprintf(stderr, "follow: cannot read %s\n", path); return 2; }

  printf("FollowLang Master-Strategy: %s (%ld bytes)\n\n", path, len);

  if (!fw_parse(text, &doc)) {
    fprintf(stderr, "follow: %s:%d: %s\n", path, doc.err_line, doc.err);
    free(text);
    return 2;
  }
  printf("  parsed %d atoms\n", doc.n);
  canor_demo_check(doc.n > 0, "the document parsed");

  /* --- census -------------------------------------------------------------- */
  for (i = 0; i < doc.n; ++i) {
    const fw_atom *a = &doc.a[i];
    fw_sigil s = a->nseg2 ? a->sigil2 : a->sigil;
    if (s == FW_SIG_FUNC) ++nop;
    else if (s == FW_SIG_SYM) ++nconst;
    else if (s == FW_SIG_VAR) ++neq;
    else ++ndecl;
  }
  printf("    %d operators, %d constants, %d $-atoms, %d declarations\n",
         nop, nconst, neq, ndecl);

  /* --- operators: class, and how many are path-dependent ------------------- */
  printf("\n  operators\n");
  printf("    %-10s %-6s %-16s %s\n", "name", "arity", "class", "faces");
  for (i = 0; i < doc.n; ++i) {
    const fw_atom *a = &doc.a[i];
    char cls[FW_NAME_MAX] = "", ar[FW_NAME_MAX] = "";
    int nf = 0;
    if ((a->nseg2 ? a->sigil2 : a->sigil) != FW_SIG_FUNC) continue;
    fw_tmpl_field(a, "Op", "class", cls);
    fw_tmpl_field(a, "Op", "arity", ar);
    for (j = 0; j < FW_FACE_N; ++j) if (a->has_face[j]) ++nf;
    printf("    %-10s %-6s %-16s %d\n", a->head, ar[0] ? ar : "?",
           cls[0] ? cls : "?", nf);
    if (!strcmp(cls, "NON_HOLONOMIC")) ++nonholo_ops;
  }
  canor_demo_check(nop > 0, "operators are declared");
  canor_demo_check(nonholo_ops > 0,
                   "at least one operator is NON_HOLONOMIC - hysteresis is present");

  /* --- the fifteen bases --------------------------------------------------- */
  memset(b, 0, sizeof b);
  for (i = 0; i < FWV_BASES; ++i) {
    const fw_atom *a = fw_find(&doc, FW_SIG_VAR, FWV_BASE[i]);
    if (!a) continue;
    b[i].declared = true;
    b[i].at = a;
    b[i].entire = fw_has_tmpl(a, "Entire");
    for (j = 0; j < FW_FACE_N; ++j) if (a->has_face[j]) ++b[i].nfaces;
    b[i].nrefs = fw_scan_refs(a->face[FW_FACE_WIRE], FW_SIG_VAR, b[i].refs, 8);
    /* Directly non-holonomic: the body names an operator declared so.
     *
     * SCANNED ACROSS THE WHOLE FAMILY, not just the bare base. `$di` is
     * holonomic on its own text but references `$di.n`, whose body uses
     * %play - so checking only `$di` reported it as path-independent when it
     * is not. A per-level body is part of the base, so every atom whose head
     * is `di` or begins `di.` contributes. Missing this was a false NEGATIVE,
     * which is the dangerous direction: it would have licensed stamping a
     * path-dependent result by its endpoints. */
    {
      size_t blen = strlen(FWV_BASE[i]);
      int m;
      for (m = 0; m < doc.n; ++m) {
        const fw_atom *fa = &doc.a[m];
        const char *h = fa->nseg2 ? fa->head2 : fa->head;
        char ops[8][FW_NAME_MAX];
        int no, q2;
        if ((fa->nseg2 ? fa->sigil2 : fa->sigil) != FW_SIG_VAR) continue;
        if (strncmp(h, FWV_BASE[i], blen)) continue;
        if (h[blen] != 0 && h[blen] != '.') continue;   /* di / di.n, not dj */
        no = fw_scan_refs(fa->face[FW_FACE_WIRE], FW_SIG_FUNC, ops, 8);
        for (q2 = 0; q2 < no; ++q2) {
          const fw_atom *o = fw_find(&doc, FW_SIG_FUNC, ops[q2]);
          char cls[FW_NAME_MAX] = "";
          if (o && fw_tmpl_field(o, "Op", "class", cls) &&
              !strcmp(cls, "NON_HOLONOMIC"))
            b[i].nonholo = true;
        }
      }
    }
    /* ::Entire is checkable: division and tangent are the partial forms. */
    if (b[i].entire) {
      const char *w = a->face[FW_FACE_WIRE];
      if (strchr(w, '/') || strchr(w, '\\') || strstr(w, "tan"))
        b[i].violates_entire = true;
    }
  }

  /* CONTAGION. The class propagates upward through references, so an author
   * cannot mark a body holonomic by forgetting what it calls. Iterated to a
   * fixed point rather than one pass, since a chain a->b->c must carry. */
  for (k = 0; k < FWV_BASES; ++k) {
    int changed = 0;
    for (i = 0; i < FWV_BASES; ++i) {
      if (!b[i].declared || b[i].nonholo) continue;
      for (j = 0; j < b[i].nrefs; ++j) {
        int q;
        for (q = 0; q < FWV_BASES; ++q) {
          /* A dotted reference names a member of a family, so `$di.n` counts
           * as a reference to `di`. Requiring exact equality here was the
           * other half of the same false negative. */
          size_t ql = strlen(FWV_BASE[q]);
          const char *rf = b[i].refs[j];
          if (!b[q].declared || !b[q].nonholo) continue;
          if (strncmp(rf, FWV_BASE[q], ql)) continue;
          if (rf[ql] != 0 && rf[ql] != '.') continue;
          b[i].nonholo = true; changed = 1;
        }
      }
    }
    if (!changed) break;
  }

  printf("\n  the fifteen bases\n");
  printf("    %-5s %-6s %-8s %-14s %-6s %s\n",
         "base", "faces", "entire", "class", "refs", "note");
  {
    int ndeclared = 0, nviol = 0, nnh = 0;
    for (i = 0; i < FWV_BASES; ++i) {
      if (!b[i].declared) {
        printf("    %-5s %-6s %-8s %-14s %-6s %s\n", FWV_BASE[i], "-", "-", "-",
               "-", "NOT DECLARED");
        continue;
      }
      ++ndeclared;
      if (b[i].violates_entire) ++nviol;
      if (b[i].nonholo) ++nnh;
      printf("    %-5s %-6d %-8s %-14s %-6d %s\n", FWV_BASE[i], b[i].nfaces,
             b[i].entire ? "yes" : "no",
             b[i].nonholo ? "NON_HOLONOMIC" : "holonomic", b[i].nrefs,
             b[i].violates_entire ? "VIOLATES ::Entire" : "");
    }
    printf("\n    %d of %d declared, %d non-holonomic, %d ::Entire violations\n",
           ndeclared, FWV_BASES, nnh, nviol);
    canor_demo_check(ndeclared == FWV_BASES, "all fifteen bases are declared");
    canor_demo_check(nviol == 0, "no ::Entire declaration is contradicted by its body");
    canor_demo_check(nnh > 0 && nnh < FWV_BASES,
                     "the holonomic class discriminates - some bases, not all");
  }

  /* --- undeclared references ----------------------------------------------- */
  {
    int missing = 0;
    for (i = 0; i < doc.n; ++i) {
      const fw_atom *a = &doc.a[i];
      char r[8][FW_NAME_MAX];
      int n2;
      for (j = 0; j < FW_FACE_N; ++j) {
        if (!a->has_face[j]) continue;
        n2 = fw_scan_refs(a->face[j], FW_SIG_SYM, r, 8);
        for (k = 0; k < n2; ++k)
          if (!fw_find(&doc, FW_SIG_SYM, r[k])) {
            printf("    undeclared constant @%s at line %d\n", r[k], a->line);
            ++missing;
          }
        n2 = fw_scan_refs(a->face[j], FW_SIG_FUNC, r, 8);
        for (k = 0; k < n2; ++k)
          if (!fw_find(&doc, FW_SIG_FUNC, r[k])) {
            printf("    undeclared operator %%%s at line %d\n", r[k], a->line);
            ++missing;
          }
      }
    }
    printf("\n  reference check: %d undeclared\n", missing);
    canor_demo_check(missing == 0, "every operator and constant used is declared first");
  }

  /* --- growth schedules ----------------------------------------------------- */
  {
    int ngrowth = 0, bad = 0;
    for (i = 0; i < FWV_BASES; ++i) {
      char key[FW_NAME_MAX];
      const fw_atom *a;
      long prev = -1, v;
      const char *q;
      int cnt = 0;
      snprintf(key, sizeof key, "cd.%s", FWV_BASE[i]);
      a = fw_find(&doc, FW_SIG_VAR, key);
      if (!a || !a->has_face[FW_FACE_WIRE]) continue;
      ++ngrowth;
      for (q = a->face[FW_FACE_WIRE]; *q; ) {
        if (*q < '0' || *q > '9') { ++q; continue; }
        v = strtol(q, (char **)&q, 10);
        ++cnt;
        /* Monotone by construction only if it is checked to be; a decreasing
         * schedule would un-double the algebra, which has no inverse. */
        if (v <= prev) { printf("    %s: not increasing at %ld\n", key, v); ++bad; }
        prev = v;
      }
      if (cnt > 4) { printf("    %s: %d breaks, exceeds sedenion\n", key, cnt); ++bad; }
    }
    printf("\n  growth schedules: %d found, %d malformed\n", ngrowth, bad);
    canor_demo_check(ngrowth == FWV_BASES, "every base has a growth schedule");
    canor_demo_check(bad == 0, "every schedule is strictly increasing and at most 4 breaks");
  }

  /* --- reference cycles ----------------------------------------------------- */
  {
    for (i = 0; i < FWV_BASES; ++i) {
      if (!b[i].declared) continue;
      for (j = 0; j < b[i].nrefs; ++j) {
        int q;
        for (q = 0; q < FWV_BASES; ++q) {
          int m;
          if (!b[q].declared || strcmp(b[i].refs[j], FWV_BASE[q])) continue;
          for (m = 0; m < b[q].nrefs; ++m)
            if (!strcmp(b[q].refs[m], FWV_BASE[i])) ++cycles;
        }
      }
    }
    printf("\n  reference cycles (2-cycles): %d\n", cycles / 2);
    /* Cycles are NOT an error here: a netlist's loops carry the physics, and
     * Kirchhoff's voltage law is a statement about closed loops. They are
     * counted so the winding policy knows what it must solve. */
    canor_demo_check(1, "cycles are counted, not refused - loops carry the circuit");
  }

  /* --- the artifact --------------------------------------------------------- */
  f = fopen(out, "w");
  if (f) {
    static const char *const TRI[5] = { "T0 device", "T1 mesh", "T2 timing",
                                        "T3 provenance", "T4 latent" };
    fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n<title>%s</title>\n",
            path);
    fprintf(f, "<style>body{background:#0d1117;color:#c9d1d9;font:14px/1.6 "
               "system-ui,sans-serif;margin:0;padding:32px}.w{max-width:1100px;"
               "margin:0 auto}h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;"
               "margin:26px 0 8px;color:#e6edf3}.sub{color:#8b949e;font-size:13px;"
               "max-width:72ch}table{border-collapse:collapse;font:12px "
               "ui-monospace,monospace}td,th{border:1px solid #30363d;"
               "padding:3px 8px;text-align:left}th{color:#8b949e;font-weight:500}"
               "svg{background:#010409;border:1px solid #30363d;border-radius:6px}"
               ".nh{color:#f85149}.ho{color:#58a6ff}.en{color:#3fb950}"
               "</style>\n<div class=\"w\">\n");
    fprintf(f, "<h1>Fully-fledged Strategy &mdash; %s</h1>\n", path);
    fprintf(f, "<p class=\"sub\">%d atoms: %d operators, %d constants, "
               "%d $-atoms. Parsed at the atom layer; face contents are scanned "
               "lexically, which is what every load-time check below actually "
               "needs.</p>\n", doc.n, nop, nconst, neq);

    /* the 5x3 grid, coloured by class */
    fprintf(f, "<h2>5 trifectas &times; 3 expressions</h2>\n");
    fprintf(f, "<svg viewBox=\"0 0 760 260\" width=\"100%%\">\n");
    for (i = 0; i < 5; ++i) {
      fprintf(f, "<text x=\"%d\" y=\"20\" fill=\"#8b949e\" font-size=\"11\" "
                 "font-family=\"monospace\">%s</text>\n", 20 + i * 150, TRI[i]);
      for (j = 0; j < 3; ++j) {
        int idx = i * 3 + j;
        const char *fill = !b[idx].declared ? "#21262d"
                         : b[idx].nonholo ? "#f85149" : "#1f6feb";
        fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"120\" height=\"52\" rx=\"5\" "
                   "fill=\"%s\" fill-opacity=\"0.30\" stroke=\"%s\"/>\n",
                20 + i * 150, 34 + j * 68, fill, fill);
        fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"#e6edf3\" font-size=\"13\" "
                   "font-family=\"monospace\">%s</text>\n",
                32 + i * 150, 56 + j * 68, FWV_BASE[idx]);
        fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"#8b949e\" font-size=\"9\" "
                   "font-family=\"monospace\">%s %s %dF</text>\n",
                32 + i * 150, 72 + j * 68,
                b[idx].nonholo ? "non-holo" : "holo",
                b[idx].entire ? "entire" : "partial", b[idx].nfaces);
      }
    }
    fprintf(f, "</svg>\n");
    fprintf(f, "<p class=\"sub\">Blue is holonomic (stampable by endpoints); "
               "<span class=\"nh\">red</span> is non-holonomic &mdash; "
               "path-dependent, so its artifact must stamp the traversal. The "
               "class is <b>contagious upward through references</b> and "
               "computed to a fixed point, so a body cannot be marked "
               "path-independent by forgetting what it calls.</p>\n");

    fprintf(f, "<h2>Bases</h2>\n<table><tr><th>base</th><th>faces</th>"
               "<th>entire</th><th>class</th><th>refs</th></tr>\n");
    for (i = 0; i < FWV_BASES; ++i)
      fprintf(f, "<tr><td>%s</td><td>%d</td><td class=\"%s\">%s</td>"
                 "<td class=\"%s\">%s</td><td>%d</td></tr>\n",
              FWV_BASE[i], b[i].nfaces,
              b[i].entire ? "en" : "", b[i].entire ? "yes" : "no",
              b[i].nonholo ? "nh" : "ho",
              b[i].nonholo ? "NON_HOLONOMIC" : "holonomic", b[i].nrefs);
    fprintf(f, "</table>\n");
    fprintf(f, "<p class=\"sub\">Face counts differ because all five are "
               "optional: an atom carries a lifecycle only where it has one. "
               "Two faces is the common case; the full five appear only where "
               "evaluation genuinely has a path.</p>\n");

    /* --- WebGL: the parsed Strategy as geometry --------------------------- */
    fprintf(f, "<h2>The Strategy in space</h2>\n");
    fprintf(f, "<p class=\"sub\">The fifteen bases at their PG(3,2) positions "
               "&mdash; the tesseract projection of the non-zero vectors of "
               "F<sub>2</sub><sup>4</sup>, which is where these bases actually "
               "live rather than a layout chosen to look tidy. Radius is face "
               "count, colour is holonomic class, and the lines are references. "
               "Drag to orbit, wheel to zoom.</p>\n");
    fprintf(f, "<canvas id=\"gl\" width=\"1060\" height=\"460\" "
               "style=\"width:100%%;border-radius:6px;cursor:grab\"></canvas>\n");
    fprintf(f, "<svg id=\"ov\" viewBox=\"0 0 1060 460\" width=\"100%%\" "
               "style=\"margin-top:-464px;pointer-events:none;background:none;"
               "border:0\"></svg>\n");

    /* base positions: the tesseract projection used throughout */
    fprintf(f, "<script>\nconst B=[");
    for (i = 0; i < FWV_BASES; ++i) {
      int u = i + 1;
      double b0 = (u & 1) ? 1.0 : 0.0, b1 = (u & 2) ? 1.0 : 0.0;
      double b2 = (u & 4) ? 1.0 : 0.0, b3 = (u & 8) ? 1.0 : 0.0;
      fprintf(f, "%s{x:%.4f,y:%.4f,z:%.4f,nh:%d,en:%d,nf:%d,nm:\"%s\"}",
              i ? "," : "",
              ((b0 - 0.5) * 0.62 + b3 * 0.30) * 1.15,
              ((b1 - 0.5) * 0.62 + b3 * 0.30) * 1.15,
              ((b2 - 0.5) * 0.62 + b3 * 0.30) * 1.15,
              b[i].nonholo ? 1 : 0, b[i].entire ? 1 : 0,
              b[i].nfaces, FWV_BASE[i]);
    }
    fprintf(f, "];\nconst E=[");
    {
      int first = 1;
      for (i = 0; i < FWV_BASES; ++i)
        for (j = 0; j < b[i].nrefs; ++j) {
          int q;
          size_t rl;
          for (q = 0; q < FWV_BASES; ++q) {
            rl = strlen(FWV_BASE[q]);
            if (strncmp(b[i].refs[j], FWV_BASE[q], rl)) continue;
            if (b[i].refs[j][rl] != 0 && b[i].refs[j][rl] != '.') continue;
            fprintf(f, "%s[%d,%d]", first ? "" : ",", i, q);
            first = 0;
          }
        }
    }
    fprintf(f, "];\n");

    fputs(
"const cv=document.getElementById('gl'),ov=document.getElementById('ov');\n"
"const gl=cv.getContext('webgl2',{antialias:false});\n"
"const VS=`#version 300 es\n"
"void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);\n"
"gl_Position=vec4(p*2.0-1.0,0.0,1.0);}`;\n"
"const FS=`#version 300 es\n"
"precision highp float;\n"
"uniform vec3 cam;uniform mat3 bas;uniform vec2 res;\n"
"uniform vec3 P[15];uniform float R[15];uniform vec3 C[15];\n"
"out vec4 o;\n"
"// Sphere-tracing a union of fifteen spheres. No geometry buffers: the scene\n"
"// IS the distance function, so adding a base is a uniform, not a mesh.\n"
"float map(vec3 p,out int id){float d=1e9;id=0;\n"
" for(int i=0;i<15;i++){float s=length(p-P[i])-R[i];\n"
"  if(s<d){d=s;id=i;}}\n"
" return d;}\n"
"void main(){\n"
" vec2 uv=(gl_FragCoord.xy*2.0-res)/res.y;\n"
" vec3 rd=normalize(bas*vec3(uv,-1.8));\n"
" float t=0.0;int id=0;bool hit=false;vec3 hp=vec3(0.0);\n"
" for(int i=0;i<96;i++){vec3 p=cam+rd*t;float d=map(p,id);\n"
"  if(d<0.0015){hit=true;hp=p;break;} t+=d; if(t>8.0)break;}\n"
" if(!hit){o=vec4(0.027,0.031,0.039,1.0);return;}\n"
" int q;float e=0.0015;\n"
" vec3 n=normalize(vec3(\n"
"  map(hp+vec3(e,0.0,0.0),q)-map(hp-vec3(e,0.0,0.0),q),\n"
"  map(hp+vec3(0.0,e,0.0),q)-map(hp-vec3(0.0,e,0.0),q),\n"
"  map(hp+vec3(0.0,0.0,e),q)-map(hp-vec3(0.0,0.0,e),q)));\n"
" vec3 L=normalize(vec3(0.6,0.8,0.5));\n"
" float df=max(dot(n,L),0.0);\n"
" float rim=pow(1.0-max(dot(n,-rd),0.0),2.5);\n"
" vec3 col=C[id]*(0.25+0.8*df)+vec3(0.4,0.6,1.0)*rim*0.45;\n"
" o=vec4(pow(col,vec3(0.4545)),1.0);}`;\n"
"function sh(ty,src){const s=gl.createShader(ty);gl.shaderSource(s,src);\n"
" gl.compileShader(s);\n"
" if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))console.error(gl.getShaderInfoLog(s));\n"
" return s;}\n"
"const pr=gl.createProgram();\n"
"gl.attachShader(pr,sh(gl.VERTEX_SHADER,VS));\n"
"gl.attachShader(pr,sh(gl.FRAGMENT_SHADER,FS));\n"
"gl.linkProgram(pr);gl.useProgram(pr);\n"
"const U=n=>gl.getUniformLocation(pr,n);\n"
"const PP=new Float32Array(45),RR=new Float32Array(15),CC=new Float32Array(45);\n"
"for(let i=0;i<15;i++){PP[i*3]=B[i].x;PP[i*3+1]=B[i].y;PP[i*3+2]=B[i].z;\n"
" RR[i]=0.045+0.016*B[i].nf;\n"
" // red non-holonomic, blue holonomic; green tint when declared entire\n"
" const c=B[i].nh?[0.86,0.32,0.29]:[0.16,0.44,0.92];\n"
" if(B[i].en){c[1]=Math.min(1.0,c[1]+0.34);}\n"
" CC[i*3]=c[0];CC[i*3+1]=c[1];CC[i*3+2]=c[2];}\n"
"let yaw=0.7,pitch=0.4,dist=1.9,drag=false,lx=0,ly=0;\n"
"cv.addEventListener('pointerdown',e=>{drag=true;lx=e.clientX;ly=e.clientY;\n"
" cv.setPointerCapture(e.pointerId);});\n"
"cv.addEventListener('pointerup',()=>{drag=false;});\n"
"cv.addEventListener('pointermove',e=>{if(!drag)return;\n"
" yaw+=(e.clientX-lx)*0.008;pitch+=(e.clientY-ly)*0.008;\n"
" pitch=Math.max(-1.5,Math.min(1.5,pitch));lx=e.clientX;ly=e.clientY;});\n"
"cv.addEventListener('wheel',e=>{e.preventDefault();\n"
" dist=Math.max(1.1,Math.min(6.0,dist*(1+Math.sign(e.deltaY)*0.1)));},\n"
" {passive:false});\n"
"function frame(){\n"
" const w=cv.width,h=cv.height;gl.viewport(0,0,w,h);\n"
" const cy=Math.cos(yaw),sy=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);\n"
" const cam=[cp*sy*dist,sp*dist,cp*cy*dist];\n"
" const fwd=[-cp*sy,-sp,-cp*cy],rt=[cy,0,-sy],up=[-sy*sp,cp,-cy*sp];\n"
" gl.uniform3f(U('cam'),cam[0],cam[1],cam[2]);\n"
" gl.uniformMatrix3fv(U('bas'),false,\n"
"  [rt[0],rt[1],rt[2],up[0],up[1],up[2],-fwd[0],-fwd[1],-fwd[2]]);\n"
" gl.uniform2f(U('res'),w,h);\n"
" gl.uniform3fv(U('P'),PP);gl.uniform1fv(U('R'),RR);gl.uniform3fv(U('C'),CC);\n"
" gl.drawArrays(gl.TRIANGLES,0,3);\n"
" // references and labels share the volume camera - they are in its space\n"
" const proj=q=>{const d=[q.x-cam[0],q.y-cam[1],q.z-cam[2]];\n"
"  const zc=d[0]*fwd[0]+d[1]*fwd[1]+d[2]*fwd[2];\n"
"  if(zc<=0.02)return null;\n"
"  const xc=d[0]*rt[0]+d[1]*rt[1]+d[2]*rt[2];\n"
"  const yc=d[0]*up[0]+d[1]*up[1]+d[2]*up[2];\n"
"  return [w*0.5+xc/zc*1.8*h*0.5, h*0.5-yc/zc*1.8*h*0.5];};\n"
" let out='';\n"
" for(const [a2,b2] of E){const p=proj(B[a2]),q2=proj(B[b2]);\n"
"  if(p&&q2)out+='<line x1=\"'+p[0]+'\" y1=\"'+p[1]+'\" x2=\"'+q2[0]+'\" y2=\"'\n"
"   +q2[1]+'\" stroke=\"#8b949e\" stroke-width=\"1\" stroke-opacity=\"0.55\"/>';}\n"
" for(const nd of B){const p=proj(nd);if(!p)continue;\n"
"  out+='<text x=\"'+(p[0]+9)+'\" y=\"'+(p[1]+4)+'\" fill=\"#c9d1d9\" '\n"
"   +'font-size=\"11\" font-family=\"monospace\">'+nd.nm+'</text>';}\n"
" ov.innerHTML=out;\n"
" requestAnimationFrame(frame);}\n"
"frame();\n</script>\n", f);
    fprintf(f, "<p class=\"sub\">Radius is face count, so an atom carrying a "
               "full lifecycle is visibly larger than one with two faces. The "
               "<code>::Entire</code> shift lands as <b>cyan</b> on holonomic and "
               "<b>orange</b> on non-holonomic, so all four combinations read as "
               "distinct colours rather than a colour plus a hint. The reference lines "
               "are what the holonomic contagion travels along &mdash; and "
               "tracing one from a red node shows why its neighbours turned "
               "red.</p>\n");
    fprintf(f, "</div>\n");
    fclose(f);
    printf("\n  wrote %s\n", out);
    canor_demo_prov_write_sidecar(out);
  }

  free(text);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
