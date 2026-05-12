/* uninit.c: Implementation of uninitialized page.
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,
	.swap_out = NULL,
	.destroy = uninit_destroy,
	.type = VM_UNINIT,
};

/* DO NOT MODIFY this function */
/*
	uninit_new() 를 호출하면 여기서 page를 초기화 시켜준다.
	union 의 3가지 타입에서 uninit page로 설정한다.

	page라는 struct page를 va 주소에 해당하는 UNINIT page로 만들어라.
	그리고 나중에 page fault가 나서 이 page를 실제로 초기화할 때는
	type에 맞게 initializer를 먼저 실행하고,
	그 다음 init(page, aux)를 실행할 수 있도록
	init과 aux도 저장해둬라.
*/
void
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {

	/*
		page : 비어있는 page 객체, 아래에서 page 안을 uninit 으로 채워넣는다.
		va : 이 페이지의 가상 주소
		init : page fault 가 발생했을 때, 실제 내용을 채우는 함수(lazy_load_segment)
		type : uninit page가 어떤 page 타입으로 바뀔지
		aux : 나중에 lazy_load_segment가 실행될 때
			  어느 파일의 어느 위치에서 몇 바이트를 읽어야 하는지 알려주는 정보
		initializer : page 자체를 ANON/FILE 타입으로 바꾸고 operations를 설정하는 함수
	*/
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,
		}
	};
}

/* Initalize the page on first fault */
static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit;

	/* Fetch first, page_initialize may overwrite the values */
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. */
	return uninit->page_initializer (page, uninit->type, kva) &&
		(init ? init (page, aux) : true);
}

	/* uninit_page가 가지고 있는 resource를 해제한다.

	대부분의 page는 실행 중에 다른 page object로 변환되지만,
	프로세스가 종료될 때까지 uninit page로 남아 있는 page가 있을 수 있다.
	이런 page들은 실행 중 한 번도 참조되지 않은 page들이다.

	PAGE 자체는 caller가 해제할 것이다. 
	
	이 말이 spt에 page로 insert 됬지만 
	프로그램 실행중에 한번도 호출되지 않은 page들을
	삭제 하는 함수 인거 같음.*/
static void
uninit_destroy (struct page *page) {
	struct uninit_page *uninit = &page->uninit;
	/* page struct 가 보유하던 resource를 해제한다. */
	if(VM_TYPE(page->uninit.type) == VM_ANON){
		vm_dealloc_page(page);
	}
}
