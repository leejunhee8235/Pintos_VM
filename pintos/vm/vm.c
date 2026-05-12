/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/mmu.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/*
		주어진 supplemental page table에서 va에 해당하는 struct page를 찾는다.
		찾지 못하면 NULL을 return한다.

		hash_find() 함수를 사용할 것 같은데
		인자로는 (struct hash *h, struct hash_elem *e) 다음과 같이 들어간다.
	*/
	struct page p;
	struct hash_elem *e;

	p.va = pg_round_down(va);
	/*
		&spt->pages 에서, 
		즉 해당 프로세스가 가지고 있는 spt에서 
		page의 elem과 같은 elem을 찾는다.
	
		AI 답변
		=> page의 elem과 같은 elem을 찾는다기보다, hash_find()가 내부적으로
		p와 같은 기준의 page를 찾는것이다.
		p와 같은 기준? => p -> va
		hash_elem을 통해서, 같은키(va)를 가진 page를 찾는다.
	*/
	e = hash_find(&spt->pages, &p.hash_elem);
	// page에 hash_elem을 사용하는 이유?

	return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/*
		주어진 SPT에 인자로 주어진 page를 insert 한다.
		va가 이미 spt안에 존재하는지 여부를 확인해야 한다.
	*/
	/*
		hash_insert() 함수 사용
		: hash에서 element와 같은 element를 찾습니다.
		같은 element가 없으면 element를 hash table에 insert하고 null pointer를 반환합니다.
		이미 같은 element가 있으면 hash table을 수정하지 않고 기존 element를 반환합니다.

		hash_insert() 내부에 중복 elem 체크를 하는건가?
	*/
	int succ = false;
	struct hash_elem *e = hash_insert(&spt->pages, &page->hash_elem);
	/*
		hash_insert()의 반환값이 null 이면 insert에 성공했다 라는 의미
		null 이 아니면 기존의 elem을 반환함 => insert에 실패함
	*/
	if(e == NULL){
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.

*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	/* frame 메타데이터를 먼저 만들고, 실제 내용을 담을 4KB user pool
	 * 페이지를 frame->kva에 연결한다. */
	frame = malloc(sizeof *frame);
	if (frame == NULL) {
		return NULL;
	}
	frame->kva = palloc_get_page(PAL_USER);
	if (frame->kva == NULL) {
		free(frame);
		// vm_evict_frame();
		PANIC("todo:eviction");
		return NULL;
	}
	
	frame->page = NULL;
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va) {
	struct page *page = NULL;
	/* TODO: Fill this function */

	/* V1의 SPT 조회를 사용한다. va는 page 중간 주소일 수 있으므로
	 * spt_find_page()가 page 시작 주소로 내려서 등록된 page를 찾아야 한다. */
	page = spt_find_page(&thread_current ()-> spt, va);
	if (page == NULL) {
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	struct thread *t = thread_current();
	
	if (frame == NULL)
		return false;

	/* Set links TODO: Fill this function */
	/* 이 연결은 VM 내부 관리용이다. user code가 page->va로 접근하려면
	 * 아래에서 pml4 매핑까지 등록해야 한다. */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* TODO: V1에서 struct page에 writable을 저장하면 page->writable로
	 * 교체해야 한다. */
	if (!pml4_set_page(t->pml4, page->va, frame->kva, page->writable)) {
		page->frame = NULL;
		frame->page = NULL;
		palloc_free_page(frame->kva);
		free(frame);
		return false;
	}

	/* swap_in은 "이 frame에 page 내용을 채우는" 공통 hook이다.
	 * uninit은 initializer 실행, anon은 swap/zero-fill, file은 파일 읽기를 한다. */
	if (!swap_in (page, frame->kva)) {
		pml4_clear_page(t->pml4, page->va);
		page->frame = NULL;
		frame->page = NULL;
		palloc_free_page(frame->kva);
		free(frame);
		return false;
	}

	return true;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	/*
		SPT를 initialize 하는 함수.
		해당 SPT는 hash 자료구조를 사용하기로 설정해놓았음.
		그러면 hash table을 init 해야한다.
	*/
	hash_init(&spt->pages, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	/*
		src의 supplemental page table을 dst로 copy합니다.
		child가 parent의 execution context를 상속해야 할 때, 즉 fork()에서 사용됩니다.
		src의 supplemental page table에 있는 각 page를 순회하면서,
		그 entry의 정확한 copy를 dst의 supplemental page table에 만들어야 합니다.
		uninit page를 allocate하고, 즉시 claim해야 합니다.
	*/
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

/*
	gitbook hash 페이지 참조
*/
uint64_t page_hash(const struct hash_elem *p_, void *aux UNUSED){
	const struct page *p = hash_entry(p_, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

bool page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
	const struct page *a = hash_entry(a_, struct page, hash_elem);
	const struct page *b = hash_entry(b_, struct page, hash_elem);

	return a->va < b->va;
}
