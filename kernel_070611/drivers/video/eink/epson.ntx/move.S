
#if 1

#include <linux/linkage.h>
#include <asm/assembler.h>

		.text

/*
 * Prototype: void *MoveImage(void *dest, const void *src, size_t n);
 *
 * Note:
 *
 * If the memory regions don't overlap, we simply branch to memcpy which is
 * normally a bit faster. Otherwise the copy is done going downwards.  This
 * is a transposition of the code from copy_template.S but with the copy
 * occurring in the opposite direction.
 */

ENTRY(MoveImage)

   	mov 		r2, r2, lsr #1
;1:	ldr 		r3, [r1], #4

	ldr r3, [r1]
	str r3, [r0]

1:	ldmia 	r1!, {r3}
		str 		r3,	[r0]
		subs 		r2, r2, #1
   	bne 		1b
   	nop
   	nop
   	mov 		pc, lr

#endif
