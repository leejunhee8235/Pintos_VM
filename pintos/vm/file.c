/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "filesys/file.h"
#include "threads/malloc.h"
#include "userprog/process.h"

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
	struct file_page *file_page = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page = &page->file;
	struct frame *frame = page->frame;

	// 1. dirty page이면 파일에 다시 기록
	if (frame != NULL && pml4_is_dirty(thread_current()->pml4, page->va)) {
		// 파일에 다시 기록 -> 근데 궁금한게 다 뿌시고 지울건데 바뀐거 기록 왜함 -> 메모리에서만 지우는거지 파일 변경은 보존해야하니까... 
		// mmap은 파일을 메모리에 붙여 놓고 쓰는 기능 

		file_write_at (file_page->file, frame->kva, file_page->read_bytes, file_page->offset); 
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
	
	// 파일.... close(fd) 하라는거 아니야? file 포인터가 가리키는 실제 file 객체를 닫아야 함 
	// remove로 파일 이름 제거도 해야되나요? 
}
static bool
lazy_load_file_page(struct page *page, void *aux)
{
	struct file_page *info = aux;
	file_seek(info->file, info->offset);
	off_t read_byte = file_read(info->file, page->frame->kva, info->read_bytes);

	memset(page->frame->kva + info->read_bytes, 0, info->zero_bytes);

	free(info);

	return true;
}
/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	void *start_addr = addr;
	struct thread *cur = thread_current();
	// 실패할 조건
	// printf("[do_mmap] addr=%p length=%zu writable=%d file=%p offset=%d\n",
	// 	   addr, length, writable, file, offset);
	off_t length_file = file_length(file);
	if (length_file == 0 || pg_round_down(addr) != addr || addr == 0 || length == 0 || offset < 0 || (offset % PGSIZE) != 0){
		return NULL;
	}
	/*
		fd로 열린 파일의 길이가 0 bytes이면 실패 size? 파일의 length를 따로 얻어올 수 있나?
		addr가 page aligned 아니면 실패
		addr이 0인 경우 실패
		length가 0인 경우 실패
		offset이 0보다 작은 경우
		offset이 PGSIZE의 배수가 아니면 실패
	*/
	// 매핑 하려는 페이지 범위를 어떻게 받아올까?
	// addr이 매핑하려는 공간의 시작 주소
	// length만큼 읽어야하니까 addr + length?

	int page_count = (length / PGSIZE);
	if((length % PGSIZE) != 0){
		page_count += 1;
	}
	//spt_find_page();
	// mmap(0x0004000, 6000, 1, fd, 0);
	// length + addr 이 가능한가?
	struct file_page *file_info = malloc(sizeof(struct file_page));

	while (page_count > 0)
	{
		//printf("[test]addr : %p", addr);
		if (spt_find_page(&cur->spt, addr) || addr > &cur->rsp){
			return NULL;
		}

		file_info->file = file;
		file_info->offset = offset;
		file_info->read_bytes = length >= PGSIZE ? PGSIZE : length;
		file_info->zero_bytes = PGSIZE - file_info->read_bytes;
		if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_file_page, file_info))
		{
			return NULL;
		}
		// 스택 영역의 가장 끝 주소를 어떻게 얻어올까?
		addr = pg_next(addr);
		length -= PGSIZE;
		page_count--;
	}
	// 매핑하려는 페이지 범위가 이미 존재하는 매핑된 페이지와 겹치는 경우 실패
	// 스택 영역과 겹치는 경우 실패
	
	// fd로 열린 파일에 offset에서 시작한 위치부터 length bytes 만큼을 vm에 올리는 것 -> 이 위치가 addr부터 시작 
	// 최종적으로 호출해야하는 함수? SPT에 올리는 것이므로 vm_alloc_page_with_initializer 을 호출해야 할 것
	return start_addr;
};

/* Do the munmap */
void
do_munmap (void *addr) {
	// addr에 해당하는 mapping을 해제한다.
	// addr로 시작하는 mapping 정보를 찾아야한다. 어떻게?
	struct thread *cur = thread_current();
	struct page *page = spt_find_page(&cur->spt, addr);
	if (page == NULL){
		return;
	}
	if(VM_TYPE(page->operations->type) != VM_FILE){
		return;
	}
	// 매핑된 정보가 있다.
	// 해당 file에 있는 모든 매핑된 page들을 해제해야하니까 반복문을 돌면서 해제를 해줘야한다.
	while (page != NULL && VM_TYPE(page->operations->type) == VM_FILE){
		spt_remove_page(&cur->spt, page);
		file_backed_destroy(page);
		addr = pg_next(addr);
		page = spt_find_page(&cur->spt, addr);
	}
}