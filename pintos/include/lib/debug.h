#ifndef __LIB_DEBUG_H
#define __LIB_DEBUG_H

/* GCC을 사용하면 함수에 "속성"을 추가할 수 있습니다.
 * 매개변수 등을 사용하여 해당 속성을 나타냅니다.
 * 자세한 내용은 GCC 설명서를 참조하세요. */
#define UNUSED __attribute__ ((unused))
#define NO_RETURN __attribute__ ((noreturn))
#define NO_INLINE __attribute__ ((noinline))
#define PRINTF_FORMAT(FMT, FIRST) __attribute__ ((format (printf, FMT, FIRST)))

/* OS을 중지하고 소스 파일 이름, 줄 번호 및
 * 함수 이름과 사용자별 메시지. */
#define PANIC(...) debug_panic (__FILE__, __LINE__, __func__, __VA_ARGS__)

void debug_panic (const char *file, int line, const char *function,
		const char *message, ...) PRINTF_FORMAT (4, 5) NO_RETURN;
void debug_backtrace (void);

#endif



/* 이는 헤더 가드 외부에 있으므로 debug.h가
 * NDEBUG 의 다른 설정으로 여러 번 포함되었습니다. */
#undef ASSERT
#undef NOT_REACHED

#ifndef NDEBUG
#define ASSERT(CONDITION)                                       \
	if ((CONDITION)) { } else {                             \
		PANIC ("assertion `%s' failed.", #CONDITION);   \
	}
#define NOT_REACHED() PANIC ("executed an unreachable statement");
#else
#define ASSERT(CONDITION) ((void) 0)
#define NOT_REACHED() for (;;)
#endif /* lib/debug.h */
