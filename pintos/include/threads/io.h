/* 이 파일은 MIT의 6.828에 사용된 소스 코드에서 파생되었습니다.
   강의. 원본 저작권 표시는 전체 내용으로 복제됩니다.
   아래에. */

/*
 * 저작권 (C) 1997 매사추세츠 공과대학
 *
 * 본 소프트웨어는 저작권 보유자에 의해 제공됩니다.
 * 다음 라이선스로 제공합니다. 이 소프트웨어를 취득하거나 복사함으로써,
 * 귀하는 다음 사항을 읽고 이해했으며 준수할 것에 동의합니다.
 * 다음 이용 약관:
 *
 * 이 소프트웨어를 사용, 복사, 수정, 배포 및 판매할 수 있는 권한
 * 어떤 목적으로든 수수료나 로열티 없이 문서를 작성하는 것은
 * 본 고지의 전체 텍스트가 모든 소프트웨어 및 문서의 사본,
 * 또는 그 일부의 사본에 표시되는 경우 이에 따라 허가됩니다.
 * 귀하가 수정한 내용을 포함합니다.
 *
 * 이 소프트웨어는 "있는 그대로" 제공되며, 저작권 보유자는 명시적이든
 * 묵시적이든 어떠한 진술이나 보증도 하지 않습니다. 예를 들어, 이에
 * 한정되지 않고 상품성, 특정 목적 적합성, 또는 이 소프트웨어나 문서의
 * 사용이 제3자의 특허, 저작권, 상표권 또는 기타 권리를 침해하지
 * 않는다는 보증을 하지 않습니다. 저작권 보유자는 이 소프트웨어나
 * 문서의 사용에 대해 어떠한 책임도 지지 않습니다.
 *
 * 저작권 보유자의 이름과 상표는 사전 서면 허가 없이 이 소프트웨어와
 * 관련된 광고나 홍보에 사용할 수 없습니다. 이 소프트웨어와 관련 문서의
 * 저작권 소유권은 항상 저작권 보유자에게 있습니다. 모든 저작권자 목록은
 * 이 소프트웨어와 함께 제공된 AUTHORS 파일을 참조하십시오.
 * 모든 저작권 소유자 목록을 확인하세요.
 *
 * 이 파일은 이전 저작권이 있는 소프트웨어에서 파생되었을 수 있습니다.
 * 이 저작권은 AUTHORS 파일에 나열된 저작권 보유자가 변경한 부분에만
 * 적용됩니다. 이 파일의 나머지 부분은 아래에 나열된 저작권 고지가
 * 있는 경우 그 고지를 따릅니다.
 */

#ifndef THREADS_IO_H
#define THREADS_IO_H

#include <stddef.h>
#include <stdint.h>

/* PORT에서 바이트를 읽고 반환합니다. */
static inline uint8_t
inb (uint16_t port) {
	/* [IA32-v2a] " IN "을 참조하세요. */
	uint8_t data;
	asm volatile ("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

/* PORT에서 CNT바이트를 차례로 읽고 저장합니다.
   ADDR 에서 시작하는 버퍼에 넣습니다. */
static inline void
insb (uint16_t port, void *addr, size_t cnt) {
	/* [IA32-v2a] " INS "을 참조하세요. */
	asm volatile ("cld; repne; insb"
			: "=D" (addr), "=c" (cnt)
			: "d" (port), "0" (addr), "1" (cnt)
			: "memory", "cc");
}

/* PORT 에서 16비트를 읽고 반환합니다. */
static inline uint16_t
inw (uint16_t port) {
	uint16_t data;
	/* [IA32-v2a] " IN "을 참조하세요. */
	asm volatile ("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

/* PORT에서 CNT 16비트(하프워드) 단위를 하나씩 읽습니다.
   다른 하나를 추가하고 ADDR에서 시작하는 버퍼에 저장합니다. */
static inline void
insw (uint16_t port, void *addr, size_t cnt) {
	/* [IA32-v2a] " INS "을 참조하세요. */
	asm volatile ("cld; repne; insw"
			: "=D" (addr), "=c" (cnt)
			: "d" (port), "0" (addr), "1" (cnt)
			: "memory", "cc");
}

/* PORT에서 32비트를 읽고 반환합니다. */
static inline uint32_t
inl (uint16_t port) {
	/* [IA32-v2a] " IN "을 참조하세요. */
	uint32_t data;
	asm volatile ("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

/* PORT에서 CNT 32비트(워드) 단위를 차례로 읽습니다.
   ADDR 에서 시작하는 버퍼에 저장합니다. */
static inline void
insl (uint16_t port, void *addr, size_t cnt) {
	/* [IA32-v2a] " INS "을 참조하세요. */
	asm volatile ("cld; repne; insl"
			: "=D" (addr), "=c" (cnt)
			: "d" (port), "0" (addr), "1" (cnt)
			: "memory", "cc");
}

/* DATA 바이트를 PORT 에 씁니다. */
static inline void
outb (uint16_t port, uint8_t data) {
	/* [IA32-v2b] " OUT "을 참조하세요. */
	asm volatile ("outb %0,%w1" : : "a" (data), "d" (port));
}

/* CNT -바이트 버퍼에 있는 데이터의 각 바이트를 PORT에 씁니다.
   ADDR부터 시작합니다. */
static inline void
outsb (uint16_t port, const void *addr, size_t cnt) {
	/* [IA32-v2b] " OUTS "을 참조하세요. */
	asm volatile ("cld; repne; outsb"
			: "=S" (addr), "=c" (cnt)
			: "d" (port), "0" (addr), "1" (cnt)
			: "cc");
}

/* 16비트 DATA을 PORT에 씁니다. */
static inline void
outw (uint16_t port, uint16_t data) {
	/* [IA32-v2b] " OUT "을 참조하세요. */
	asm volatile ("outw %0,%w1" : : "a" (data), "d" (port));
}

/* PORT에 데이터의 각 16비트 단위(하프워드)를 씁니다.
   CNT - ADDR에서 시작하는 하프워드 버퍼. */
static inline void
outsw (uint16_t port, const void *addr, size_t cnt) {
	/* [IA32-v2b] " OUTS "을 참조하세요. */
	asm volatile ("cld; repne; outsw"
			: "=S" (addr), "=c" (cnt)
			: "d" (port), "0" (addr), "1" (cnt)
			: "cc");
}

/* 32비트 DATA을 PORT에 씁니다. */
static inline void
outl (uint16_t port, uint32_t data) {
	/* [IA32-v2b] " OUT "을 참조하세요. */
	asm volatile ("outl %0,%w1" : : "a" (data), "d" (port));
}

/* CNT -word에 있는 데이터의 각 32비트 단위(워드)를 PORT에 씁니다.
   ADDR에서 시작하는 버퍼. */
static inline void
outsl (uint16_t port, const void *addr, size_t cnt) {
	/* [IA32-v2b] " OUTS "을 참조하세요. */
	asm volatile ("cld; repne; outsl"
			: "=S" (addr), "=c" (cnt)
			: "d" (port), "0" (addr), "1" (cnt)
			: "cc");
}

#endif /* threads/io.h */
