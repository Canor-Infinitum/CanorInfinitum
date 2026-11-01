/* canor_demo_volume.c - volume - a 3D/4D quaternionic escape field, raymarched in the browser
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* volume - a 3D/4D quaternionic escape field, raymarched in the browser      */
/* ========================================================================== */
/* THE DIVISION OF LABOUR, AND WHY IT IS THIS WAY ROUND.
 *
 * The FIELD is computed here, in C, in fixed-point integers - no floating
 * point anywhere in the iteration. It is therefore bit-identical on every
 * host, reproducible from the config alone, and covered by the Golay stamp
 * like every other artifact. The BROWSER only displays it: the raymarcher
 * reads a finished volume and never computes an escape.
 *
 * That split is the point. A shader that iterated the quaternion itself would
 * be faster and would look the same, but its output would depend on the GPU's
 * float behaviour and could not be stamped, compared across machines, or
 * re-derived. Section 27 forbids nondeterminism, not approximation - so a
 * deterministic fixed-point field is admissible where a float one is not, and
 * the pixels are then just a rendering of data that already exists.
 *
 * FIXED POINT, Q40.24. Products are (a*b) >> 24 with |a|,|b| < 2^26, so the
 * intermediate fits int64 with room to spare. Escape is |q|^2 > 4, tested
 * exactly in the same representation. No rounding mode, no fused multiply-add,
 * nothing a compiler may reassociate: the same input gives the same integer
 * everywhere.
 *
 * --- WHY JULIA AND NOT ONLY MANDELBROT --------------------------------------
 *
 * For q, c in H the map q -> q^2 + c has
 *
 *     q^2  =  (a^2 - b^2 - c^2 - d^2)  +  2a(bi + cj + dk)
 *
 * so the imaginary part of q^2 stays PARALLEL to the imaginary part of q. If c
 * is taken as the voxel position with a fixed imaginary direction - the
 * Mandelbrot construction - the whole orbit is confined to the complex
 * subalgebra spanned by 1 and that direction. The result is exactly the 2D
 * Mandelbrot set revolved about the real axis: genuinely 3D to look at, and
 * carrying no more information than the 2D picture it came from. That was
 * checked earlier and it is stated here rather than quietly rendered as though
 * it were a discovery.
 *
 * A JULIA set with c a FIXED quaternion having all four components non-zero is
 * not confined that way, and is genuinely three-dimensional. So both modes are
 * offered and labelled: the Mandelbrot mode to see the familiar set and why it
 * is a solid of revolution, the Julia mode to see structure that a 2D slice
 * cannot reconstruct.
 *
 * --- WHAT MAKES IT PER-STRATEGY ---------------------------------------------
 *
 * In Julia mode the constant c is derived from the Strategy's own differential
 * families - one component per trifecta, folded through canor_flowvalue_add under
 * the Strategy's declared bias. A fully-fledged Strategy therefore supplies
 * all four components from its own structure, and two Strategies give two
 * different sets. A Strategy that declares one trifecta supplies one component
 * and the rest fall back to a stated default, which is reported rather than
 * hidden - an undeclared trifecta contributes nothing, exactly as it does
 * everywhere else in this tool.
 *
 * --- THE FOURTH DIMENSION ---------------------------------------------------
 *
 * The volume is computed at several values of the real coordinate w and all of
 * them are embedded, so the slider scrubs a real fourth axis rather than
 * re-rendering the same solid. That is why the file is large: it is 4D data,
 * not a 3D object with a spare control.
 */

#define VOL_N 48                 /* voxels per side */
#define VOL_W 8                  /* w-slices: the fourth axis */

int64_t canor_demo_vfp(double x) { return (int64_t)(x * (double)CANOR_DEMO_VOL_ONE); }
static int64_t vmul(int64_t a, int64_t b) { return (a * b) >> CANOR_DEMO_VOL_FP; }

/* One orbit, entirely in integers. Returns the escape iteration, or `fuel`
 * when it never escaped. */
int canor_demo_vol_escape(int64_t a, int64_t b, int64_t c2, int64_t d,
                      int64_t ca, int64_t cb, int64_t cc, int64_t cd,
                      int fuel) {
  int i;
  const int64_t LIMIT = 4LL << CANOR_DEMO_VOL_FP;
  for (i = 0; i < fuel; ++i) {
    int64_t n2 = vmul(a, a) + vmul(b, b) + vmul(c2, c2) + vmul(d, d);
    int64_t na, nb, nc, nd;
    if (n2 > LIMIT) return i;
    /* q^2 = (a^2 - b^2 - c^2 - d^2) + 2a(bi + cj + dk) */
    na = vmul(a, a) - vmul(b, b) - vmul(c2, c2) - vmul(d, d) + ca;
    nb = 2 * vmul(a, b) + cb;
    nc = 2 * vmul(a, c2) + cc;
    nd = 2 * vmul(a, d) + cd;
    /* A magnitude beyond the fixed-point range would wrap, and a wrapped
     * value is a plausible wrong number rather than a visible failure. The
     * orbit is already escaping by then, so it is reported as escaped. */
    if (na > (1LL << 50) || na < -(1LL << 50) ||
        nb > (1LL << 50) || nb < -(1LL << 50) ||
        nc > (1LL << 50) || nc < -(1LL << 50) ||
        nd > (1LL << 50) || nd < -(1LL << 50)) return i;
    a = na; b = nb; c2 = nc; d = nd;
  }
  return fuel;
}

/* The Strategy's own contribution to c: one component per trifecta, folded
 * under its declared bias. Returns how many trifectas actually supplied one. */
/* A BASE CONSTANT PLUS A BOUNDED PERTURBATION, and the structure matters.
 *
 * An earlier form of this mapped each FLOWVALUE to an ABSOLUTE component
 * value. It produced c = 0.24 + 0.60i + 0.20j + 0.20k for one Strategy, whose
 * Julia set is EMPTY - every one of 884736 voxels escaped. That is not a
 * finding about the Strategy; it is a badly chosen parameterisation, and it
 * would have made most Strategies render an empty box.
 *
 * The base below sits inside the connectedness locus, where the Julia set is
 * non-empty and has visible structure. The Strategy then supplies a BOUNDED
 * perturbation, so every Strategy lands somewhere interesting and two
 * Strategies still differ visibly.
 *
 * THIS IS ALSO WHAT MAKES THE VOXELISATIONS COMPARABLE. Same lattice, same
 * base, same fuel, same escape test - the map from Strategy to volume is one
 * fixed function, and any difference between two renders is attributable to
 * the Strategies alone. Had the base moved with the Strategy there would be no
 * common frame and the two pictures would not be evidence about each other. */
#define VOL_BASE_C { -0.20, 0.60, 0.20, 0.20 }
int canor_demo_vol_strategy_c(const canor_strategy *st, int64_t out[4]) {
  int t, supplied = 0;
  static const double BASE[4] = VOL_BASE_C;
  int32_t args[4] = { 2, 3, 5, 7 };
  for (t = 0; t < 4; ++t) out[t] = canor_demo_vfp(BASE[t]);
  for (t = 0; t < 4 && t < CANOR_TRIFECTAS; ++t) {
    FLOWVALUE acc = FV_PosZero;
    double d = 0.0;
    int k, got = 0;
    for (k = 0; k < 3; ++k) {
      FLOWVALUE v;
      if (!canor_dtc(st, t, k, args, 4, &v)) continue;
      acc = got ? canor_flowvalue_add(st->bias, &st->id, acc, v) : v;
      ++got;
    }
    if (!got) continue;                 /* undeclared: base, unperturbed */
    /* The four signed zeros map onto a symmetric perturbation. Arbitrary but
     * FIXED and declared, so two Strategies differ because their FLOWVALUEs
     * differ and not because the mapping drifted between runs. */
    switch ((int)acc & 3) {
      case 0: d = -0.12; break;   /* NegZero */
      case 1: d = -0.04; break;   /* NegOne  */
      case 2: d =  0.04; break;   /* PosZero */
      default: d = 0.12; break;   /* PosOne  */
    }
    out[t] = canor_demo_vfp(BASE[t] + d);
    ++supplied;
  }
  return supplied;
}

int canor_demo_cmd_volume(const canor_demo_config_t *c, int sel, const char *out, int fuel) {
  const canor_strategy *st = &c->strat[sel];
  unsigned char *fieldM = NULL, *fieldJ = NULL;
  size_t plane = (size_t)VOL_N * VOL_N * VOL_N;
  size_t total = plane * VOL_W;
  int64_t cq[4];
  int supplied, wi, ix, iy, iz;
  FILE *f;
  long solidM = 0, solidJ = 0;

  if (fuel <= 0 || fuel > 255) fuel = 64;
  fieldM = malloc(total);
  fieldJ = malloc(total);
  if (!fieldM || !fieldJ) { free(fieldM); free(fieldJ); return 2; }

  supplied = canor_demo_vol_strategy_c(st, cq);

  printf("Quaternionic escape field, %dx%dx%d x %d w-slices\n\n",
         VOL_N, VOL_N, VOL_N, VOL_W);
  printf("  strategy      %s (%d of %d trifectas)\n", st->id.name,
         st->ntrifecta, CANOR_TRIFECTAS);
  printf("  julia c       %.3f %+.3fi %+.3fj %+.3fk  (%d of 4 from the Strategy)\n",
         (double)cq[0] / CANOR_DEMO_VOL_ONE, (double)cq[1] / CANOR_DEMO_VOL_ONE,
         (double)cq[2] / CANOR_DEMO_VOL_ONE, (double)cq[3] / CANOR_DEMO_VOL_ONE, supplied);
  printf("  fuel          %d iterations\n", fuel);
  printf("  arithmetic    fixed-point Q40.24, no floating point in the orbit\n");

  for (wi = 0; wi < VOL_W; ++wi) {
    /* w runs over [-0.8, 0.8]: the real coordinate, and the fourth axis the
     * slider scrubs. */
    int64_t w = canor_demo_vfp(-0.8 + 1.6 * (double)wi / (double)(VOL_W - 1));
    for (iz = 0; iz < VOL_N; ++iz)
      for (iy = 0; iy < VOL_N; ++iy)
        for (ix = 0; ix < VOL_N; ++ix) {
          double sx = -1.5 + 3.0 * (double)ix / (double)(VOL_N - 1);
          double sy = -1.5 + 3.0 * (double)iy / (double)(VOL_N - 1);
          double sz = -1.5 + 3.0 * (double)iz / (double)(VOL_N - 1);
          int64_t y = canor_demo_vfp(sy), z = canor_demo_vfp(sz);
          size_t o = (size_t)wi * plane +
                     (size_t)iz * VOL_N * VOL_N + (size_t)iy * VOL_N + ix;
          int e;
          /* THE REAL PART GOES ON A SPATIAL AXIS, NOT ON THE SLIDER.
           *
           * q^2 keeps the imaginary part parallel, so a Mandelbrot escape
           * depends only on (Re c, |Im c|). If the real part rides the w
           * slider, every spatial slice depends on radius alone and renders as
           * a featureless BALL - which is what the first version drew. It was
           * not wrong arithmetic; it was the recognisable structure projected
           * onto the one axis that cannot show it.
           *
           * With the real part on x, the slice is the 2D Mandelbrot set
           * revolved about the x-axis: the cardioid and its bulbs, visible.
           * The x range is shifted to [-2.1, 0.9] because that is where the
           * set actually lives. */
          e = canor_demo_vol_escape(0, 0, 0, 0, canor_demo_vfp(sx - 0.6), y, z, w, fuel);
          fieldM[o] = (unsigned char)(e * 255 / fuel);
          if (e >= fuel) ++solidM;
          /* JULIA: q is the voxel, c fixed from the Strategy. c has all four
           * components non-zero, so no confinement and no revolution. */
          e = canor_demo_vol_escape(canor_demo_vfp(sx), y, z, w, cq[0], cq[1], cq[2], cq[3], fuel);
          fieldJ[o] = (unsigned char)(e * 255 / fuel);
          if (e >= fuel) ++solidJ;
        }
  }

  printf("  bounded set   mandelbrot %ld voxels, julia %ld of %lu\n",
         solidM, solidJ, (unsigned long)total);
  /* An empty Julia set renders an empty box, which looks like a bug and is
   * really a c outside the connectedness locus. Said plainly rather than left
   * for the viewer to discover by rotating nothing. */
  if (solidJ == 0)
    printf("  WARNING       the Julia set is EMPTY at this c - every voxel "
           "escaped\n");

  f = fopen(out, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", out); free(fieldM); free(fieldJ); return 2; }

  fputs("<!doctype html>\n<meta charset=\"utf-8\">\n", f);
  fprintf(f, "<title>%s - quaternionic volume</title>\n", st->id.name);
  fputs("<style>\n"
        "html,body{margin:0;height:100%;background:#07080a;color:#c9d1d9;"
        "font:13px/1.5 system-ui,sans-serif;overflow:hidden}\n"
        "#c{position:fixed;inset:0;width:100%;height:100%;display:block;cursor:grab}\n"
        "#c:active{cursor:grabbing}\n"
        "#ui{position:fixed;top:12px;left:12px;width:250px;padding:14px 16px;\n"
        "background:rgba(13,17,23,.88);border:1px solid #30363d;border-radius:8px;\n"
        "backdrop-filter:blur(8px)}\n"
        "h1{font-size:14px;margin:0 0 2px}\n"
        ".s{color:#8b949e;font-size:11px;margin:0 0 10px}\n"
        "label{display:block;margin:9px 0 2px;color:#8b949e;font-size:11px}\n"
        "input[type=range]{width:100%;accent-color:#58a6ff}\n"
        "button{background:#21262d;color:#c9d1d9;border:1px solid #30363d;\n"
        "border-radius:5px;padding:4px 9px;font:11px system-ui;cursor:pointer}\n"
        "button.on{background:#1f6feb;border-color:#1f6feb;color:#fff}\n"
        ".r{display:flex;gap:5px;margin-top:3px}\n"
        "#note{position:fixed;bottom:12px;left:12px;right:12px;max-width:640px;\n"
        "color:#6e7681;font-size:11px;line-height:1.5}\n"
        "#err{position:fixed;inset:0;display:none;place-items:center;padding:40px;\n"
        "color:#f85149;font:13px ui-monospace,monospace;text-align:center}\n"
        "</style>\n", f);
  fputs("<canvas id=\"c\"></canvas>\n<div id=\"err\"></div>\n<div id=\"ui\">\n", f);
  fprintf(f, "<h1>%s</h1>\n", st->id.name);
  fprintf(f, "<p class=\"s\">%d&sup3; &times; %d w-slices, fuel %d</p>\n",
          VOL_N, VOL_W, fuel);
  fputs("<div class=\"r\"><button id=\"bj\" class=\"on\">Julia</button>"
        "<button id=\"bm\">Mandelbrot</button></div>\n"
        "<label>w &mdash; the fourth axis <b id=\"lw\"></b></label>"
        "<input id=\"w\" type=\"range\" min=\"0\" max=\"", f);
  fprintf(f, "%d", VOL_W - 1);
  fputs("\" value=\"", f);
  fprintf(f, "%d", VOL_W / 2);
  fputs("\" step=\"1\">\n"
        "<label>surface threshold <b id=\"lt\"></b></label>"
        "<input id=\"t\" type=\"range\" min=\"2\" max=\"254\" value=\"200\">\n"
        "<label>zoom <b id=\"lz\"></b></label>"
        "<input id=\"z\" type=\"range\" min=\"60\" max=\"400\" value=\"170\">\n"
        "<div class=\"r\" style=\"margin-top:10px\">"
        "<button id=\"bv\">voxel blocks</button>"
        "<button id=\"br\">spin</button></div>\n"
        "</div>\n", f);
  fputs("<div id=\"note\"></div>\n", f);

  /* the data */
  fputs("<script id=\"dm\" type=\"application/octet-stream\">", f);
  canor_demo_b64_out(f, fieldM, total);
  fputs("</script>\n<script id=\"dj\" type=\"application/octet-stream\">", f);
  canor_demo_b64_out(f, fieldJ, total);
  fputs("</script>\n", f);

  fprintf(f, "<script>\nconst N=%d,W=%d,PLANE=N*N*N;\n", VOL_N, VOL_W);
  fprintf(f, "const NOTE=%s;\n",
          "\"Field computed in C in fixed-point Q40.24 - no floating point in "
          "the orbit - so it is bit-identical on every host and covered by the "
          "artifact's Golay stamp. The browser only displays it. Mandelbrot "
          "mode is a solid of revolution: q\\u00b2 keeps the imaginary part "
          "parallel, so the orbit stays in one complex subalgebra and the shape "
          "carries no more information than the 2D set. Julia mode, with c "
          "fixed and all four components non-zero, is not confined that way and "
          "is genuinely 3D.\"");

  fputs(
"function unb64(id){const s=document.getElementById(id).textContent.trim();\n"
" const bin=atob(s);const a=new Uint8Array(bin.length);\n"
" for(let i=0;i<bin.length;i++)a[i]=bin.charCodeAt(i);return a;}\n"
"const DM=unb64('dm'),DJ=unb64('dj');\n"
"const cv=document.getElementById('c');\n"
"const gl=cv.getContext('webgl2',{antialias:false});\n"
"function fail(m){const e=document.getElementById('err');\n"
" e.style.display='grid';e.textContent=m;cv.style.display='none';}\n"
"if(!gl){fail('This page needs WebGL2. The volume data is still embedded in the file.');}\n"
"else{\n"
"const VS=`#version 300 es\n"
"void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);\n"
"gl_Position=vec4(p*2.0-1.0,0.0,1.0);}`;\n"
"const FS=`#version 300 es\n"
"precision highp float;precision highp sampler3D;\n"
"uniform sampler3D V;uniform vec3 cam;uniform mat3 bas;\n"
"uniform float thr;uniform vec2 res;uniform float chunky;uniform float n;\n"
"out vec4 o;\n"
"float smp(vec3 p){vec3 q=p+0.5;\n"
" if(chunky>0.5)q=(floor(q*n)+0.5)/n;\n"
" return texture(V,q).r;}\n"
"bool box(vec3 ro,vec3 rd,out float t0,out float t1){\n"
" vec3 iv=1.0/rd,a=(vec3(-0.5)-ro)*iv,b=(vec3(0.5)-ro)*iv;\n"
" vec3 lo=min(a,b),hi=max(a,b);\n"
" t0=max(max(lo.x,lo.y),lo.z);t1=min(min(hi.x,hi.y),hi.z);\n"
" return t1>max(t0,0.0);}\n"
"void main(){\n"
" vec2 uv=(gl_FragCoord.xy*2.0-res)/res.y;\n"
" vec3 rd=normalize(bas*vec3(uv,-1.7));\n"
" float t0,t1;\n"
" if(!box(cam,rd,t0,t1)){o=vec4(0.027,0.031,0.039,1.0);return;}\n"
" t0=max(t0,0.0);\n"
" float dt=(t1-t0)/192.0,t=t0;bool hit=false;vec3 hp=vec3(0.0);\n"
" for(int i=0;i<192;i++){\n"
"  vec3 p=cam+rd*t;\n"
"  if(smp(p)>=thr){hit=true;hp=p;break;}\n"
"  t+=dt;if(t>t1)break;}\n"
" if(!hit){o=vec4(0.027,0.031,0.039,1.0);return;}\n"
" for(int i=0;i<5;i++){float h=dt*0.5;vec3 q=hp-rd*h;\n"
"  if(smp(q)>=thr)hp=q;dt=h;}\n"
" float e=1.6/n;\n"
" vec3 nr=normalize(-vec3(\n"
"  smp(hp+vec3(e,0.0,0.0))-smp(hp-vec3(e,0.0,0.0)),\n"
"  smp(hp+vec3(0.0,e,0.0))-smp(hp-vec3(0.0,e,0.0)),\n"
"  smp(hp+vec3(0.0,0.0,e))-smp(hp-vec3(0.0,0.0,e)))+vec3(1e-6));\n"
" vec3 L=normalize(vec3(0.6,0.8,0.5));\n"
" float d=max(dot(nr,L),0.0),rim=pow(1.0-max(dot(nr,-rd),0.0),2.5);\n"
" vec3 col=mix(vec3(0.10,0.30,0.62),vec3(0.85,0.55,0.25),\n"
"   clamp(hp.y+0.5,0.0,1.0));\n"
" col=col*(0.20+0.85*d)+vec3(0.35,0.55,0.95)*rim*0.55;\n"
" float fog=clamp((t-t0)/max(t1-t0,1e-4),0.0,1.0);\n"
" col=mix(col,vec3(0.027,0.031,0.039),fog*0.35);\n"
" o=vec4(pow(col,vec3(0.4545)),1.0);}`;\n"
"function sh(ty,src){const s=gl.createShader(ty);gl.shaderSource(s,src);\n"
" gl.compileShader(s);\n"
" if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))\n"
"   fail('shader: '+gl.getShaderInfoLog(s));\n"
" return s;}\n"
"const pr=gl.createProgram();\n"
"gl.attachShader(pr,sh(gl.VERTEX_SHADER,VS));\n"
"gl.attachShader(pr,sh(gl.FRAGMENT_SHADER,FS));\n"
"gl.linkProgram(pr);gl.useProgram(pr);\n"
"const tex=gl.createTexture();\n"
"gl.bindTexture(gl.TEXTURE_3D,tex);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_R,gl.CLAMP_TO_EDGE);\n"
"let mode='j',wi=Math.floor(W/2);\n"
"function upload(){const src=(mode==='j')?DJ:DM;\n"
" const sub=src.subarray(wi*PLANE,(wi+1)*PLANE);\n"
" gl.pixelStorei(gl.UNPACK_ALIGNMENT,1);\n"
" gl.texImage3D(gl.TEXTURE_3D,0,gl.R8,N,N,N,0,gl.RED,gl.UNSIGNED_BYTE,sub);}\n"
"upload();\n"
"const U=n=>gl.getUniformLocation(pr,n);\n"
"let yaw=0.7,pitch=0.5,dist=1.7,spin=false,chunky=false;\n"
"let drag=false,lx=0,ly=0;\n"
"cv.addEventListener('pointerdown',e=>{drag=true;lx=e.clientX;ly=e.clientY;\n"
" cv.setPointerCapture(e.pointerId);});\n"
"cv.addEventListener('pointerup',e=>{drag=false;});\n"
"cv.addEventListener('pointermove',e=>{if(!drag)return;\n"
" yaw+=(e.clientX-lx)*0.0075;pitch+=(e.clientY-ly)*0.0075;\n"
" pitch=Math.max(-1.55,Math.min(1.55,pitch));lx=e.clientX;ly=e.clientY;});\n"
"cv.addEventListener('wheel',e=>{e.preventDefault();\n"
" dist=Math.max(0.6,Math.min(4.0,dist*(1.0+Math.sign(e.deltaY)*0.09)));\n"
" document.getElementById('z').value=Math.round(dist*100);\n"
" document.getElementById('lz').textContent=dist.toFixed(2);},{passive:false});\n"
"const wS=document.getElementById('w'),tS=document.getElementById('t'),\n"
"      zS=document.getElementById('z');\n"
"function wtxt(){return (-0.8+1.6*wi/(W-1)).toFixed(2);}\n"
"wS.oninput=()=>{wi=+wS.value;upload();\n"
" document.getElementById('lw').textContent=wtxt();};\n"
"tS.oninput=()=>{document.getElementById('lt').textContent=tS.value;};\n"
"zS.oninput=()=>{dist=+zS.value/100;\n"
" document.getElementById('lz').textContent=dist.toFixed(2);};\n"
"const bj=document.getElementById('bj'),bm=document.getElementById('bm');\n"
"bj.onclick=()=>{mode='j';bj.classList.add('on');bm.classList.remove('on');upload();};\n"
"bm.onclick=()=>{mode='m';bm.classList.add('on');bj.classList.remove('on');upload();};\n"
"const bv=document.getElementById('bv'),br=document.getElementById('br');\n"
"bv.onclick=()=>{chunky=!chunky;bv.classList.toggle('on',chunky);\n"
" gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_MAG_FILTER,\n"
"   chunky?gl.NEAREST:gl.LINEAR);};\n"
"br.onclick=()=>{spin=!spin;br.classList.toggle('on',spin);};\n"
"document.getElementById('lw').textContent=wtxt();\n"
"document.getElementById('lt').textContent=tS.value;\n"
"document.getElementById('lz').textContent=dist.toFixed(2);\n"
"document.getElementById('note').textContent=NOTE;\n"
"function frame(){\n"
" const dpr=Math.min(devicePixelRatio||1,1.75);\n"
" const w=Math.floor(innerWidth*dpr),h=Math.floor(innerHeight*dpr);\n"
" if(cv.width!==w||cv.height!==h){cv.width=w;cv.height=h;}\n"
" gl.viewport(0,0,w,h);\n"
" if(spin)yaw+=0.004;\n"
" const cy=Math.cos(yaw),sy=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);\n"
" const fwd=[-cp*sy,-sp,-cp*cy];\n"
" const cam=[cp*sy*dist,sp*dist,cp*cy*dist];\n"
" const rt=[cy,0,-sy];\n"
" const up=[-sy*sp,cp,-cy*sp];\n"
" gl.uniform3f(U('cam'),cam[0],cam[1],cam[2]);\n"
" gl.uniformMatrix3fv(U('bas'),false,\n"
"  [rt[0],rt[1],rt[2],up[0],up[1],up[2],-fwd[0],-fwd[1],-fwd[2]]);\n"
" gl.uniform1f(U('thr'),(+tS.value)/255.0);\n"
" gl.uniform2f(U('res'),w,h);\n"
" gl.uniform1f(U('chunky'),chunky?1.0:0.0);\n"
" gl.uniform1f(U('n'),N);\n"
" gl.uniform1i(U('V'),0);\n"
" gl.drawArrays(gl.TRIANGLES,0,3);\n"
" requestAnimationFrame(frame);}\n"
"frame();\n"
"}\n</script>\n", f);

  fclose(f);
  free(fieldM);
  free(fieldJ);

  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  printf("  drag to orbit, wheel to zoom, w-slider scrubs the fourth axis\n");
  return 0;
}
