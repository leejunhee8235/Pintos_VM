#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
struct page;
enum vm_type;

// 익명 페이지를 설명하는 구조체
struct anon_page {
    size_t swap_slot; // swap 위치 
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif