.data

message:
.asciiz "Hello MIPS!"

second:
.asciiz "ABC"

number:
.word 42

.text

main:
addi $v0, $zero, 10
syscall