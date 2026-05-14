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
/*
	lazy loading 을 위한 page를 만들고, SPT에 등록하는 함수
*/
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {
	/*
		인자에 어떤 값들이 들어가는지 먼저 체크한다.
		type : uninit page가 초기화 될 때 어떤 type 인지 정하는 인자(anon, file)
		upage : 유저의 가상주소 -> 어느 가상 주소 영역에 이 페이지가 할당 될 것인지
		writable : 해당 page가 읽기권한인지 쓰기권한인지
		init : 어떤 type으로 initializer 해줄 건지?? -> 잘모르겠음
		AI 답변 -> page fault가 일어났을 때, 실제 내용을 채우는 함수이다(lazy_load_segment)
		aux : 페이지가 초기화 될 때, 들고 있는 메타 데이터(알아야 하는) 
		ex) read_byte(몇 바이트 읽어야 하는지), zero_byte(얼마만큼 0 패딩을 해야하는지), file(어떤 파일인지), offset(어디부터 읽어야 하는지)...
	*/
	ASSERT(VM_TYPE(type) != VM_UNINIT);
	struct supplemental_page_table *spt = &thread_current ()->spt;
	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		struct page *page = malloc(sizeof(struct page));
		// page malloc 에 실패했을 때
		if(page == NULL){
			return false;
		}
		// VM type에 맞는 init? 
		switch(VM_TYPE(type)){
			case VM_ANON:
				// 여기서 uninit page 구조체를 생성한 다음, 그 구조체 안의 initializer에 anon or file 을 넣어준다
				// uninit page를 만들어 주는 것이기 때문에 type에는 VM_UNINIT 를 넣어줘야한다?
				// type 에는 uninit page가 변해야 할 type을 적어놔야 한다.
				uninit_new(page, upage, init, type, aux, anon_initializer);
				break;
			case VM_FILE:
				uninit_new(page, upage, init, type, aux, file_backed_initializer);
				break;
			default:
				free(page);
				return false;
		}
		/*
			TODO: page를 생성하고, VM type에 맞는 initializer를 가져온다. -> 이걸 어떻게 가져오는데
			TODO: 그 다음 uninit_new를 호출해서 "uninit" page 구조체를 만든다.
			TODO: uninit_new를 호출한 뒤에 필요한 field를 수정해야 한다. -> 필요한 field?
		*/
		page->writable = writable;
		/* TODO: Insert the page into the spt. */
		// spt_insert 값이 null 이면 성공적으로 insert가 되었다는 뜻(true)
		// 그래서 null 이 아니면(false)면 할당한 페이지를 free
		if(!spt_insert_page(spt, page)){
			free(page);
			return false;
		}
		return true;
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
/*
	실제로 stack의 크기를 늘리는 함수.
	pintos에서는 1MB로 제한한다.
*/
static void
vm_stack_growth (void *addr) {
	/*
		익명페이지를 하나 이상 할당하여 스택 크기를 늘린다?
	*/
	bool growth_success = false;
	// void *stack_bottom = (void *)(((uint8_t *)addr) - PGSIZE);
	// printf("[여기까지 와요?]\n");
	growth_success = vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), true);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f, void *addr,
		bool user UNUSED, bool write, bool not_present UNUSED) {
	//printf("[handler 들어왔나요?]\n");

	if(is_kernel_vaddr(addr)){
		//printf("[혹시 여기에요?]\n");
		return false;
	}

	struct thread *curr = thread_current();
	struct supplemental_page_table *spt = &curr->spt;

	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	/*
		write 권한 위반일 경우에는 process_exit()
	*/
	struct page *page = spt_find_page(spt, addr);
	// printf("[여기에요?]\n");
	/*
		권한 위반이 아닐경우에는 pml4에 매핑이 안되있는 경우
		근데 여기서 또 분기가 나뉜다.
		spt_find_page() 를 호출하고 만약 spt에도 없으면 stack_growth 검사를 해야 함
		근데 spt에 있으면 vm_do_claim_page로 pml4에 매핑 시켜 주면 된다.
	*/
	if(page == NULL){
		/*
			이 경우 spt에도 page가 없는 상황이다.
			그럼 stack 검사를 해야한다.
			stack growth 조건을 어떻게 검사할까
			=> gitbook에는 fault가 난 접근이(fault_addr?)이 실제로 stack 접근처럼 보이는 경우에만
			   stack을 늘려야 한다.
			   이를 위해 구현자는 stack 접근과 일반적인 잘못된 memory 접근을 구분 할 수 있는 판단기준을 만들어야 함.
		*/
		/*
			디버그로 확인해본 결과, stack_growth 검사에서 스택을 늘려야 하는 범위는 스택 포인터 - 8 까지 인거같음.
			따라서 addr이 rsp - 8 ~ rsp 사이에 있을 때 stack_growth 를 수행 하면 되지 않을까?
		*/
		//printf("[spt에 page가 없나요?]\n");
		if (addr >= f->rsp - STACK_RANGE)
		{
			// stack을 늘려야 할 경우에는 vm_sg 함수 호출
			//printf("[스택을 늘려야 합니까?]\n");
			vm_stack_growth(addr);
			// printf("[addr] : %p\n", addr);
			// printf("[rsp의 위치] : %p\n", f->rsp);
			return true;
		}
		// printf("[rsp - 8 의 위치] : %p\n", f->rsp - 8);
		// printf("[addr] : %p\n", addr);
		// printf("[rsp의 위치] : %p\n", f->rsp);

		curr->exit_status = -1;
		thread_exit ();
	}

	if (write == true && page->writable == false){
		curr->exit_status = -1;
		thread_exit();
	}
	/*
		이제 spt에 page가 존재하는 경우이다.
		흐름도에 따르면 vm_do_claim_page를 호출한다.
	*/
	//handler 조건 통과했나요]\n");
	return vm_do_claim_page(page);
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
