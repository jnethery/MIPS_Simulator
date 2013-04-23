/*
 * assembler.c - A basic MIPS assembler with limited functionality written in C.
 * authors: Josiah Nethery
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 256

/***
*		This struct is used to store all the necessary data for an instruction. The *next pointer
*		is used because the instructions are stored in a linked-list abstract data type
***/
struct instruction
{
	int op;
	int r1;
	int r2;
	int r3;
	int funct;
	int offset;
	int jsec;
	char label[BUFFER_SIZE];
	int address;
	struct instruction *next;
};

/***
*		These register names are basically copied and pasted from spimcore and are used 
*		when turning register names such as $v0 or $2 into ints.
***/
const char RegName[36][6] = {
	"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", 
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", 
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
	"pc", "stat", "lo", "hi" };

/***
*		Functions prototypes
***/
void print_output(struct instruction *inst, FILE *output);
struct instruction* set_label_addresses(struct instruction *inst, FILE *input);
struct instruction* process_file(struct instruction *inst, FILE *input);
void set_op_funct(int *op, int *funct, char *string);
int get_reg(char *string);
void strip_reg_string(char *string, char *stripped, int trim_radius);
void get_mem_offset_and_word_reg(char *string, int *mem_offset, int *mem_reg);
int check_for_label(char* string);
int find_label_address(struct instruction *inst, char label[BUFFER_SIZE]);
int calculate_offset(int start_address, int end_address);
struct instruction* add_ll_node(struct instruction *inst, char label[BUFFER_SIZE]);
struct instruction* modify_ll_node(struct instruction *inst, int address, int op, int r1, int r2, int r3, int funct, int offset, int jsec);

/*** main
*
***/
int main(int argc, char **argv)
{
	FILE *input;
	FILE *output;
	struct instruction *inst = NULL; //initializing the instruction list
	if(argc == 1) //if argc is 1, that means that only the executable name is specified and there's no input or output files 
	{
		printf("Error: No file specified\n");
	}
	else if(argc == 2) //if argc is 2, that means that only the executable name and input file are specified 
	{
		printf("Error: no output file specified\n");
	}
	else if (argc == 3) //if argc is 3, we're good to go
	{
		input = fopen(argv[1], "r"); //open the input file for reading
		output = fopen(argv[2], "w"); //open the output file for writing 
		inst = set_label_addresses(inst, input); //cycle through the input file and look for labels and note their memory addresses. 
		fclose(input); //close the input file and re-open it to go through the main processing loop
		input = fopen(argv[1], "r");
		inst = process_file(inst, input); 
		print_output(inst, output); //write to the ouput file
		fclose(input);
		fclose(output);
	}
	else //if argc is over 3, then there are too many arguments
	{
		printf("Error: Too many arguments passed to assembler\n");
	}
	return 0;
}

/*** print_output
*		print_output takes in the instruction linked list and the output file as arguments.
*		the instruction list is cycled through and the code in hex is output to the destination file.
***/ 
void print_output(struct instruction *inst, FILE *output)
{
	while (inst != NULL)
	{
		int code = 0x0;
		code += inst->op << 26;
		code += inst->r1 << 21;
		code += inst->r2 << 16;
		code += inst->r3 << 11;
		code += inst->funct & 0x3F;
		code += inst->offset & 0xFFFF;
		code += (inst->jsec >> 2) & 0x3FFFFFF;
		fprintf(output, "%08x\n", code);
		inst = inst->next;
	}
}

/*** set_label_addresses
*		This function is used to initially cycle through the input file looking for labels. This is necessary,
*		because if, for example, you come across a beq instruction that references a label that hasn't been 
*		declared yet, the compilation will fail. 
***/ 
struct instruction* set_label_addresses(struct instruction *inst, FILE *input)
{
	char string[BUFFER_SIZE];
	char label[BUFFER_SIZE];
	char* c;
	while(fscanf(input, "%s", string) != EOF)
	{
		if (check_for_label(string) == 1)
			strcpy(label, string);
		else
			strcpy(label, "");
		inst = add_ll_node(inst, label);
		c = fgets(string, BUFFER_SIZE, input);
	}
	return inst;
}

/*** process_file
*		This function is the main input file processing loop. It takes the instruction string in ASM and turns it into 
*		machine code. First, the op-code and corresponding funct are determined and then the register names are read in and processed.
*		The offset and jsec values are determined using other functions. 
***/ 
struct instruction* process_file(struct instruction *inst, FILE *input)
{
	char string[BUFFER_SIZE];
	//note that the current instruction address is tracked. this is for use with branching and jumps.
	int op, r1, r2, r3, funct, offset, jsec, address=0x4000; 
	while(fscanf(input, "%s", string) != EOF)
	{
		if (check_for_label(string) == 1)
		{
			fscanf(input, "%s", string);
		}
		set_op_funct(&op, &funct, string);
		if (op == 0) //add, sub, and, or, slt, sltu
		{
			fscanf(input, "%s", string);
			r3 = get_reg(string);
			fscanf(input, "%s", string);
			r1 = get_reg(string);
			fscanf(input, "%s", string);
			r2 = get_reg(string);
			offset = 0;
			jsec = 0;
		}
		else if (op == 2) //j
		{
			r1 = 0;
			r2 = 0;
			r3 = 0;
			offset = 0;
			fscanf(input, "%s", string);
			jsec = find_label_address(inst, strcat(string,":"));
		}
		else if (op == 4) //beq
		{
			fscanf(input, "%s", string);
			r1 = get_reg(string);
			fscanf(input, "%s", string);
			r2 = get_reg(string);
			r3 = 0;
			fscanf(input, "%s", string);
			offset = calculate_offset(address, find_label_address(inst, strcat(string,":")));
			jsec = 0;
		}
		else if (op == 8) //addi
		{
			fscanf(input, "%s", string);
			r1 = get_reg(string);
			fscanf(input, "%s", string);
			r2 = get_reg(string);
			r3 = 0;
			fscanf(input, "%d", &offset);
			jsec = 0;		
		}
		else if (op == 10 || op == 11) //slti, sltiu
		{
			fscanf(input, "%s", string);
			r2 = get_reg(string);
			fscanf(input, "%s", string);
			r1 = get_reg(string);
			r3 = 0;
			fscanf(input, "%d", &offset);
			jsec = 0;	
		}
		else if (op == 15) //lui
		{
			fscanf(input, "%s", string);
			r1 = 0;
			r2 = get_reg(string);
			r3 = 0;
			fscanf(input, "%d", &offset);
			jsec = 0;
		}
		else if (op == 35 || op == 43) //lw, sw
		{
			fscanf(input, "%s", string);
			r2 = get_reg(string);
			fscanf(input, "%s", string);
			int offset, reg;
			get_mem_offset_and_word_reg(string, &offset, &reg);
			r1 = reg;
			r3 = 0;
			offset = offset;
			jsec = 0;
		}
		inst = modify_ll_node(inst, address, op, r1, r2, r3, funct, offset, jsec);
		address += 0x4; //the current address is incremented by 4 after every instruction is processed 
	}
	return inst;
}

/*** set_op_funct
*		This function sets the op-code and corresponding funct code for the instruction read in.
***/
void set_op_funct(int *op, int *funct, char *string)
{
	int error = -100;
	if (strcmp(string, "add") == 0)
	{
		*op = 0;
		*funct = 32;
	}
	else if (strcmp(string, "sub") == 0)
	{
		*op = 0;
		*funct = 34;
	}
	else if (strcmp(string, "and") == 0)
	{
		*op = 0;
		*funct = 36;
	}
	else if (strcmp(string, "or") == 0)
	{
		*op = 0;
		*funct = 37;
	}
	else if (strcmp(string, "slt") == 0)
	{
		*op = 0;
		*funct = 42;
	}
	else if (strcmp(string, "sltu") == 0)
	{
		*op = 0;
		*funct = 43;
	}
	else if (strcmp(string, "j") == 0)
	{
		*op = 2;
		*funct = 0;
	}
	else if (strcmp(string, "beq") == 0)
	{
		*op = 4;
		*funct = 0;
	}
	else if (strcmp(string, "addi") == 0)
	{
		*op = 8;
		*funct = 0;
	}
	else if (strcmp(string, "slti") == 0)
	{
		*op = 10;
		*funct = 0;
	}
	else if (strcmp(string, "sltiu") == 0)
	{
		*op = 11;
		*funct = 0;
	}
	else if (strcmp(string, "lui") == 0)
	{
		*op = 15;
		*funct = 0;
	}
	else if (strcmp(string, "lw") == 0)
	{
		*op = 35;
		*funct = 0;
	}
	else if (strcmp(string, "sw") == 0)
	{
		*op = 43;
		*funct = 0;
	}
	else
		*op = error; //this is in the event of attempting to execute an instruction that isn't implemented. 
}

/*** get_reg
*		This function is used for turning plain-text register names into int values.
***/
int get_reg(char *string)
{
	int error = -100;
	int trim_radius;
	//the trim_radius us set by seeing if the register name string ends in a comma.
	//if it does, we know to trim off two characters (the first and last, not including the terminator).
	//otherwise, we just trim off the first (the $ sign).
	if (string[strlen(string)-1] == ',')
		trim_radius = 2;
	else
		trim_radius = 1;
	char stripped[strlen(string)-trim_radius];
	strip_reg_string(string, stripped, trim_radius);
		
	int i;
	char i_string[BUFFER_SIZE];
	for (i = 0; i < 36; i++)
	{
		itoa(i, i_string, 10); //turn the value of i into a string so that it can be compared with the plain-text register name, such as $8
		if (strcmp(stripped, i_string) == 0 || strcmp(stripped, RegName[i]) == 0)
			return i; //returns the int value of the register 
	}
	return error; //in the event that an invalid register number is reference, an error code is returned 
}

/*** strip_reg_string
*		This function is used to trim the plain-text register string, based on the trim_radius calcluated
*		in get_reg.
***/
void strip_reg_string(char *string, char *stripped, int trim_radius)
{
	int i;
	for (i = 0; i < strlen(string) - trim_radius; i++)
	{
		stripped[i] = string[i+1];
	}
	stripped[i] = '\0'; //the terminator is added back to the end of the string to make it a valid string
}

/*** get_mem_offset_and_word_reg
*		This function is used to turn strings such as 100($29) into an offset int and a register int
***/
void get_mem_offset_and_word_reg(char *string, int *mem_offset, int *mem_reg)
{
	char offset[BUFFER_SIZE];
	char reg[BUFFER_SIZE];
	int i = 0, j = 0;
	while(string[i] != '(')
	{
		offset[i] = string[i];
		i++;
	}
	i++; //i is incremented again here to skip over the '(' character
	itoa(*mem_offset, offset, 10); //turns the string offset into an integer in base 10
	while(string[i] != ')')
	{
		reg[j] = string[i];
		i++;
		j++;
	}
	*mem_reg = get_reg(reg); //calls get_reg to turn the register value, such as $29 into an int
}

/*** check_for_label
*		This function checks the last character in the string (not the terminator) to see if it's ':'
*		If it is, we know that this string is supposed to be a label. Returns 1 if it's a label, and 0 otherwise.
***/
int check_for_label(char* string)
{
	if(string[strlen(string)-1] == ':')
		return 1;
	return 0;
}

/*** find_label_address
*		This function takes in a label and searches the instruction list for an instruction with that label.
*		Returns the memory address of that function.
***/
int find_label_address(struct instruction *inst, char label[BUFFER_SIZE])
{
	struct instruction *helper;
	helper = inst;
	while(strcmp(helper->label, label) != 0)
	{
		helper = helper->next;
	}
	return helper->address;
}

/*** calculate_offset
*		This function simply finds the difference in words between two memory addresses (used for branches).
***/
int calculate_offset(int start_address, int end_address)
{
	return (end_address - (start_address+0x4))/4;
}

/*** modify_ll_node
*		This function changes the values of an instruction at a specified memory address. 
*		This function is used during the main processing cycle, after the linked list has already been created.
***/
struct instruction* modify_ll_node(struct instruction *inst, int address, int op, int r1, int r2, int r3, int funct, int offset, int jsec)
{
	struct instruction *helper;
	helper = inst;
	while(helper->address != address)
	{
		helper = helper->next;
	}
	helper->op = op;
	helper->r1 = r1;
	helper->r2 = r2;
	helper->r3 = r3;
	helper->funct = funct;
	helper->offset = offset;
	helper->jsec = jsec;
	
	return inst;
}

/*** add_ll_node
*		This function creates and adds a new node to the instruction list with a label and 
*		a memory address. Note that during creation of nodes, the address variable is incremented 
*		by 4 every time we jump to the next node in the list. 
***/
struct instruction* add_ll_node(struct instruction *inst, char label[BUFFER_SIZE])
{
	struct instruction *new_inst = (struct instruction*)malloc(sizeof(struct instruction));
	strcpy(new_inst->label, label);
	new_inst->next = NULL;
	if(inst == NULL)
	{
		new_inst->address = 0x4000;
		inst = new_inst;
	}
	else
	{
		int addr = 0x4004;
		struct instruction *helper;
		helper = inst;
		while(helper->next != NULL)
		{
			helper = helper->next;
			addr += 0x4;
		}
		new_inst->address = addr;
		helper->next = new_inst;
	}
	return inst;
}