/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "filesys/file.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	struct frame *frame = page->frame;

	// 1. dirty page이면 파일에 다시 기록
	if (frame != NULL && pml4_is_dirty(thread_current()->pml4, page->va)) {
		// 파일에 다시 기록 -> 근데 궁금한게 다 뿌시고 지울건데 바뀐거 기록 왜함 -> 메모리에서만 지우는거지 파일 변경은 보존해야하니까... 
		// mmap은 파일을 메모리에 붙여 놓고 쓰는 기능 

		file_write_at (file_page->file, frame->kva, file_page->read_bytes, file_page->ofs); 
	}

	// 2. frame 해제
	if (frame != NULL) {

		if (thread_current()->pml4 != NULL) {
			pml4_clear_page(thread_current()->pml4, page->va);
		}

		if (frame->kva != NULL) {
			palloc_free_page(frame->kva);
		}

		frame->page = NULL;
		page->frame = NULL;
		free(frame);
	}

	// 3. file-backed 정보 정리

	// struct file *file;
    // off_t ofs;
    // size_t read_bytes;
    // size_t zero_bytes;
	// file_page가 struct page 안의 union 멤버라서, 마지막에 vm_dealloc_page()가 struct page 자체를 free(page) 해버리고 안에 있던 위 정보들은 같이 사라지므로 NULL, 0으로 만들 필요 없음 
	
	파일.... close(fd) 하라는거 아니야? file 포인터가 가리키는 실제 file 객체를 닫아야 함 
	remove로 파일 이름 제거도 해야되나요? 
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {

	// 실패할 조건
	off_t length_file = file_length (file);
	if (length_file == 0 || pg_round_down(addr) != addr || addr == 0 || length == 0 || offset <= 0)
		return NULL;
	// fd로 열린 파일의 길이가 0 bytes이면 실패 size? 파일의 length를 따로 얻어올 수 있나? 
	// addr가 page aligned 아니면 실패 
	// addr이 0인 경우 실패
	// length가 0인 경우 실패
	// offset이 0보다 작은 경우 

	// 매핑하려는 페이지 범위가 이미 존재하는 매핑된 페이지와 겹치는 경우 실패 
	// 스택 영역과 겹치는 경우 실패

	fd로 열린 파일에 offset에서 시작한 위치부터 length bytes 만큼을 vm에 올리는 것 -> 이 위치가 addr부터 시작 
	최종적으로 호출해야하는 함수? SPT에 올리는 것이므로 vm_alloc_page_with_initializer 을 호출해야 할 것 
	
	
	
}

/* Do the munmap */
void
do_munmap (void *addr) {
}