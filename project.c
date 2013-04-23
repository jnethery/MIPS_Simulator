/*
 * project.c - A MIPS simulator written in C. Simulates instruction fetching, instruction partitioning,
 * instruction decoding, reading and writing to registers, sign extending, ALU operations, 
 * reading and writing to memory and updating the program counter.
 * authors: Josiah Nethery
 */

#include "spimcore.h"

/* ALU */
/* 10 Points */
/*** The ALU (Arithmetic Logic Unity)
*		The ALU takes in values from either two registers or a register and an offset value (i.e., for addi).
*		The ALU will perform operations such as addition, subtraction, bit-shifting, etc and spit out a result and a Zero flag.
***/
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero)
{
	switch (ALUControl)
	{
		case '0': //add
			*ALUresult = A + B;
			break;
		case '1': //sub
			*ALUresult = A - B;
			break;
		case '2': //slt, slti
			if (A < B)
				*ALUresult = 1;
			else
				*ALUresult = 0;
			break;
		case '3': //sltu, sltiu
			if ((int)A < (int)B)
				*ALUresult = 1;
			else
				*ALUresult = 0;
			break;
		case '4': //and
			*ALUresult = A & B;
			break;
		case '5': //or
			*ALUresult = A | B;
			break;
		case '6': //sll
			*ALUresult = B << 16;
			break;
	}
	if (*ALUresult == 0)
		*Zero = '1';
	else
		*Zero = '0';
}

/* instruction fetch */
/* 10 Points */
/*** Instruction fetch
*		In the instruction fetch stage, the instruction from memory at the address indicated by the PC
*		(program counter) is loaded. This should be a 32-bit number. The instruction has not yet been decoded.
*		During this stage, the program counter is checked to see if it is word-aligned or over the memory limit
*		for the machine. The program halts if either of these cases are true.
***/
int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction)
{
	if (PC%4 != 0 || PC > 65536) //checking to see if the PC is word-aligned
	{
		return 1;
	}
	else
	{
		*instruction = Mem[PC >> 2]; //get instruction from memory
		return 0;
	}
}


/* instruction partition */
/* 10 Points */
/*** Instruction partition
*		In the instruction partition stage, the 32-bit instruction is partitioned (read: split) into 7 parts:
*		The op, which is the operation code, r1-r3, which are the registers used in this operation, funct, which is the function code
*		for r-type instructions, offset, which is used for both immediate values and branching and the jsec, which is the address to
*		jump to in the event of a jump.
***/
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1,unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{
	//use bit-masking and shifting to partition the instruction
	//since, for example, we know that the first 6 bits are the op-code, we shift the 32-bit instruction right 26 digits,
	//leaving us with the 6 bits and 26 trailing 0's.
	//using a bit-mask (which is just a logical and in this case), we can take the bits from a number that matter to us.
	//if, for example, we had a 5-bit number but we only concerned with the first 4 bits, we could say (number & 01111) or (number & 0x4).
	//it's much easier (and concise) to write the mask in hexadecimal than it is to write it in binary.
	*op = instruction >> 26;
	*r1 = instruction >> 21 & 0x1F;
	*r2 = instruction >> 16 & 0x1F;
	*r3 = instruction >> 11 & 0x1F;
	*funct = instruction & 0x3F;
	*offset = instruction & 0xFFFF;
	*jsec = instruction & 0x3FFFFFF;
}



/* instruction decode */
/* 15 Points */
/*** Instruction decode
*		In the instruction decode stage, you need to look at the instruction's op-code and from there 
*		set the control signals to correspond with the operation. If, for example, you were to execute a store word
*		instruction (sw), you'd know that you're going to need to write to memory and that your ALU source is going 
*		to be an offset, so you would set your ALUSrc control signal to 1 and your MemWrite control signal to 1.
*		Everything else can be set to 0 (with the exception of RegDst, which must be set to 2, since 0 is a valid RegDst).
***/
		
int instruction_decode(unsigned op,struct_controls *controls)
{
	/*** The comments below were for our quick reference when setting the control signals correctly ***/
	//MemRead - the operation needs to read from memory
	//MemWrite - the operation needs to write to memory
	//RegWrite - the operation needs to write to a register
	//RegDst - 0 if the operation is storing in r2 (i-type/branch), 1 if it's storing in r3 (r-type)
	//Jump - if the operation causes a jump
	//Branch - if the operation causes a branch
	//MemtoReg - if the operation needs to write to a register from memory
	//ALUSrc - 0 if the ALU source is the r2 register (r-type), 1 if the ALU source is the s-ext
	//ALUOp - 0 - add or don't care, 1 - sub, 2 - slt, 3 - sltu, 4 - and, 5 - or, 6 - sll, 7 - r-type

	// r-type
	if(op == 0)
	{
		controls->MemRead = '0';
		controls->MemWrite = '0';
		controls->RegWrite = '1';
		controls->RegDst = '1';
		controls->Jump = '0';
		controls->Branch = '0';
		controls->MemtoReg = '0';
		controls->ALUSrc = '0';
		controls->ALUOp = '7';
		return 0;
	}
	
	// jump
	else if(op == 2)
	{
		controls->RegDst = '2';
		controls->Jump = '1';
		controls->Branch = '0';
		controls->MemtoReg = '0';
		controls->MemRead = '0';
		controls->MemWrite = '0';
		controls->RegWrite = '0';
		controls->ALUSrc = '1';
		controls->ALUOp = '0';
		return 0;
	}
	
	// beq
	else if(op == 4)
	{
		controls->MemRead = '0';
		controls->MemWrite = '0';
		controls->RegWrite = '0';
		controls->RegDst = '2';
		controls->Jump = '0';
		controls->Branch = '1';
		controls->MemtoReg = '0';
		controls->ALUSrc = '0';
		controls->ALUOp = '1';
		return 0;
	}

	// addi
	else if(op == 8)
	{
		controls->MemRead = '0';
		controls->MemWrite = '0';
		controls->RegWrite = '1';
		controls->RegDst = '0';
		controls->Jump = '0';
		controls->Branch = '0';
		controls->MemtoReg = '0';
		controls->ALUSrc = '1';
		controls->ALUOp = '0';
		return 0;
	}
	
	// slti
	else if(op == 10)
	{
		controls->MemRead = '0';
		controls->MemWrite = '0';
		controls->RegWrite = '1';
		controls->RegDst = '0';
		controls->Jump = '0';
		controls->Branch = '0';
		controls->MemtoReg = '0';
		controls->ALUSrc = '1';
		controls->ALUOp = '2';
		return 0;
	}

	// sltiu
	else if(op == 11)
	{
		controls->MemRead = '0';
		controls->MemWrite = '0';
		controls->RegWrite = '1';
		controls->RegDst = '0';
		controls->Jump = '0';
		controls->Branch = '2';
		controls->MemtoReg = '0';
		controls->ALUSrc = '1';
		controls->ALUOp = '3';
		return 0;
	}
	
	// lui
	else if(op == 15)
	{
		controls->MemRead = '0';
		controls->MemWrite = '0';
		controls->RegWrite = '1';
		controls->RegDst = '0';
		controls->Jump = '0';
		controls->Branch = '0';
		controls->MemtoReg = '0';
		controls->ALUSrc = '1';
		controls->ALUOp = '6';
		return 0;
	}

	// lw
	else if(op == 35)
	{
		controls->MemRead = '1';
		controls->MemWrite = '0';
		controls->RegWrite = '1';
		controls->RegDst = '0';
		controls->Jump = '0';
		controls->Branch = '0';
		controls->MemtoReg = '1';
		controls->ALUSrc = '1';
		controls->ALUOp = '0';   
		return 0;
	}

	// sw
	else if(op == 43)
	{
		controls->MemRead = '0';
		controls->MemWrite = '1';
		controls->RegWrite = '0';
		controls->RegDst = '2';
		controls->Jump = '0';
		controls->Branch = '0';
		controls->MemtoReg = '0';
		controls->ALUSrc = '1';
		controls->ALUOp = '0';
		return 0;
	}

	// illegal operation
	else
	{
		return 1;
	}
}

/* Read Register */
/* 5 Points */
/*** Read register
*		In the register read stage, you simply take the values from the registers and put them in their respective 
*		data variables. This is simulating a bit-stream coming out of the registers. 
***/
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2)
{
	//set data equal to the register values
	*data1 = Reg[r1];
	*data2 = Reg[r2];
}


/* Sign Extend */
/* 10 Points */
/*** Sign extension
*		In the sign extension stage, you take an offset value, and examine its most significant bit. 
*		If the MSB is 0, then the number is positive and if it's 1, the number is negative. 
*		You extend this bit to the left 16-bits, turning the 16-bit offset into a 32-bit number.
***/
void sign_extend(unsigned offset,unsigned *extended_value)
{
	*extended_value = offset >> 15 == 0 ? offset : (offset | 0xFFFF0000);
}

/* ALU operations */
/* 10 Points */
/*** ALU operations
*		In the ALU operations stage, you look at the ALUSrc control signal to see whether or not you're going to need to use r2 in your ALU 
*		or if you're going to use the extended offset. With an ALUSrc of 0, you use register 2 and the ALU operation is dictated by the funct 
*		value (or in the case of branching on equal, if the ALUOp control signal is 1). With an ALUSrc of 1, you use the extended offset in your
*		ALU operation and you use your ALUOp control signal to determine the operation. 
***/
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
	if (ALUSrc == '0') //r-type
	{
		if(funct == 32) //add
		{
			ALU(data1, data2, '0', ALUresult, Zero); 
			return 0; 
		}
		else if(funct == 34) //sub
		{
			ALU(data1, data2, '1', ALUresult, Zero); 
			return 0;
		}
		else if(funct == 36) //and
		{
			ALU(data1, data2, '4', ALUresult, Zero); 
			return 0;
		}
		else if(funct == 37) //or
		{
			ALU(data1, data2, '5', ALUresult, Zero); 
			return 0;
		}
		else if(funct == 42) //slt
		{
			ALU(data1, data2, '2', ALUresult, Zero); 
			return 0; 
		}
		else if(funct == 43) //sltu
		{
			ALU(data1, data2, '3', ALUresult, Zero); 
			return 0;
		}
		else if(ALUOp == '1') //beq
		{
			ALU(data1, data2, '1', ALUresult, Zero);
			return 0;
		}
		else
			return 1;
	}
	else if (ALUSrc == '1') //i-type and branching
	{
		if(ALUOp == '0') //addi, sw, lw
		{
			ALU(data1, extended_value, ALUOp, ALUresult, Zero); 
			return 0;
		} 
		else if(ALUOp == '2') //slti
		{
			ALU(data1, extended_value, ALUOp, ALUresult, Zero); 
			return 0;
		}
		else if(ALUOp == '3') //sltiu
		{
			ALU(data1, extended_value, ALUOp, ALUresult, Zero); 
			return 0;
		}
		else if(ALUOp == '6') //lui
		{
			ALU(data1, extended_value, ALUOp, ALUresult, Zero); 
			return 0;
		}
		else
			return 1;
	}
	else
	{
		return 1;
	}
}

/* Read / Write Memory */
/* 10 Points */
/*** Read or Write Memory
*		In the memory read write stage, you check your MemWrite and MemRead control signals to determine if you're
*		going to be reading or writing to memory. If your're going to be reading or writing, then you have to check
*		your program counter to make sure that it's word-aligned. If it's not, then the program will halt, because 
*		writing to an un-aligned address will completely screw up your memory (or your registers). When you write to 
*		memory, you write a register value to the address obtained from the ALU adding together an offset with a register 
		value and when you read from memory, you read from that address into a register. 
***/
int rw_memory(unsigned ALUresult,unsigned data1,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{
	if ((ALUresult % 4) != 0 && (MemWrite == '1' || MemRead == '1'))
		return 1;
	if (ALUresult > 65536 && (MemWrite == '1' || MemRead == '1'))
		return 1;
		
	ALUresult = ALUresult >> 2;
	if (MemWrite == '1')
	{
		Mem[ALUresult] = data1;
	}
	if (MemRead == '1')
	{
		*memdata = Mem[ALUresult];
	}
	return 0;
}


/* Write Register */
/* 10 Points */
/*** Write register
*		In the register write stage, you need to check your RegWrite control signal to see if you're allowed to write to a register.
*		If you are, then you need to check whether or not you're going to be writing from memory or from the ALU result (MemtoReg control
*		signal determines this). If your RegDst control signal is 0, you're going to be writing to r2 and if it's 1, you're going to be writing
*		to r3. 
***/
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{
	if (RegWrite == '1')
	{
		if (MemtoReg == '0')
		{
			if (RegDst == '0')
				Reg[r2] = ALUresult;
			else if (RegDst == '1')
				Reg[r3] = ALUresult;
		}
		else if (MemtoReg == '1')
		{
			if (RegDst == '0')
				Reg[r2] = memdata;
			else if (RegDst == '1')
				Reg[r3] = memdata;
		}
	}
}

/* PC update */
/* 10 Points */
/*** Program Counter update
*		In the Program Counter update stage, you have to check your Jump, Branch and Zero control signals to see if you're going
*		to update your program counter by a number other than the usual 4. If the Jump control signal is 1, then you know you're going
*		to have to use a masked and shifted jsec value, which represents a location in memory. If your Branch control signal is one AND
* 		your Zero control signal is 1 (signifying that the branch IF EQUAL condition is met), then you change your Program Counter by the 
*		offset plus 4. 
***/
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
	if (Jump == '1')
	{
		*PC = (*PC & 0xF8000000) + (jsec << 2);	
	}
	else if (Branch == '1' && Zero == '1')
	{
		*PC = *PC + (extended_value << 2) + 4; 
	}
	else
	{
		*PC = *PC + 4;
	}
}

