.data

message:
.asciiz "Hello MIPS!"

.text

main:
la $a0, message

li $v0, 4
syscall

li $v0, 10
syscall