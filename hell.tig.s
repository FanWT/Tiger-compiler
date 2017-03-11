.globl tigermain
tigermain:
pushl %ebp
movl %esp,%ebp
pushl %ebx
pushl %edx
pushl %esi
subl $60,%esp
movl $2,%ebx
movl %ebp,4(%esp)
movl %ebx,0(%esp)
call a
movl %ebp,4(%esp)
movl %eax,0(%esp)
call printi
jmp L0
L0:
addl $60,%esp
popl %esi
popl %edx
popl %ebx
movl %ebp,%esp
popl %ebp
ret

a:
pushl %ebp
movl %esp,%ebp
pushl %ebx
pushl %edx
pushl %esi
subl $60,%esp
movl $3,%ebx
movl %ebp,4(%esp)
movl %ebx,0(%esp)
call b
jmp L1
L1:
addl $60,%esp
popl %esi
popl %edx
popl %ebx
movl %ebp,%esp
popl %ebp
ret

b:
pushl %ebp
movl %esp,%ebp
pushl %ebx
pushl %edx
pushl %esi
subl $60,%esp
movl 12(%ebp),%eax
movl 8(%eax),%ebx
movl 8(%ebp),%eax
addl %eax,%ebx
movl %ebx,%eax
jmp L2
L2:
addl $60,%esp
popl %esi
popl %edx
popl %ebx
movl %ebp,%esp
popl %ebp
ret

