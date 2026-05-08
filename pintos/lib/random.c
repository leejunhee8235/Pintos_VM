#include "random.h"
#include <stdbool.h>
#include <stdint.h>
#include "debug.h"

/* RC4 기반 의사 난수 생성기(PRNG).

   RC4은 스트림 암호입니다. 우리는 그것을 여기에서 사용하지 않습니다.
   암호화 속성은 구현하기 쉽기 때문에
   그 출력은 비암호화에 대해 매우 무작위적입니다.
   목적.

   자세한 내용은 http://en.wikipedia.org/wiki/RC4_(cipher)을 참조하세요.
   RC4 에 있습니다.*/

/* RC4 상태. */
static uint8_t s[256];          /* 에스[]. */
static uint8_t s_i, s_j;        /* 나, 제이. */

/* 이미 초기화되었나요? */
static bool inited;     

/* A와 B가 가리키는 바이트를 교환합니다. */
static inline void
swap_byte (uint8_t *a, uint8_t *b) {
	uint8_t t = *a;
	*a = *b;
	*b = t;
}

/* 지정된 SEED을 사용하여 PRNG을 초기화하거나 다시 초기화합니다. */
void
random_init (unsigned seed) {
	uint8_t *seedp = (uint8_t *) &seed;
	int i;
	uint8_t j;

	for (i = 0; i < 256; i++) 
		s[i] = i;
	for (i = j = 0; i < 256; i++) {
		j += s[i] + seedp[i % sizeof seed];
		swap_byte (s + i, s + j);
	}

	s_i = s_j = 0;
	inited = true;
}

/* SIZE 임의 바이트를 BUF 에 씁니다. */
void
random_bytes (void *buf_, size_t size) {
	uint8_t *buf;

	if (!inited)
		random_init (0);

	for (buf = buf_; size-- > 0; buf++) {
		uint8_t s_k;

		s_i++;
		s_j += s[s_i];
		swap_byte (s + s_i, s + s_j);

		s_k = s[s_i] + s[s_j];
		*buf = s[s_k];
	}
}

/* 의사 난수 unsigned long을 반환합니다.
   random_ulong() % n을 사용하여 범위 내 임의의 숫자를 얻습니다.
   0...n(제외). */
unsigned long
random_ulong (void) {
	unsigned long ul;
	random_bytes (&ul, sizeof ul);
	return ul;
}
