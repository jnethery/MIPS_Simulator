addi $t0, $t0, 4
addi $t1, $t1, 5
add $t5, $t0, $t1
sub $t5, $t1, $t0
and $t5, $t0, $t1
or $t5, $t0, $t1
sw $t1, 100($s2)
lw $t5, 100($s2)
lui $t4, 15
slt $t5, $t0, $t1
slti $t5, $t0, 8
sltu $t5, $t0, $t1
sltiu $t5, $t0, 8
test: beq $t0, $t1, branch
add $t1, $t0, $0
j test
branch: 
 