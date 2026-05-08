#include "threads/palloc.h"
#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "threads/init.h"
#include "threads/loader.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

/* 페이지 할당자. 페이지 크기(또는
   여러 페이지) 청크. 할당자는 malloc.h를 참조하세요.
   더 작은 덩어리를 나눠주세요.

   시스템 메모리는 커널이라는 두 개의 "풀"로 나뉩니다.
   그리고 사용자 풀. 사용자 풀은 사용자(가상) 메모리용입니다.
   페이지, 그 밖의 모든 것을 위한 커널 풀. 여기서 아이디어는
   커널에는 자체 작업을 위한 메모리가 필요합니다.
   사용자 프로세스가 미친 듯이 바뀌더라도 말이죠.

   기본적으로 시스템 RAM의 절반은 커널 풀에 제공되며
   사용자 풀의 절반. 그것은 엄청난 과잉이 될 것입니다.
   커널 풀이지만 데모 목적으로는 괜찮습니다. */

/* 메모리 풀. */
struct pool {
	struct lock lock;               /* 상호 배제. */
	struct bitmap *used_map;        /* 무료 페이지의 비트맵. */
	uint8_t *base;                  /* 수영장의 기초. */
};

/* 풀 2개: 하나는 커널 데이터용이고 다른 하나는 사용자 페이지용입니다. */
static struct pool kernel_pool, user_pool;

/* 사용자 풀에 넣을 최대 페이지 수입니다. */
size_t user_page_limit = SIZE_MAX;
static void
init_pool (struct pool *p, void **bm_base, uint64_t start, uint64_t end);

static bool page_from_pool (const struct pool *, void *page);

/* 멀티부팅 정보 */
struct multiboot_info {
	uint32_t flags;
	uint32_t mem_low;
	uint32_t mem_high;
	uint32_t __unused[8];
	uint32_t mmap_len;
	uint32_t mmap_base;
};

/* e820 항목 */
struct e820_entry {
	uint32_t size;
	uint32_t mem_lo;
	uint32_t mem_hi;
	uint32_t len_lo;
	uint32_t len_hi;
	uint32_t type;
};

/* ext_mem/base_mem의 범위 정보를 나타냅니다. */
struct area {
	uint64_t start;
	uint64_t end;
	uint64_t size;
};

#define BASE_MEM_THRESHOLD 0x100000
#define USABLE 1
#define ACPI_RECLAIMABLE 3
#define APPEND_HILO(hi, lo) (((uint64_t) ((hi)) << 32) + (lo))

/* e820 항목을 반복하고 basemem 및 extmem 범위를 구문 분석합니다. */
static void
resolve_area_info (struct area *base_mem, struct area *ext_mem) {
	struct multiboot_info *mb_info = ptov (MULTIBOOT_INFO);
	struct e820_entry *entries = ptov (mb_info->mmap_base);
	uint32_t i;

	for (i = 0; i < mb_info->mmap_len / sizeof (struct e820_entry); i++) {
		struct e820_entry *entry = &entries[i];
		if (entry->type == ACPI_RECLAIMABLE || entry->type == USABLE) {
			uint64_t start = APPEND_HILO (entry->mem_hi, entry->mem_lo);
			uint64_t size = APPEND_HILO (entry->len_hi, entry->len_lo);
			uint64_t end = start + size;
			printf("%llx ~ %llx %d\n", start, end, entry->type);

			struct area *area = start < BASE_MEM_THRESHOLD ? base_mem : ext_mem;

			// 이 영역에 속하는 첫 번째 항목입니다.
			if (area->size == 0) {
				*area = (struct area) {
					.start = start,
					.end = end,
					.size = size,
				};
			} else {  // 그렇지 않으면
				// 시작 연장
				if (area->start > start)
					area->start = start;
				// 끝을 확장
				if (area->end < end)
					area->end = end;
				// 크기 확장
				area->size += size;
			}
		}
	}
}

/*
 * 풀을 채웁니다.
 * 모든 페이지는 이 할당자에 의해 관리되며 코드 페이지도 포함됩니다.
 * 기본적으로 메모리의 절반은 커널에, 절반은 사용자에게 제공합니다.
 * 우리는 base_mem 부분을 가능한 한 커널에 푸시합니다.
 */
static void
populate_pools (struct area *base_mem, struct area *ext_mem) {
	extern char _end;
	void *free_start = pg_round_up (&_end);

	uint64_t total_pages = (base_mem->size + ext_mem->size) / PGSIZE;
	uint64_t user_pages = total_pages / 2 > user_page_limit ?
		user_page_limit : total_pages / 2;
	uint64_t kern_pages = total_pages - user_pages;

	// E820 맵을 구문 분석하여 각 풀에 대한 메모리 영역을 요청합니다.
	enum { KERN_START, KERN, USER_START, USER } state = KERN_START;
	uint64_t rem = kern_pages;
	uint64_t region_start = 0, end = 0, start, size, size_in_pg;

	struct multiboot_info *mb_info = ptov (MULTIBOOT_INFO);
	struct e820_entry *entries = ptov (mb_info->mmap_base);

	uint32_t i;
	for (i = 0; i < mb_info->mmap_len / sizeof (struct e820_entry); i++) {
		struct e820_entry *entry = &entries[i];
		if (entry->type == ACPI_RECLAIMABLE || entry->type == USABLE) {
			start = (uint64_t) ptov (APPEND_HILO (entry->mem_hi, entry->mem_lo));
			size = APPEND_HILO (entry->len_hi, entry->len_lo);
			end = start + size;
			size_in_pg = size / PGSIZE;

			if (state == KERN_START) {
				region_start = start;
				state = KERN;
			}

			switch (state) {
				case KERN:
					if (rem > size_in_pg) {
						rem -= size_in_pg;
						break;
					}
					// 커널 풀 생성
					init_pool (&kernel_pool,
							&free_start, region_start, start + rem * PGSIZE);
					// 다음 상태로 전환
					if (rem == size_in_pg) {
						rem = user_pages;
						state = USER_START;
					} else {
						region_start = start + rem * PGSIZE;
						rem = user_pages - size_in_pg + rem;
						state = USER;
					}
					break;
				case USER_START:
					region_start = start;
					state = USER;
					break;
				case USER:
					if (rem > size_in_pg) {
						rem -= size_in_pg;
						break;
					}
					ASSERT (rem == size);
					break;
				default:
					NOT_REACHED ();
			}
		}
	}

	// 사용자 풀 생성
	init_pool(&user_pool, &free_start, region_start, end);

	// e820_entry를 반복합니다. 사용할 수 있도록 설정합니다.
	uint64_t usable_bound = (uint64_t) free_start;
	struct pool *pool;
	void *pool_end;
	size_t page_idx, page_cnt;

	for (i = 0; i < mb_info->mmap_len / sizeof (struct e820_entry); i++) {
		struct e820_entry *entry = &entries[i];
		if (entry->type == ACPI_RECLAIMABLE || entry->type == USABLE) {
			uint64_t start = (uint64_t)
				ptov (APPEND_HILO (entry->mem_hi, entry->mem_lo));
			uint64_t size = APPEND_HILO (entry->len_hi, entry->len_lo);
			uint64_t end = start + size;

			// TODO: 0x1000 ~ 0x200000를 추가하세요. 지금은 문제가 되지 않습니다.
			// 모든 페이지를 사용할 수 없습니다.
			if (end < usable_bound)
				continue;

			start = (uint64_t)
				pg_round_up (start >= usable_bound ? start : usable_bound);
split:
			if (page_from_pool (&kernel_pool, (void *) start))
				pool = &kernel_pool;
			else if (page_from_pool (&user_pool, (void *) start))
				pool = &user_pool;
			else
				NOT_REACHED ();

			pool_end = pool->base + bitmap_size (pool->used_map) * PGSIZE;
			page_idx = pg_no (start) - pg_no (pool->base);
			if ((uint64_t) pool_end < end) {
				page_cnt = ((uint64_t) pool_end - start) / PGSIZE;
				bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
				start = (uint64_t) pool_end;
				goto split;
			} else {
				page_cnt = ((uint64_t) end - start) / PGSIZE;
				bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
			}
		}
	}
}

/* 페이지 할당자를 초기화하고 메모리 크기를 가져옵니다. */
uint64_t
palloc_init (void) {
  /* 링커가 기록한 커널의 끝입니다.
     kernel.lds.S를 참조하십시오. */
	extern char _end;
	struct area base_mem = { .size = 0 };
	struct area ext_mem = { .size = 0 };

	resolve_area_info (&base_mem, &ext_mem);
	printf ("Pintos booting with: \n");
	printf ("\tbase_mem: 0x%llx ~ 0x%llx (Usable: %'llu kB)\n",
		  base_mem.start, base_mem.end, base_mem.size / 1024);
	printf ("\text_mem: 0x%llx ~ 0x%llx (Usable: %'llu kB)\n",
		  ext_mem.start, ext_mem.end, ext_mem.size / 1024);
	populate_pools (&base_mem, &ext_mem);
	return ext_mem.end;
}

/* PAGE_CNT 연속된 사용 가능한 페이지 그룹을 획득하고 반환합니다.
   PAL_USER이 설정된 경우 사용자 풀에서 페이지를 가져옵니다.
   그렇지 않으면 커널 풀에서. PAL_ZERO이 FLAGS에 설정된 경우,
   그러면 페이지가 0으로 채워집니다. 페이지 수가 너무 적은 경우
   사용 가능하며 PAL_ASSERT이 설정되어 있지 않으면 널 포인터를 반환합니다.
   FLAGS, 이 경우 커널 패닉이 발생합니다. */
void *
palloc_get_multiple (enum palloc_flags flags, size_t page_cnt) {
	struct pool *pool = flags & PAL_USER ? &user_pool : &kernel_pool;

	lock_acquire (&pool->lock);
	size_t page_idx = bitmap_scan_and_flip (pool->used_map, 0, page_cnt, false);
	lock_release (&pool->lock);
	void *pages;

	if (page_idx != BITMAP_ERROR)
		pages = pool->base + PGSIZE * page_idx;
	else
		pages = NULL;

	if (pages) {
		if (flags & PAL_ZERO)
			memset (pages, 0, PGSIZE * page_cnt);
	} else {
		if (flags & PAL_ASSERT)
			PANIC ("palloc_get: out of pages");
	}

	return pages;
}

/* 단일 사용 가능한 페이지를 얻고 해당 커널 가상을 반환합니다.
   주소.
   PAL_USER이 설정된 경우 사용자 풀에서 페이지를 가져옵니다.
   그렇지 않으면 커널 풀에서. PAL_ZERO이 FLAGS에 설정된 경우,
   그러면 페이지가 0으로 채워집니다. 페이지가 없는 경우
   사용 가능하며 PAL_ASSERT이 설정되어 있지 않으면 널 포인터를 반환합니다.
   FLAGS, 이 경우 커널 패닉이 발생합니다. */
void *
palloc_get_page (enum palloc_flags flags) {
	return palloc_get_multiple (flags, 1);
}

/* PAGES부터 시작하는 PAGE_CNT 페이지를 해제합니다. */
void
palloc_free_multiple (void *pages, size_t page_cnt) {
	struct pool *pool;
	size_t page_idx;

	ASSERT (pg_ofs (pages) == 0);
	if (pages == NULL || page_cnt == 0)
		return;

	if (page_from_pool (&kernel_pool, pages))
		pool = &kernel_pool;
	else if (page_from_pool (&user_pool, pages))
		pool = &user_pool;
	else
		NOT_REACHED ();

	page_idx = pg_no (pages) - pg_no (pool->base);

#ifndef NDEBUG
	memset (pages, 0xcc, PGSIZE * page_cnt);
#endif
	ASSERT (bitmap_all (pool->used_map, page_idx, page_cnt));
	bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
}

/* PAGE에서 페이지를 해제합니다. */
void
palloc_free_page (void *page) {
	palloc_free_multiple (page, 1);
}

/* START에서 시작하고 END에서 끝나도록 풀 P를 초기화합니다. */
static void
init_pool (struct pool *p, void **bm_base, uint64_t start, uint64_t end) {
  /* 풀의 Used_map을 베이스에 놓을 것입니다.
     비트맵에 필요한 공간 계산
     수영장 크기에서 이를 뺍니다. */
	uint64_t pgcnt = (end - start) / PGSIZE;
	size_t bm_pages = DIV_ROUND_UP (bitmap_buf_size (pgcnt), PGSIZE) * PGSIZE;

	lock_init(&p->lock);
	p->used_map = bitmap_create_in_buf (pgcnt, *bm_base, bm_pages);
	p->base = (void *) start;

	// 모두 사용할 수 없음으로 표시하세요.
	bitmap_set_all(p->used_map, true);

	*bm_base += bm_pages;
}

/* PAGE이 POOL에서 할당된 경우 true를 반환합니다.
   그렇지 않으면 거짓입니다. */
static bool
page_from_pool (const struct pool *pool, void *page) {
	size_t page_no = pg_no (page);
	size_t start_page = pg_no (pool->base);
	size_t end_page = start_page + bitmap_size (pool->used_map);
	return page_no >= start_page && page_no < end_page;
}
