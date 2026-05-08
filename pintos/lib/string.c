#include <string.h>
#include <debug.h>

/* SIZE 바이트를 SRC에서 DST로 복사합니다. 이는 겹치지 않아야 합니다.
   DST을 반환합니다. */
void *
memcpy (void *dst_, const void *src_, size_t size) {
	unsigned char *dst = dst_;
	const unsigned char *src = src_;

	ASSERT (dst != NULL || size == 0);
	ASSERT (src != NULL || size == 0);

	while (size-- > 0)
		*dst++ = *src++;

	return dst_;
}

/* SIZE 바이트를 SRC에서 DST로 복사합니다.
   중복. DST을 반환합니다. */
void *
memmove (void *dst_, const void *src_, size_t size) {
	unsigned char *dst = dst_;
	const unsigned char *src = src_;

	ASSERT (dst != NULL || size == 0);
	ASSERT (src != NULL || size == 0);

	if (dst < src) {
		while (size-- > 0)
			*dst++ = *src++;
	} else {
		dst += size;
		src += size;
		while (size-- > 0)
			*--dst = *--src;
	}

	return dst;
}

/* SIZE 바이트의 두 블록에서 첫 번째 다른 바이트를 찾습니다.
   A와 B에서. A의 바이트가 다음과 같은 경우 양수 값을 반환합니다.
   더 크거나 B의 바이트가 더 크면 음수 값이거나 0입니다.
   블록 A와 B가 동일한 경우. */
int
memcmp (const void *a_, const void *b_, size_t size) {
	const unsigned char *a = a_;
	const unsigned char *b = b_;

	ASSERT (a != NULL || size == 0);
	ASSERT (b != NULL || size == 0);

	for (; size-- > 0; a++, b++)
		if (*a != *b)
			return *a > *b ? +1 : -1;
	return 0;
}

/* 문자열 A와 B에서 처음으로 다른 문자를 찾습니다.
   A의 문자가 부호 없는 문자인 경우 양수 값을 반환합니다.
   char)가 더 크면 음수 값입니다. B의 문자(예:
   부호 없는 문자)는 더 크거나 문자열 A와 B가 다음과 같은 경우 0입니다.
   동일한. */
int
strcmp (const char *a_, const char *b_) {
	const unsigned char *a = (const unsigned char *) a_;
	const unsigned char *b = (const unsigned char *) b_;

	ASSERT (a != NULL);
	ASSERT (b != NULL);

	while (*a != '\0' && *a == *b) {
		a++;
		b++;
	}

	return *a < *b ? -1 : *a > *b;
}

/* 첫 번째 CH의 첫 번째 발생에 대한 포인터를 반환합니다.
   BLOCK에서 시작하는 SIZE바이트. CH인 경우 널 포인터를 반환합니다.
   BLOCK에서는 발생하지 않습니다. */
void *
memchr (const void *block_, int ch_, size_t size) {
	const unsigned char *block = block_;
	unsigned char ch = ch_;

	ASSERT (block != NULL || size == 0);

	for (; size-- > 0; block++)
		if (*block == ch)
			return (void *) block;

	return NULL;
}

/* STRING에서 처음으로 나타나는 C를 찾아서 반환합니다.
   C가 STRING에 나타나지 않으면 널 포인터입니다. C == '\0'인 경우
   그런 다음 끝에 있는 null 종결자에 대한 포인터를 반환합니다.
   STRING. */
char *
strchr (const char *string, int c_) {
	char c = c_;

	ASSERT (string);

	for (;;)
		if (*string == c)
			return (char *) string;
		else if (*string == '\0')
			return NULL;
		else
			string++;
}

/* STRING의 초기 부분 문자열 길이를 반환합니다.
   STOP에 없는 문자로 구성됩니다. */
size_t
strcspn (const char *string, const char *stop) {
	size_t length;

	for (length = 0; string[length] != '\0'; length++)
		if (strchr (stop, string[length]) != NULL)
			break;
	return length;
}

/* STRING의 첫 번째 문자에 대한 포인터를 반환합니다.
   STOP에도 있습니다. STRING의 문자가 STOP에 없으면 다음을 반환합니다.
   널 포인터. */
char *
strpbrk (const char *string, const char *stop) {
	for (; *string != '\0'; string++)
		if (strchr (stop, *string) != NULL)
			return (char *) string;
	return NULL;
}

/* STRING에서 마지막으로 나타나는 C에 대한 포인터를 반환합니다.
   STRING에서 C가 발생하지 않으면 널 포인터를 반환합니다. */
char *
strrchr (const char *string, int c_) {
	char c = c_;
	const char *p = NULL;

	for (; *string != '\0'; string++)
		if (*string == c)
			p = string;
	return (char *) p;
}

/* STRING의 초기 부분 문자열 길이를 반환합니다.
   SKIP의 문자로 구성됩니다. */
size_t
strspn (const char *string, const char *skip) {
	size_t length;

	for (length = 0; string[length] != '\0'; length++)
		if (strchr (skip, string[length]) == NULL)
			break;
	return length;
}

/* 내에서 처음으로 나타나는 NEEDLE에 대한 포인터를 반환합니다.
   HAYSTACK. NEEDLE이 존재하지 않으면 널 포인터를 반환합니다.
   HAYSTACK 내. */
char *
strstr (const char *haystack, const char *needle) {
	size_t haystack_len = strlen (haystack);
	size_t needle_len = strlen (needle);

	if (haystack_len >= needle_len) {
		size_t i;

		for (i = 0; i <= haystack_len - needle_len; i++)
			if (!memcmp (haystack + i, needle, needle_len))
				return (char *) haystack + i;
	}

	return NULL;
}

/* 문자열을 DELIMITERS으로 구분된 토큰으로 나눕니다. 그만큼
   이 함수가 처음 호출될 때 S는 호출할 문자열이어야 합니다.
   토큰화하고 후속 호출에서는 널 포인터여야 합니다.
   SAVE_PTR은(는) 유지하는 데 사용되는 'char *' 변수의 주소입니다.
   토크나이저의 위치를 ​​추적합니다. 매번 반환 값
   문자열의 다음 토큰이거나, 없으면 널 포인터입니다.
   토큰이 남습니다.

   이 함수는 여러 개의 인접한 구분 기호를 단일로 처리합니다.
   구분 기호. 반환된 토큰은 길이가 0이 될 수 없습니다.
   DELIMITERS은(는) 한 호출에서 다음 호출로 변경될 수 있습니다.
   단일 문자열.

   strtok_r()은 문자열 S를 수정하여 구분 기호를 null로 변경합니다.
   바이트. 따라서 S는 수정 가능한 문자열이어야 합니다. 문자열 리터럴,
   특히 C에서는 수정할 수 *없습니다*.
   이전 버전과의 호환성은 `const'가 아닙니다.

   사용 예:

   char s[] = "토큰화할 문자열. ";
   char *토큰, *save_ptr;

   for (token = strtok_r (s, " ", &save_ptr); 토큰 != NULL;
   토큰 = strtok_r (NULL, " ", &save_ptr))
   printf("'%s'\n", 토큰);

출력:

'끈'
'에게'
'토큰화.'
*/
char *
strtok_r (char *s, const char *delimiters, char **save_ptr) {
	char *token;

	ASSERT (delimiters != NULL);
	ASSERT (save_ptr != NULL);

	/* S가 Null이 아닌 경우 S에서 시작합니다.
	   S가 null이면 저장된 위치부터 시작합니다. */
	if (s == NULL)
		s = *save_ptr;
	ASSERT (s != NULL);

	/* 현재 위치에서 DELIMITERS을(를) 건너뜁니다. */
	while (strchr (delimiters, *s) != NULL) {
		/* 검색하는 경우 strchr()은 항상 null이 아닌 값을 반환합니다.
		   널 바이트의 경우 모든 문자열에 널이 포함되어 있기 때문입니다.
		   바이트(끝). */
		if (*s == '\0') {
			*save_ptr = s;
			return NULL;
		}

		s++;
	}

	/* 문자열 끝까지 DELIMITERS이 아닌 항목을 건너뜁니다. */
	token = s;
	while (strchr (delimiters, *s) == NULL)
		s++;
	if (*s != '\0') {
		*s = '\0';
		*save_ptr = s + 1;
	} else
		*save_ptr = s;
	return token;
}

/* DST의 SIZE 바이트를 VALUE로 설정합니다. */
void *
memset (void *dst_, int value, size_t size) {
	unsigned char *dst = dst_;

	ASSERT (dst != NULL || size == 0);

	while (size-- > 0)
		*dst++ = value;

	return dst_;
}

/* STRING의 길이를 반환합니다. */
size_t
strlen (const char *string) {
	const char *p;

	ASSERT (string);

	for (p = string; *p != '\0'; p++)
		continue;
	return p - string;
}

/* STRING의 길이가 MAXLEN 문자보다 작으면 다음을 반환합니다.
   실제 길이입니다. 그렇지 않으면 MAXLEN 을 반환합니다. */
size_t
strnlen (const char *string, size_t maxlen) {
	size_t length;

	for (length = 0; string[length] != '\0' && length < maxlen; length++)
		continue;
	return length;
}

/* 문자열 SRC을 DST에 복사합니다. SRC이 SIZE보다 긴 경우 - 1
   문자, SIZE - 1자만 복사됩니다. 널
   종결자는 SIZE이 0이 아닌 한 항상 DST에 기록됩니다.
   null 종결자를 포함하지 않고 SRC의 길이를 반환합니다.

   strlcpy()은(는) 표준 C 라이브러리에는 없지만
   점점 인기를 얻고 있는 확장 프로그램입니다. 보다
http://www.courtesan.com/todd/papers/strlcpy.html
strlcpy()에 대한 정보. */
size_t
strlcpy (char *dst, const char *src, size_t size) {
	size_t src_len;

	ASSERT (dst != NULL);
	ASSERT (src != NULL);

	src_len = strlen (src);
	if (size > 0) {
		size_t dst_len = size - 1;
		if (src_len < dst_len)
			dst_len = src_len;
		memcpy (dst, src, dst_len);
		dst[dst_len] = '\0';
	}
	return src_len;
}

/* 문자열 SRC을 DST에 연결합니다. 연결된 문자열은
   SIZE - 1자로 제한됩니다. 널 종결자는 항상
   SIZE이 0이 아닌 한 DST에 기록됩니다.
   연결된 문자열은 다음과 같이 가정했을 것입니다.
   널 종결자를 포함하지 않는 충분한 공간.

   strlcat()은(는) 표준 C 라이브러리에는 없지만
   점점 인기를 얻고 있는 확장 프로그램입니다. 보다
http://www.courtesan.com/todd/papers/strlcpy.html
strlcpy()에 대한 정보. */
size_t
strlcat (char *dst, const char *src, size_t size) {
	size_t src_len, dst_len;

	ASSERT (dst != NULL);
	ASSERT (src != NULL);

	src_len = strlen (src);
	dst_len = strlen (dst);
	if (size > 0 && dst_len < size) {
		size_t copy_cnt = size - dst_len - 1;
		if (src_len < copy_cnt)
			copy_cnt = src_len;
		memcpy (dst + dst_len, src, copy_cnt);
		dst[dst_len + copy_cnt] = '\0';
	}
	return src_len + dst_len;
}

