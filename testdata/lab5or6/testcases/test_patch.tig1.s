.text
.globl patchtest
.type patchtest, @function
.set patchtest_framesize, 88
patchtest:
subq $88, %rsp
# remove movq %rbx, %rbx
# remove movq %rbp, %rbp
# remove movq %r12, %r12
# remove movq %r13, %r13
movq %r14, %r11
movq %r15, %r10
L1:
movq %rdi, %rax
movq %rsi, %rdi
movq %rdx, %rsi
movq %rcx, %rdx
movq %r8, %rcx
movq %r9, %r15
leaq fp, %r9
movq $-8, %r8
# remove movq %r9, %r9
addq %r8, %r9
movq (%r9), %r8  ##r 8 = parameter 7
movq %r8, %r14  #r14 = parameter 7
leaq fp, %r9
movq $-16, %r8
# remove movq %r9, %r9
addq %r8, %r9
movq (%r9), %r8
movq %r8, %r9 # r9 = parameter 8
leaq patchtest_framesize(%rsp), t167
# Ops! Spill after
movq t167, (patchtest_framesize-80)(%rsp)
movq $-24, %r8
# remove movq t147, t146
# Ops! Spilled before
movq (patchtest_framesize-72)(%rsp), t165
addq %r8, t146
# Ops! Spilled before
movq (patchtest_framesize-72)(%rsp), t166
movq (t166), %r8
# remove movq %r8, %r8
# remove movq %rax, %rax
movq %rdi, %rax
movq %rsi, %rax
movq %rdx, %rax
movq %rcx, %rax
leaq patchtest_framesize(%rsp), %rax
movq $-8, %rdi
# remove movq %rax, %rax
addq %rdi, %rax
movq %r15, (%rax)
leaq patchtest_framesize(%rsp), %rax
movq $-16, %rdi
# remove movq %rax, %rax
addq %rdi, %rax
movq %r14, (%rax)
leaq patchtest_framesize(%rsp), %rax
movq $-24, %rdi
# remove movq %rax, %rax
addq %rdi, %rax
movq %r9, (%rax)
leaq patchtest_framesize(%rsp), %rax
movq $-32, %rdi
# remove movq %rax, %rax
addq %rdi, %rax
movq %r8, (%rax)
leaq patchtest_framesize(%rsp), %rax
movq $-24, %rdi
# remove movq %rax, %rax
addq %rdi, %rax
movq (%rax), %rax
# remove movq %rax, %rax
jmp L0
L0:
# remove movq %rbx, %rbx
# remove movq %rbp, %rbp
# remove movq %r12, %r12
# remove movq %r13, %r13
movq %r11, %r14
movq %r10, %r15


addq $88, %rsp
retq
.size patchtest, .-patchtest
.globl tigermain
.type tigermain, @function
.set tigermain_framesize, 56
tigermain:
subq $56, %rsp
# remove movq %rbx, %rbx
# remove movq %rbp, %rbp
# remove movq %r12, %r12
# remove movq %r13, %r13
# remove movq %r14, %r14
# remove movq %r15, %r15
L3:
subq $8,%rsp
movq $9, %rdi
# remove movq %rdi, %rdi
movq $4, %rsi
# remove movq %rsi, %rsi
movq $3, %rdx
# remove movq %rdx, %rdx
movq $2, %rcx
# remove movq %rcx, %rcx
movq $11, %r8
# remove movq %r8, %r8
movq $11, %r9
# remove movq %r9, %r9
movq $23, %rax
subq $8,%rsp
movq %rax, (%rsp)
movq $45, %rax
subq $8,%rsp
movq %rax, (%rsp)
leaq tigermain_framesize(%rsp), %rax
subq $8,%rsp
movq %rax, (%rsp)
addq $32,%rsp
call patchtest
# remove movq %rax, %rax
# remove movq %rax, %rax
movq %rax, %rdi
call printi
# remove movq %rax, %rax
jmp L2
L2:
# remove movq %rbx, %rbx
# remove movq %rbp, %rbp
# remove movq %r12, %r12
# remove movq %r13, %r13
# remove movq %r14, %r14
# remove movq %r15, %r15


addq $56, %rsp
retq
.size tigermain, .-tigermain
.section .rodata
