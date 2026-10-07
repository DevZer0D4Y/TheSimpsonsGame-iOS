/**
 * @file        rex/core/fiber_aarch64.S
 * @brief       AArch64 cooperative context switch for rex::thread::Fiber
 *
 * @license     BSD 3-Clause License
 *
 * @remarks     Android's bionic does not implement getcontext/makecontext/
 *              swapcontext (they are glibc extensions), so the POSIX fiber
 *              backend cannot be used there. Switching on a pool of host
 *              threads is not an option either: the runtime keeps per-guest-
 *              thread state in thread_local storage (XThread's
 *              current_xthread_tls_), which must survive a fiber switch. This
 *              keeps execution on the same OS thread, exactly as ucontext does.
 *
 *              void rex_fiber_switch(RexFiberContext* from, RexFiberContext* to)
 *
 *              Saves the AAPCS64 callee-saved state into *from and restores it
 *              from *to. Caller-saved registers are, by definition, already
 *              dead across a call, so they need no handling. Layout must match
 *              RexFiberContext in fiber.h.
 */

#if defined(__aarch64__)

	.text
	.global rex_fiber_switch
	.hidden rex_fiber_switch
	.type rex_fiber_switch, %function
	.align 4
rex_fiber_switch:
	/* Save callee-saved GPRs, frame pointer, link register and SP. */
	stp	x19, x20, [x0, #0]
	stp	x21, x22, [x0, #16]
	stp	x23, x24, [x0, #32]
	stp	x25, x26, [x0, #48]
	stp	x27, x28, [x0, #64]
	stp	x29, x30, [x0, #80]
	mov	x9, sp
	str	x9,  [x0, #96]

	/* AAPCS64 requires only the low 64 bits of v8-v15 to be preserved. */
	stp	d8,  d9,  [x0, #104]
	stp	d10, d11, [x0, #120]
	stp	d12, d13, [x0, #136]
	stp	d14, d15, [x0, #152]

	/* Restore the target's state. */
	ldp	x19, x20, [x1, #0]
	ldp	x21, x22, [x1, #16]
	ldp	x23, x24, [x1, #32]
	ldp	x25, x26, [x1, #48]
	ldp	x27, x28, [x1, #64]
	ldp	x29, x30, [x1, #80]
	ldr	x9,  [x1, #96]
	mov	sp, x9

	ldp	d8,  d9,  [x1, #104]
	ldp	d10, d11, [x1, #120]
	ldp	d12, d13, [x1, #136]
	ldp	d14, d15, [x1, #152]

	/* Returns into the target's x30: its SwitchTo call site, or, for a fiber
	   that has never run, the trampoline seeded by Fiber::Create. RET is not
	   a BTI-guarded branch, so no landing pad is required at either. */
	ret
	.size rex_fiber_switch, . - rex_fiber_switch

/* Mark the stack non-executable; without this the linker warns and may fall
   back to an executable stack for the whole shared object. */
	.section .note.GNU-stack, "", %progbits

#endif  /* __aarch64__ */
