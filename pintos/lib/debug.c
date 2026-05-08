#include <debug.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* 호출 스택, 즉 주소 목록을 인쇄합니다.
   우리가 중첩된 각 기능. gdb 또는 addr2line
   이것을 파일 이름으로 변환하기 위해 kernel.o에 적용할 수 있습니다.
   줄 번호, 함수 이름.  */
void
debug_backtrace (void) {
	static bool explained;
	void **frame;

	printf ("Call stack:");
	for (frame = __builtin_frame_address (0);
			frame != NULL && frame[0] != NULL;
			frame = frame[0])
		printf (" %p", frame[1]);
	printf (".\n");

	if (!explained) {
		explained = true;
		printf ("The `backtrace' program can make call stacks useful.\n"
				"Read \"Backtraces\" in the \"Debugging Tools\" chapter\n"
				"of the Pintos documentation for more information.\n");
	}
}
