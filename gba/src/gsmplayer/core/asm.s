@ int fracumul(unsigned int x, unsigned int frac)
@ Multiply by a 0.32 fractional number between 0 and 1.
@ Used for fast division by a constant.

.ARM
.balign 4
.GLOBL  fracumul

fracumul:
  umull r1,r2,r0,r1
  mov r0, r2
  bx lr
