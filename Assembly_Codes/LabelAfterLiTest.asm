li $t0, 0x12345678

j target

li $t1, 999

target:
li $t2, 42

addi $v0, $zero, 10
syscall