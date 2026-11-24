# ================================
# BASIC ARITHMETIC
# ================================

addi $t0, $zero, 10
addi $t1, $zero, 20

add  $t2, $t0, $t1
sub  $t3, $t2, $t0

addu $t4, $t0, $t1
subu $t5, $t1, $t0


# ================================
# LOGICAL
# ================================

and  $s0, $t0, $t1
or   $s1, $t0, $t1
xor  $s2, $t0, $t1
nor  $s3, $t0, $t1

andi $s4, $t1, 0xF
ori  $s5, $zero, 0xFF
xori $s6, $s5, 0xF


# ================================
# COMPARISONS
# ================================

slt   $s7, $t0, $t1
slti  $t6, $t0, 15
sltu  $t7, $t0, $t1
sltiu $t8, $t0, 15


# ================================
# SHIFTS
# ================================

sll $t9, $t0, 2
srl $k0, $t9, 1
sra $k1, $t9, 2


# ================================
# MEMORY
# ================================

sw $t2, 100($zero)
lw $a0, 100($zero)


# ================================
# BRANCH TESTS
# ================================

beq $t0, $t0, beq_taken

addi $v1, $zero, 111

beq_taken:
addi $v1, $zero, 1

bne $t0, $t1, bne_taken

addi $a1, $zero, 111

bne_taken:
addi $a1, $zero, 2


# ================================
# JUMP TEST
# ================================

j after_jump

addi $a2, $zero, 111

after_jump:
addi $a2, $zero, 3


# ================================
# JAL / JR TEST
# ================================

jal function_test

addi $a3, $zero, 4

j after_function


function_test:
addi $a3, $zero, 40
jr $ra


after_function:


# ================================
# MULT / HI / LO
# ================================

addi $t0, $zero, 6
addi $t1, $zero, 7

mult $t0, $t1

mflo $s0
mfhi $s1


# ================================
# SYSCALL PRINT
# ================================

addi $a0, $zero, 42
addi $v0, $zero, 1
syscall


# ================================
# EXIT
# ================================

addi $v0, $zero, 10
syscall