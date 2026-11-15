/* STRINGS.AOU - R11A guest test: string span/token, stdlib search and sort,
 * strtol boundaries, and the bounded snprintf. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static int passes = 0, fails = 0;

static void check(const char *name, int ok)
{
	printf("CHECK %s: %s\n", name, ok ? "ok" : "FAIL");
	if (ok) passes++; else fails++;
}

static int cmp_int(const void *a, const void *b)
{
	int x = *(const int *) a, y = *(const int *) b;
	return (x > y) - (x < y);
}

static int cmp_str(const void *a, const void *b)
{
	return strcmp(*(char *const *) a, *(char *const *) b);
}

#define N 200
static int data[N];

int main(int argc, char **argv)
{
	char buf[64];
	char text[40];
	char *tok;
	int ntok = 0;
	int i;
	long sum0 = 0, sum1 = 0;
	unsigned long seed = 12345UL;
	char *words[6];
	char *found;
	int key;

	(void) argc;
	(void) argv;

	/* strspn / strcspn / strpbrk */
	check("strspn", strspn("aabcXX", "ab") == 3);
	check("strspn empty", strspn("", "a") == 0);
	check("strspn none", strspn("xyz", "ab") == 0);
	check("strcspn", strcspn("hello world", " ") == 5);
	check("strcspn none", strcspn("abc", "xyz") == 3);
	check("strpbrk", strpbrk("abcdef", "dx") != NULL && strcmp(strpbrk("abcdef", "dx"), "def") == 0);
	check("strpbrk miss", strpbrk("abc", "xyz") == NULL);

	/* strtok skips leading and repeated delimiters; empty tokens never appear. */
	strcpy(text, "  one,two,,three  ");
	tok = strtok(text, ", ");
	while (tok != NULL) {
		ntok++;
		if (ntok == 1) check("tok one", strcmp(tok, "one") == 0);
		if (ntok == 2) check("tok two", strcmp(tok, "two") == 0);
		if (ntok == 3) check("tok three", strcmp(tok, "three") == 0);
		tok = strtok(NULL, ", ");
	}
	check("strtok count 3", ntok == 3);
	strcpy(text, "");
	check("strtok empty input", strtok(text, ",") == NULL);

	/* strtol / strtoul boundaries */
	check("strtol neg", strtol("  -17", NULL, 10) == -17L);
	check("strtol hex base0", strtol("0x1F", NULL, 0) == 31L);
	check("strtol octal base0", strtol("077", NULL, 0) == 63L);
	check("strtol base36", strtol("zz", NULL, 36) == 1295L);
	check("strtol stops at non-digit", strtol("12abc", NULL, 10) == 12L);
	{
		char *end = NULL;
		strtol("12abc", &end, 10);
		check("strtol end pointer", end != NULL && *end == 'a');
		{
			char src[4];
			char *e2 = NULL;
			strcpy(src, "xyz");
			check("strtol no digits value", strtol(src, &e2, 10) == 0L);
			check("strtol no digits end = start", e2 == src);
		}
	}
	check("strtol overflow clamps", strtol("99999999999999999999", NULL, 10) == LONG_MAX);
	check("strtol underflow clamps", strtol("-99999999999999999999", NULL, 10) == LONG_MIN);
	check("strtol bad base 1", strtol("12", NULL, 1) == 0L);
	check("strtoul max", strtoul("4294967295", NULL, 10) == ULONG_MAX);

	/* qsort and bsearch on 200 deterministic pseudo-random ints */
	for (i = 0; i < N; i++) {
		seed = seed * 1103515245UL + 12345UL;
		data[i] = (int) ((seed >> 8) % 1000);
		sum0 += data[i];
	}
	qsort(data, N, sizeof(int), cmp_int);
	for (i = 1; i < N; i++)
		if (data[i - 1] > data[i])
			break;
	check("qsort ordered", i == N);
	for (i = 0; i < N; i++)
		sum1 += data[i];
	check("qsort is a permutation", sum0 == sum1);
	key = data[57];
	found = (char *) bsearch(&key, data, N, sizeof(int), cmp_int);
	check("bsearch hit", found != NULL && *(int *) found == key);
	key = 1000;
	check("bsearch miss", bsearch(&key, data, N, sizeof(int), cmp_int) == NULL);
	qsort(data, 0, sizeof(int), cmp_int);
	key = data[0];
	qsort(data, 1, sizeof(int), cmp_int);
	check("qsort n=0,1 no-op", data[0] == key);

	words[0] = "pear";
	words[1] = "apple";
	words[2] = "fig";
	words[3] = "banana";
	words[4] = "cherry";
	words[5] = "date";
	qsort(words, 6, sizeof(char *), cmp_str);
	check("qsort strings", strcmp(words[0], "apple") == 0 && strcmp(words[5], "pear") == 0);
	check("bsearch strings",
	      bsearch(&words[3], words, 6, sizeof(char *), cmp_str) != NULL);

	/* bounded snprintf (C99 extension) and sprintf return values */
	memset(buf, 'Z', sizeof buf);
	check("snprintf truncates", snprintf(buf, 8, "%d", 123456789) == 9);
	check("snprintf content", strncmp(buf, "1234567", 7) == 0 && buf[7] == '\0');
	memset(buf, 'Z', sizeof buf);
	check("snprintf size 0 writes nothing", snprintf(buf, 0, "x%d", 5) == 2 && buf[0] == 'Z');
	check("snprintf mixed", snprintf(buf, sizeof buf, "%s-%c-%x", "ab", 'Z', 255) == 7
	      && strcmp(buf, "ab-Z-ff") == 0);
	check("snprintf width", snprintf(buf, sizeof buf, "[%4d]", 42) == 6
	      && strcmp(buf, "[  42]") == 0);
	check("sprintf returns length", sprintf(buf, "%s", "hello") == 5 && strcmp(buf, "hello") == 0);

	printf("STRINGS: %d checks passed, %d failed\n", passes, fails);
	if (fails == 0)
		printf("QDOS/386 R11A STRINGS PASS\n");
	else
		printf("QDOS/386 R11A STRINGS FAIL\n");
	return fails == 0 ? 0 : 1;
}
