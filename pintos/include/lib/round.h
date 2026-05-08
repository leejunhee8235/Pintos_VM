#ifndef __LIB_ROUND_H
#define __LIB_ROUND_H

/* X는 STEP의 가장 가까운 배수로 반올림됩니다.
 * X >= 0의 경우 STEP >= 1만 해당됩니다. */
#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP) * (STEP))

/* X를 STEP로 나누어 반올림한 결과를 얻습니다.
 * X >= 0의 경우 STEP >= 1만 해당됩니다. */
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

/* X는 STEP의 가장 가까운 배수로 내림됩니다.
 * X >= 0의 경우 STEP >= 1만 해당됩니다. */
#define ROUND_DOWN(X, STEP) ((X) / (STEP) * (STEP))

/* DIV_ROUND_DOWN 이 없습니다. 그것은 단순히 X / STEP 입니다. */

#endif /* lib/round.h */
