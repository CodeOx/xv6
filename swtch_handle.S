# Context switch
#
#   void swtch_handle(struct context **old, struct context *new, void (*sig_handle)());
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

.globl swtch_handle
swtch_handle:
  movl 4(%esp), %eax
  movl 8(%esp), %edx
  movl 12(%esp), %ecx

  # Save old callee-saved registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
  movl %esp, (%eax)
  movl %edx, %esp

  # Load new callee-saved registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp

  jmp *%ecx

  ret
