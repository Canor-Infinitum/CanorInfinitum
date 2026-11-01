#### *./core/\*/*: **Core `Types` used**, containerized in each of its own file:
 - **`BASIS`**: *generic signed value-implementation [T: (i8, i16, i32, i64, isize)] != 0*;
 - **`NUM`**: *generic value type [T: (i8, i16, i32, i64, isize, u8, u16, u32, u64, usize), F: (i8, i16, i32, i64, isize)] {sign : BASIS[i8], value : T, exponent : BASIS[F] = 1, basis : BASIS[F] = 1}*
 - **`NUMBER`**: *factoradic-implementation of an `Array of NUMBER`*;
 - **`INT`**: *named 2-tuple type of NUMBER `{lo : NUMBER, hi : NUMBER}`;
 - **`TOKEN`**: *trait which contains specification of an `.eval()`-function which returns an INT*;
 - **`VALUE`**: *generic symbolic value type [T: (INT, NUMBER, String)] {pre : T, peri : T, post : T} with **TOKEN**-Implementation*;
 - **`REFERENCE`**: *generic symbolic reference type [T: (INT, NUMBER, String)] {pre : @T, peri : @T, post : @T} with **TOKEN**-Implementation*;
 - **`FUNCTIONAL`**: *generic symbolic functional type [T: (INT, NUMBER, String)] {pre : function returning T, peri : function returning T, post : function returning T} with **TOKEN**-Implementation*;
 - **`OBSERVABLE`**: *generic symbolic observables type [T: (VALUE, REFERENCE, FUNCTIONAL), F: (INT, NUMBER, String)] {pre : T[F], peri : T[F], post : T[F]} **TOKEN**-Implementation*;
 - **`EXPR`**: *generic type-alias [T: (VALUE, REFERENCE, FUNCTIONAL)] : `OBSERVABLE[T, String]`*
 - **`OP`**: *generic trait over `[T: (VALUE, REFERENCE, FUNCTIONAL), F: (INT, NUMBER, String)]`, which contains specification of an `.eval()`-function which returns an OBSERVABLE[T, F]*;
 - **`OPERATOR`**: *strict infix-only generic OP-Implementation over [T: (VALUE, REFERENCE, FUNCTIONAL), F: (INT, NUMBER, String)] for returning an OBSERVABLE[T, F]. Semantics similar to {LHS : OBSERVABLE::pre, OP : OBSERVABLE::peri, RHS : OBSERVABLE::post}*;
 - **`DATA`**: *generic 3-tuple `{ low : T, value : T, high : T }` of a 3-tuple `{ min : T, nom : T, max : T }` of an OBSERVABLE (implementing TOKEN)*;
 - **`REAL`**: *generic one-tuple type [T: (DATA, NUMBER, OBSERVABLE[..., ...])] {val : T}*;
 - **`INTERVAL`**: *generic two-tuple type [T: (DATA, NUMBER, OBSERVABLE[..., ...])]: {lo : T, hi : T}*;
 - **`IMAG`**: *Enumeration array of 3-tuple Extended Imaginary-Trifectas (4xTrifectas)*;
 - **`IMAGINARY`**: *strict type {imag : IMAG, ring : usize} IMAG-Implementation (Hamilton's Rules: Column Arithmetic)*;
 - **`BASUM`**: *trait which contains specifications of an `.eval()`-function which returns an IMAGINARY*;
 - **`BASE`**: *strict double-linked-list of IMAGINARY nodes with **BASUM**-Implementation*;
 - **`MODULATION`**: *strict type `{real : REAL, bounds : INTERVAL, base : BASE}`*;
 - **`CALCULUS`**: *trait which contains specification of an `.eval()`-function which returns a MODULATION*;
 - **`TENSORIAL`**: *strict type `{data : Array of MODULATION, regular : Boolean, symmetric : Boolean, antisymmetric : Boolean, unknownsymmetric : Boolean, group_id : usize}`*;
 - **`TENSOR`**: *generic tensor type [T: (VALUE, REFERENCE, FUNCTIONAL)] `{input : EXPR[T], covariant : Array of TENSORIAL, contravariant : Array of TENSORIAL}` with **CALCULUS**-Implementation*;
 - **`MEASUREMENT`**: *generic type [T: (VALUE, REFERENCE, FUNCTIONAL], F: (VALUE, REFERENCE, FUNCTIONAL]) `{reals : Array of REAL, intervals : Array of INTERVAL, base : Array of TENSOR[T], operator : OPERATOR[F, ...]}`*. Formerly named UNIT. The in-house name Unit is a different product.*;
 - **`ATOM`**: *generic sub-type [F: (VALUE, REFERENCE, FUNCTIONAL)] `MEASUREMENT[VALUE, F]`*;
 - **`CHANGE`**: *generic sub-type [F: (VALUE, REFERENCE, FUNCTIONAL)] `MEASUREMENT[REFERENCE, F]`*;
 - **`FORCE`**: *generic sub-type [F: (VALUE, REFERENCE, FUNCTIONAL)] `MEASUREMENT[FUNCTIONAL, F]`*;
 - **`EXPRESSION`**: *generic expression type [T: (VALUE, REFERENCE, FUNCTIONAL), F: (VALUE, REFERENCE, FUNCTIONAL), V: (VALUE, REFERENCE, FUNCTIONAL)] `{input : EXPR[T], output : Array of MEASUREMENT[F, V]}`*;
 - **`BBST`**: *generic kd-mapped Balanced Binary Search Tree (contains left and right nodes), the Nodes of which contain an EXPRESSION property (`left : @Node, right : @Node, value : EXPRESSION, ...`)*;
 - **`TST`**: *generic Ternary Search Tree (contains left, middle and right BBST-leaves)*;
 
# Overall Algorithm

## High-level Language `DEFINITIONS` of `Types` used:

 - BASIS
	- Each **`BASIS`** is a generic signed integer-implementation generated from the regex result of: `"^[+-][1-9][0-9]*$"`;
		 - Input: STRING denoting NUMBER BASIS (the BASIS of which, due to the regex chosen, must be a positive or negative whole number != 0)
		 - Output: An `.eval()`-function, which results for `BASIS < 0`: The absolute fractional `1/|BASIS|` and for `BASIS > 0`: Just the value `BASIS` itself.

 - NUM / NUMBER / INT
	- Each **`NUM`** is a generic type `{sign : i8, value : T, basis : BASIS = 1, exponent : BASIS = 1}` with `sign ∈ [-2, -1, +1, +2] ( = "[-0, -1, +1, +0]-signage")`.
	- Each **`NUMBER`** is a factoradics implementation of an `Array of NUM`.
	- Each **`INT`** is just a two-tuple of a `NUMBER`
	
 - VALUE / REFERENCE / FUNCTIONAL: (`TOKEN`-IMPLEMENTATIONS)
	- **`VALUE`** / **`REFERENCE`** / **`FUNCTIONAL`**: These are Symbolic or Rich Interval Arithmetic-types containing Quantum Uncertainty by-default (when evaluated as for example pre <= peri <= post).
	- **`TOKEN`**: As they implement an `.eval()`-function, they can be evaluated for returning an INT.
	
 - OBSERVABLE / EXPR / OPERATOR
	- Each **`OBSERVABLE`** is a richer implementation specifying the Quantum Uncertainty-embedded `pre`-, `peri`-, `post`-valued three-tuple (used with for example `pre <= peri <= post`).
	- Each **`EXPR`** is an `OBSERVABLE[T, String]` limited to only String values.
	- Each **`OPERATOR`** just specifies an `.eval()`-function which returns such a Quantum Uncertainty-embedded `OBSERVABLE[T, F]`.
	
 - REAL / INTERVAL
	- Each **`REAL`** is a one-tuple of either a NUMBER or an OBSERVABLE.
	- Each **`INTERVAL`** is a two-tuple of either a NUMBER or an OBSERVABLE.

 - IMAG / IMAGINARY / BASUM / BASE
	- Each **`IMAG`** is just `[one=0, (i=1, j=2, k=3), (m=4, n=5, o=6), (r=7, s=8, t=9), (e=10, f=11, g=12)]`.
	- Each **`IMAGINARY`** is a Ringed Hamilton's Rules-Column Arithmetic "Extended Imaginary Bases": `{imag : IMAG, ring : usize}`.
	- Each **`BASUM`** just specifies an `.eval()`-function whcih returns a IMAGINARY.
	- Each **`BASE`** is a Doubly-Linked-List of IMAGINARY and implements BASUM: A permutation group.
	
 - MODULATION / CALCULUS
	- Each **`MODULATION`** is a `type {real : REAL, bounds : INTERVAL, base : BASE}`.
	- Each **`CALCULUS`** just specifies an `.eval()`-function which returns a MODULATION.

 - TENSORIAL / TENSOR
	- Each **`TENSORIAL`** contains an `Array of MODULATION` and specifies group symmetry-properties.
	- Each **`TENSOR`** is a *Symbolic Expression*-evaluation based upon an `EXPR[T]` with the trait that its `.eval()`-function uses the dataset `covariant : Array of TENSORIAL, contravariant : Array of TENSORIAL` for tensor-calculus, and results in an MODULATION.
	
 - MEASUREMENT / ATOM / CHANGE / FORCE
	- Each **`MEASUREMENT`** is a generic type `{reals : Array of REAL, intervals : Array of INTERVAL, bases : Array of TENSOR[T], operator : OPERATOR[F, ...]}`, for arbitrary operators: This is the canonical full-spectrum (most generic) analytical carte-blance tensor-calculus type. This record was formerly named UNIT.
	- Each **`ATOM`** is a generic sub-typed `MEASUREMENT[VALUE, F]` for generic `F: (VALUE, REFERENCE, FUNCTIONAL)`.
	- Each **`CHANGE`** is a generic sub-typed `MEASUREMENT[REFERENCE, F]` for generic `F: (VALUE, REFERENCE, FUNCTIONAL)`.
	- Each **`FORCE`** is a generic sub-typed `MEASUREMENT[FUNCTIONAL, F]` for generic `F: (VALUE, REFERENCE, FUNCTIONAL)`.
	
 - EXPRESSION
	- Each **`EXPRESSION`** contains `{input : EXPR[T], output : Array of MEASUREMENT[F]}` for generic `T, F: (VALUE, REFERENCE, FUNCTIONAL)`.
 
  - BBST & TST
	- Each **`BBST`** is a kd-mapped Balanced Binary Search Tree of `EXPRESSIONs`.
	- Each **`TST`** is a Ternary Search Tree with Relative Cayley-Dickson-Minkowski Deltas per Edge, containing three (3x) 3D kd-mapped Balanced Binary Search Trees (`BBSTs`) as its LEAVES.

 - NODES / LEAVES
	- Each **`NODE`** is a `Node` in the Ternary Search Tree (`TST`), containing Strictly-ordered PRE-/PERI-/POST-Processing LEAVES.
	- Each **`NODE`** contains an Cayley-Dickson-Minkowski Spacetime-`INTERVAL`-Delta named `ORIGO : CayleyDicksonConstruction` relative to the Gravity Well's/Parental Node's `ORIGO`.
	- Each **`LEAF`** contains a kd-mapped Balanced Binary Search Tree (`BBST`) of `EXPRESSIONS` such that with: `(PRE-Node: CONSTRAINTS, PERI-Node: VALUES, POST-Node: FREEDOMS)` the following applies:
		1. **`PRE-Node`**: CONSTRAINTS; of which is applied to the `NODE's` `ORIGO-INTERVAL` ("relative to the NODE's ORIGO"), resulting in a 3D kd-mapped `3D-OBJECT`.
		2. **`PERI-Node`**: VALUES; of which is applied to the `3D-OBJECT` (again, relative to the NODE's ORIGO), resulting in a Volumetric 3D kd-mapped `VOLUMETRIC-OBJECT`.
		3. **`POST-Node`**: FREEDOMS; of which is applied to the `VOLUMETRIC-OBJECT` (relative to the NODE's ORIGO), now resulting in a Temporally-Animated 3D Volumetric `ANIMATED-OBJECT`.
 
 - STEP-WISE ITERATION
	- For each step-wise **`ITERATION`** of the `TST`:
		1. Apply the `PRE NODE's` kd-mapped `BBST` to the `Origo` into a new `Volumetric Lorentzian-Frame`.
		2. Apply the `PERI NODE's` kd-mapped `BBST` to the `Volumetric Lorentzian-Frame` as additional Topological/Geometrical Data: `A Volumetric Topological Spacetime Object`.
		3. Apply the `POST NODE's` kd-mapped `BBST` to the `Spacetime Object`, as additional Temporal Structure: `A Temporally-Animated Volumetric Topological Spacetime Object`.
