/* Each CANOR_V1-ALPHA core type has its own file under core/<TYPE>/. */
#include "core/BASIS/BASIS.h"
#include "core/NUM/NUM.h"
#include "core/NUMBER/NUMBER.h"
#include "core/INT/INT.h"
#include "core/TOKEN/TOKEN.h"
#include "core/VALUE/VALUE.h"
#include "core/REFERENCE/REFERENCE.h"
#include "core/FUNCTIONAL/FUNCTIONAL.h"
#include "core/OBSERVABLE/OBSERVABLE.h"
#include "core/EXPR/EXPR.h"
#include "core/OP/OP.h"
#include "core/OPERATOR/OPERATOR.h"
#include "core/DATA/DATA.h"
#include "core/REAL/REAL.h"
#include "core/INTERVAL/INTERVAL.h"
#include "core/IMAG/IMAG.h"
#include "core/IMAGINARY/IMAGINARY.h"
#include "core/BASUM/BASUM.h"
#include "core/BASE/BASE.h"
#include "core/MODULATION/MODULATION.h"
#include "core/CALCULUS/CALCULUS.h"
#include "core/TENSORIAL/TENSORIAL.h"
#include "core/TENSOR/TENSOR.h"
#include "core/UNIT/UNIT.h"
#include "core/ATOM/ATOM.h"
#include "core/CHANGE/CHANGE.h"
#include "core/FORCE/FORCE.h"
#include "core/EXPRESSION/EXPRESSION.h"
#include "core/BBST/BBST.h"
#include "core/TST/TST.h"
#include "core/NODE/NODE.h"
#include "core/LEAF/LEAF.h"

int main(void) {
  return (int)(sizeof(BASIS) + sizeof(NUM) + sizeof(NUMBER) + sizeof(INT) +
               sizeof(TOKEN) + sizeof(VALUE) + sizeof(REFERENCE) +
               sizeof(FUNCTIONAL) + sizeof(OBSERVABLE) + sizeof(EXPR) +
               sizeof(OP) + sizeof(OPERATOR) + sizeof(DATA) + sizeof(REAL) +
               sizeof(INTERVAL) + sizeof(IMAG) + sizeof(IMAGINARY) +
               sizeof(BASUM) + sizeof(BASE) + sizeof(MODULATION) +
               sizeof(CALCULUS) + sizeof(TENSORIAL) + sizeof(TENSOR) +
               sizeof(UNIT) + sizeof(ATOM) + sizeof(CHANGE) + sizeof(FORCE) +
               sizeof(EXPRESSION) + sizeof(BBST) + sizeof(TST) + sizeof(NODE) +
               sizeof(LEAF)) == 0;
}
