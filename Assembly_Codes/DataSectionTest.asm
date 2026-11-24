# Assembly_Codes/DataSectionTest.asm

.data

number:
.word 42

negative:
.word -100

large:
.word 0x12345678

.text

main:
addi $t0, $zero, 10

addi $v0, $zero, 10
syscall