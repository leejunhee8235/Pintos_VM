#ifndef THREADS_INTR_STUBS_H
#define THREADS_INTR_STUBS_H

/* 스텁을 인터럽트합니다.
 *
 * 이것은 intr-stubs.S에 있는 작은 코드 조각입니다.
 * 256개의 가능한 x86 인터럽트 각각. 각자는
 * 약간의 스택 조작 후 intr_entry() 으로 점프합니다.
 * 자세한 내용은 intr-stubs.S를 참조하세요.
 *
 * 이 배열은 각 인터럽트 스텁 진입점을 가리킵니다.
 * intr_init()이(가) 쉽게 찾을 수 있도록 말이죠. */
typedef void intr_stub_func (void);
extern intr_stub_func *intr_stubs[256];

#endif /* threads/intr-stubs.h */
