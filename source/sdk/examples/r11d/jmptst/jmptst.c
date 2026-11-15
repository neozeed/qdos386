/* JMPTST: native setjmp and longjmp (libc jmp.s).  Checks the return values,
 * longjmp(env, 0) returning 1, nested and repeated jumps, a jump from a deep
 * call chain back to a shallow frame, preservation of register variables that
 * are not changed between setjmp and longjmp, and that the stack is intact
 * afterwards.  Locals changed after setjmp are declared volatile (C89 leaves
 * their values indeterminate otherwise). */
#include <stdio.h>
#include <setjmp.h>
#include "../check.h"

static jmp_buf outer_env, inner_env, deep_env;
static int deep_result;

static void descend(int depth)
{
	if (depth == 0)
		longjmp(deep_env, 77);
	descend(depth - 1);
	deep_result = -1;            /* never reached */
}

static int count_jumps(int n)
{
	volatile int hits = 0;
	int r = setjmp(outer_env);
	hits++;
	if (r == 0 && n > 0) {
		/* jump back from the same frame n times */
		longjmp(outer_env, n);
	}
	return hits * 100 + r;       /* hits == 2 on the jump, 1 on the first pass */
}

int main(void)
{
	volatile int v1 = 0;
	int r;
	register int a = 11, b = 22, c = 33;
	volatile int stack_canary = 0x5A5A;

	/* 1. setjmp returns 0 when called directly */
	r = setjmp(inner_env);
	if (r == 0) {
		CHECK("setjmp returns 0 directly", 1);
	} else {
		CHECK("longjmp returns its value", r == 5);
		v1 = r;
	}
	if (r == 0)
		longjmp(inner_env, 5);
	CHECK("v1 holds the value after the jump", v1 == 5);

	/* 2. longjmp with 0 makes setjmp return 1 */
	r = setjmp(inner_env);
	if (r == 0)
		longjmp(inner_env, 0);
	CHECK("longjmp(env,0) returns 1", r == 1);

	/* 3. nested: a jump from an inner frame to an outer frame */
	{
		volatile int outer_pass = 0;
		r = setjmp(outer_env);
		if (r == 0) {
			outer_pass = 1;
			{
				int inner_r = setjmp(inner_env);
				if (inner_r == 0)
					longjmp(outer_env, 9);     /* skips the inner frame */
			}
			CHECK("nested: not reached", 0);
		} else {
			CHECK("nested: outer receives 9", r == 9);
			CHECK("nested: outer frame state kept", outer_pass == 1);
		}
	}

	/* 4. repeated jumps into one frame: count_jumps(n) jumps once and returns */
	CHECK("repeated: no jump returns 100", count_jumps(0) == 100);
	CHECK("repeated: jump value 4", count_jumps(4) == 204);
	CHECK("repeated: jump value 1", count_jumps(1) == 201);

	/* 5. a deep call chain jumps back to a shallow frame */
	{
		volatile int depth_done = 0;
		r = setjmp(deep_env);
		if (r == 0) {
			descend(12);
			CHECK("deep: not reached", 0);
		} else {
			depth_done = 1;
			CHECK("deep: value 77 arrives", r == 77);
			CHECK("deep: frame intact", depth_done == 1);
		}
	}

	/* 6. a loop of many jumps keeps register variables and the stack intact */
	{
		int k, ok = 1;
		volatile int passes = 0;
		for (k = 0; k < 2000; k++) {
			r = setjmp(inner_env);
			if (r == 0) {
				passes++;        /* counted once per iteration: the jump resumes after setjmp */
				longjmp(inner_env, k + 1);
			}
			if (r != k + 1)
				ok = 0;
		}
		CHECK("loop: 2000 jumps return the right value", ok);
		CHECK("loop: every iteration jumped once", passes == 2000);
	}
	CHECK("register variables kept (a)", a == 11);
	CHECK("register variables kept (b)", b == 22);
	CHECK("register variables kept (c)", c == 33);
	CHECK("stack canary intact", stack_canary == 0x5A5A);

	/* 7. setjmp in a loop with a register counter */
	{
		register int acc = 0;
		volatile int round = 0;
		jmp_buf env;
		int i;
		for (i = 0; i < 50; i++) {
			acc += i;
			if (setjmp(env) == 0 && round < 3) {
				round++;
				longjmp(env, 1);
			}
		}
		CHECK("register accumulator across jumps", acc == 1225);
		CHECK("three jumps taken", round == 3);
	}

	return finish("JMPTST");
}
