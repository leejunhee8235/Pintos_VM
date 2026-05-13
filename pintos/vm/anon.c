/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "lib/kernel/bitmap.h"
#include <string.h>
#include "threads/vaddr.h"
#include "threads/synch.h"


#define SECTORS_PER_PAGE (PGSIZE / DISK_SECTOR_SIZE)
static struct lock swap_lock;
static struct bitmap *swap_table;

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get (1, 1); // Pintos에서는 스왑 디스크를 채널 1, 디바이스 1로 지정하고 있음 
	ASSERT (swap_disk != NULL);

	size_t swap_size = disk_size (swap_disk) / SECTORS_PER_PAGE; // 스왑 디스크의 크기를 페이지 단위로 계산 
	swap_table = bitmap_create (swap_size); // swap 영역에서 어떤 페이지가 사용 중인지 여부를 추적하는 데 사용됨 
	ASSERT (swap_table != NULL);

	lock_init (&swap_lock);
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {

	ASSERT (VM_TYPE (type) == VM_ANON);
	/* Set up the handler */

	page->operations = &anon_ops; 	// 이 페이지의 operation table을 익명 페이지용 handler 묶음으로 바꾼다.

	struct anon_page *anon_page = &page->anon;

	anon_page->swap_slot = BITMAP_ERROR;   // 아직 swap disk에 저장된 위치가 없음
	memset (kva, 0, PGSIZE); // kva가 가리키는 4KB 물리 프레임 전체를 0으로 채움 

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	// swap disk 에 나가 있던 anonymous page를 다시 메모리로 가져옴
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;

	// 메모리의 내용을 디스크로 복사하여 익명 페이지(anonymous page)를 스왑 디스크(swap disk)로 스왑 아웃(swap out)합니다. 
	// 먼저 스왑 테이블(swap table)을 사용해 디스크에서 비어 있는 스왑 슬롯(swap slot)을 찾은 다음, 
	// 데이터 페이지를 해당 슬롯에 복사합니다. 데이터의 위치는 page 구조체에 저장되어야 합니다. 
	// 디스크에 더 이상 비어 있는 슬롯이 없으면 커널 패닉(kernel panic)을 발생시킬 수 있습니다. -> swap table이 모두 true면 kernel panic 발생 

}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;

	if (anon_page->swap_slot != BITMAP_ERROR) {

		// swap out 되어있으면 swap bitmap에서 해당 slot reset 
		lock_acquire (&swap_lock);
        bitmap_reset (swap_table, anon_page->swap_slot); 
        lock_release (&swap_lock);

		// 이 페이지가 더 이상 swap slot을 가지고 있지 않다는 표시 
		anon_page->swap_slot = BITMAP_ERROR;
	}
}
