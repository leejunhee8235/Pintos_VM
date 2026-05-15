#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page {

	// 변경 사항이 있는가? -> 굳이 따로 저장하지 않아도 됨. 실제 변경 여부는 PML4의 dirty bit로 확인할 수 있음 -> pml4_is_dirty(thread_current()->pml4, page->va) 로 그때그때 확인 
	// 원래 backing file 자체가 저장소 역할을 함 -> 이 페이지를 어느 파일의 어느 위치에서 몇 바이트 읽고, 나머지는 몇 바이트 0으로 채울지 정보가 필요 -> aux 같은 것... (메모리 위치가 아니라 디스크/파일 시스템 쪽 위치 정보) 

	struct file *file;
    off_t ofs;
    size_t read_bytes;
    size_t zero_bytes;
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
#endif
