addi $t0, $zero, 3

loop:
addi $t0, $t0, -1
beq $t0, $zero, done
j loop

done:
addi $t1, $zero, 99