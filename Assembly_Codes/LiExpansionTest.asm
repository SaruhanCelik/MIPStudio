# one-instruction LI
li $t0, 10

# two-instruction LI
li $t1, 0x12345678

# signed 32-bit LI
li $t2, -50000

# label after expanded LI
target:
move $s0, $t1

nop

addi $v0, $zero, 10
syscall