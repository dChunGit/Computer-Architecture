#include <stdio.h> /* standard input/output library */
#include <stdlib.h> /* Standard C Library */
#include <string.h> /* String operations library */
#include <ctype.h> /* Library for useful character operations */
#include <limits.h> /* Library for definitions of common variable type characteristics */

/*
*   Name 1: David Chun
*	Name 2: Chioma Okorie
*	UT EID 1: dc37875
*	UT EID 2: coo279
*/


FILE* infile = NULL;
FILE* outfile = NULL;

#define MAX_LINE_LENGTH 255
enum
{
	DONE, OK, EMPTY_LINE
};

/*struct for label types*/

#define MAX_LABEL_LEN 20
#define MAX_SYMBOLS 255
typedef struct Labels {
        int offset;
        char name[MAX_LABEL_LEN + 1];
} Labels;
Labels labels[MAX_SYMBOLS];
int currentLabel = 0;

/*opcode and other directives*/
char* opCodes[] = { "add", "and", "br", "brn", "brz", "brp", "brzp", "brnp", "brnz", "brnzp", "halt",
"jmp", "jsr", "jsrr", "ldb", "ldw", "lea", "nop", "not", "ret", "lshf", "rshfl", "rshfa", "rti",
"stb", "stw", "trap", "xor" };

char* invalidLabels[] = { "in", "out", "getc", "puts" };
char* directives[] = { ".orig", ".fill", ".end" };

#define NUMREG 8
typedef struct reg {
	char name[3];
	char num;
}numReg;

numReg InstReg[NUMREG] = { { "r0",0 },{ "r1",1 },{ "r2",2 },{ "r3",3 },{ "r4",4 },{ "r5",5 },{ "r6",6 },{ "r7",7 } };

/* output buffer */
#define ADDRESS 0xFFFF
int address[ADDRESS + 1];
int currpt = 0, outptr = 0;

/*
*	Prototypes
*/
void firstPass(FILE* infile);
void secondPass(FILE* infile);
void branch_Handler(Labels* labels, char *arg1, char *arg2, char *arg3, char *arg4, int* offset);

/*
*   Compiles input file into lc3b assembly and outputs to output file
*/
int main(int argc, char* argv[]) {

	/* open the source file */
	infile = fopen(argv[1], "r");
	outfile = fopen(argv[2], "w");

	if (!infile) {
		printf("Error: Cannot open file %s\n", argv[1]);
		exit(4);
	}
	if (!outfile) {
		printf("Error: Cannot open file %s\n", argv[2]);
		exit(4);
	}

	firstPass(infile);
	infile = fopen(argv[1], "r");
	secondPass(infile);

	int a = 0;
	for (a = 0; a < outptr; a++) {
		fprintf(outfile, "0x%.4X\n", address[a]);
	}
	
	fclose(infile);
	fclose(outfile);
}

/*
*   First pass through -> sets labels
*/
void firstPass(FILE *fPass) {

	char lLine[MAX_LINE_LENGTH + 1], *lLabel, *lOpcode, *lArg1,
		*lArg2, *lArg3, *lArg4;

	int lRet, rRet, lInstr;
	int orig, pointer = 0;

	do {
		lRet = readAndParse(fPass, lLine, &lLabel,
			&lOpcode, &lArg1, &lArg2, &lArg3, &lArg4);
		if (lRet != DONE && lRet != EMPTY_LINE) {
			int good = checkLabel(lLabel, currentLabel);
			
			if (good == 1) {
				
				/*assign label to label struct in labels array*/
				strcpy(labels[currentLabel].name, lLabel);
				labels[currentLabel].offset = pointer - 1;
				currentLabel++;
			} else if(strcmp(lLabel, "") != 0){
				exit(4);
			}

			if (strcmp(lLabel, ".orig") != 0)
				pointer++;
		}

	} while (lRet != DONE);
}

/*
*   Second pass through -> assembles instructions and writes to file
*/
void secondPass(FILE * lInfile) {

	char lLine[MAX_LINE_LENGTH + 1], *lLabel, *lOpcode, *lArg1,
		*lArg2, *lArg3, *lArg4;
	int lRet; int decode = 0;
	int start = 0;

	do
	{
		lRet = readAndParse(lInfile, lLine, &lLabel,
			&lOpcode, &lArg1, &lArg2, &lArg3, &lArg4);

		if (lRet != DONE && lRet != EMPTY_LINE) {

			/* if first line doesn't start with .org*/
 			if(start == 0) {
 				start = 1;
 				if((strcmp(lOpcode, directives[0])) != 0) {
 					exit(4);
 				}
 			}

			if (decode) {

				/* assemble instructions */
				if ((strcmp(lLabel, "") != 0) && (strcmp(lOpcode, "") == 0))
					exit(2);
				int result = assembleInst(lOpcode, lArg1, lArg2, lArg3, lArg4, &decode, labels);
				if (isOpcode(lOpcode) == 1) {
					address[outptr] = result;
					outptr++;
				}
				currpt++;

			}

			if ((strcmp(lOpcode, directives[0]) == 0) && (!decode)) {

				int value = toNum(lArg1);

				/* check if word aligned */
				if (value % 2 == 1) {
					exit(3);
				}

				if (valid_orig(lArg1) == -1)
					exit(3);
				decode = 1;

				address[outptr] = toNum(lArg1);
				outptr++;

			}
		}
	} while (lRet != DONE);

	if ((decode == 1)) {
		exit(4);
	}
}

/*
*	Assembles instruction from assembly
*
*	Returns int code for output
*/
int assembleInst(char* opcode, char*arg1, char*arg2, char*arg3, char*arg4, int* decode) {
	
	int r1, r2, r3, r4, out;
	int immed = 0, offset = 0;

	/* add -----------------------------------------------*/
	if ((strcmp(opcode, "add")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);

		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		if ((arg3[0] == '#') || (arg3[0] == 'x')) { /*argument3 is immediate*/
			immed = toNum(arg3);
			if ((immed < -16) || (immed > 15))
				exit(3);
			out = 0x1020 | (r1 << 9) | (r2 << 6) | (immed & 0x1F);
			return out;
		}
		else {/*argument3 is sr2*/
			r3 = regValue(arg3);
			if (r3 == -1)
				exit(4);
			out = 0x1000 | (r1 << 9) | (r2 << 6) | r3;
			return out;
		}
	}

	/* and -----------------------------------------------*/
	else if ((strcmp(opcode, "and")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);

		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		if ((arg3[0] == '#') || (arg3[0] == 'x')) {
			immed = toNum(arg3);
			if ((immed < -16) || (immed > 15))
				exit(3);
			out = 0x5020 | (r1 << 9) | (r2 << 6) | (immed & 0x1F);
			return out;
		}
		else {
			r3 = regValue(arg3);
			if (r3 == -1)
				exit(4);
			out = 0x5000 | (r1 << 9) | (r2 << 6) | r3;
			return out;
		}
	}

	/* br -----------------------------------------------*/
	else if ((strcmp(opcode, "br")) == 0) {

		branch_Handler(labels, arg1, arg2, arg3, arg4, &offset);
		out = 0x0E00 | (offset & 0x1FF);
		return out;

	}

	/* brn -----------------------------------------------*/
	else if ((strcmp(opcode, "brn")) == 0) {
		branch_Handler(labels, arg1, arg2, arg3, arg4, &offset);
		out = 0x0800 | (offset & 0x1FF);
		return out;
	}

	/* brz -----------------------------------------------*/
	else if ((strcmp(opcode, "brz")) == 0) {
		if (strcmp(arg1, "") == 0)
			exit(4);
		branch_Handler(labels, arg1, arg2, arg3, arg4, &offset);
		out = 0x0400 | (offset & 0x1FF);
		return out;
	}

	/* brp -----------------------------------------------*/
	else if ((strcmp(opcode, "brp")) == 0) {
		branch_Handler(labels, arg1, arg2, arg3, arg4, &offset);
		out = 0x0200 | (offset & 0x1FF);
		return out;
	}

	/* brzp -----------------------------------------------*/
	else if ((strcmp(opcode, "brzp")) == 0) {
		branch_Handler(labels, arg1, arg2, arg3, arg4, &offset);
		out = 0x0600 | (offset & 0x1FF);
		return out;
	}

	/* brnp -----------------------------------------------*/
	else if ((strcmp(opcode, "brnp")) == 0) {
		branch_Handler(labels, arg1, arg2, arg3, arg4, &offset);
		out = 0x0A00 | (offset & 0x1FF);
		return out;
	}

	/* brnz -----------------------------------------------*/
	else if ((strcmp(opcode, "brnz")) == 0) {
		branch_Handler(labels, arg1, arg2, arg3, arg4, &offset);
		out = 0x0C00 | (offset & 0x1FF);
		return out;
	}

	/* brnzp -----------------------------------------------*/
	else if ((strcmp(opcode, "brnzp")) == 0) {
		branch_Handler(labels, arg1, arg2, arg3, arg4, &offset);
		out = 0x0E00 | (offset & 0x1FF);
		return out;
	}

	/* halt-----------------------------------------------*/
	else if ((strcmp(opcode, "halt")) == 0) {
		if ((strcmp(arg1, "") != 0) && (strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		out = 0xF025;
		return out;
	}

	/* jmp -----------------------------------------------*/
	else if ((strcmp(opcode, "jmp")) == 0) {
		if (strcmp(arg1, "") == 0)
			exit(4);
		if ((strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		out = 0xC000 | (r1 << 6);
		return out;
	}

	/* jsr -----------------------------------------------*/
	else if ((strcmp(opcode, "jsr")) == 0) {
		int a = 0, valid_label = 0;
		int size = sizeof(labels) / sizeof(labels[0]);

		if (strcmp(arg1, "") == 0)
			exit(4);
		if ((strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		if((arg1[0] == 'x') || (arg1[0] == '#')) {
			exit(4);
		}
		while ((a < size) && (valid_label == 0)) {
			if (strcmp(arg1, labels[a].name) == 0) {
				offset = labels[a].offset;
				valid_label = 1;
			}
			a++;
		}
		if (valid_label == 0) exit(1);
		offset = offset - (currpt + 1);
		out = 0x5800 | (offset && 0x7FF);
		return out;
	}

	/* jsrr -----------------------------------------------*/
	else if ((strcmp(opcode, "jsrr")) == 0) {
		if (strcmp(arg1, "") == 0)
			exit(4);
		if ((strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		out = 0x5000 | (r1 << 6);
		return out;
	}

	/* ldb -----------------------------------------------*/
	else if ((strcmp(opcode, "ldb")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		offset = toNum(arg3);
		if ((offset < -32) || (offset > 31))
			exit(3);
		out = 0x2000 | (r1 << 9) | (r2 << 6) | (offset & 0x3F);
		return out;
	}

	/* ldw -----------------------------------------------*/
	else if ((strcmp(opcode, "ldw")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		offset = toNum(arg3);
		if ((offset < -32) || (offset > 31))
			exit(3);
		out = 0x6000 | (r1 << 9) | (r2 << 6) | (offset & 0x3F);
		return out;
	}

	/* lea -----------------------------------------------*/
	else if ((strcmp(opcode, "lea")) == 0) {
		int a = 0, valid_label = 0;
		int size = sizeof(labels);
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0))
			exit(4);
		if ((strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		if((arg2[0] == 'x') || (arg2[0] == '#')) {
			exit(4);
		}
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		while ((a < size) && (valid_label == 0)) {
			if (strcmp(arg2, labels[a].name) == 0) {
				offset = labels[a].offset;
				valid_label = 1;
			}
			a++;
		}
		if (valid_label == 0) exit(1);
		offset = offset - (currpt + 1);
		out = 0xE000 | (r1 << 9) | (offset & 0x1FF);
		return out;
	}

	/* nop -----------------------------------------------*/
	else if ((strcmp(opcode, "nop")) == 0) {
		if ((strcmp(arg1, "") != 0) && (strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		out = 0x0000;
		return out;
	}

	/* not -----------------------------------------------*/
	else if ((strcmp(opcode, "not")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0))
			exit(4);
		if ((strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		out = 0x903F | (r1 << 9) | (r2 << 6);
		return out;
	}

	/* ret -----------------------------------------------*/
	else if ((strcmp(opcode, "ret")) == 0) {
		if ((strcmp(arg1, "") != 0) && (strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		out = 0xC1C0;
		return out;
	}

	/* lshf -----------------------------------------------*/
	else if ((strcmp(opcode, "lshf")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		immed = toNum(arg3);
		if ((immed < 0) || (immed > 15))
			exit(3);
		out = 0xD000 | (r1 << 9) | (r2 << 6) | (immed & 0xF);
		return out;
	}

	/* rshfl -----------------------------------------------*/
	else if ((strcmp(opcode, "rshfl")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		immed = toNum(arg3);
		if ((immed < 0) || (immed > 15))
			exit(3);
		out = 0xD010 | (r1 << 9) | (r2 << 6) | (immed & 0xF);
		return out;
	}

	/* rshfa -----------------------------------------------*/
	else if ((strcmp(opcode, "rshfa")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		immed = toNum(arg3);
		if ((immed < 0) || (immed > 15))
			exit(3);
		out = 0xD030 | (r1 << 9) | (r2 << 6) | (immed & 0xF);
		return out;
	}

	/* rti -----------------------------------------------*/
	else if ((strcmp(opcode, "rti")) == 0) {
		if ((strcmp(arg1, "") != 0) && (strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		out = 0x8000;
		return out;
	}

	/* stb -----------------------------------------------*/
	else if ((strcmp(opcode, "stb")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		offset = toNum(arg3);
		if ((offset < -32) || (offset > 31))
			exit(3);
		out = 0x3000 | (r1 << 9) | (r2 << 6) | (offset & 0x3F);
		return out;
	}

	/*stw -----------------------------------------------*/
	else if ((strcmp(opcode, "stw")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		offset = toNum(arg3);
		if ((offset < -32) || (offset > 31))
			exit(3);
		out = 0x7000 | (r1 << 9) | (r2 << 6) | (offset & 0x3F);
		return out;
	}

	/* trap-----------------------------------------------*/
	else if ((strcmp(opcode, "trap")) == 0) {
		if (strcmp(arg1, "") == 0)
			exit(4);
		if ((strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		if (arg1[0] != 'x')
			exit(3);
		offset = toNum(arg1);
		if ((offset > 255) || (offset < 0))
			exit(3);
		out = 0xF000 | (offset & 0xFF);
		return out;
	}

	/* xor -----------------------------------------------*/
	else if ((strcmp(opcode, "xor")) == 0) {
		if ((strcmp(arg1, "") == 0) || (strcmp(arg2, "") == 0) || (strcmp(arg3, "") == 0))
			exit(4);
		if (strcmp(arg4, "") != 0)
			exit(4);
		r1 = regValue(arg1);
		if (r1 == -1)
			exit(4);
		r2 = regValue(arg2);
		if (r2 == -1)
			exit(4);
		if ((arg3[0] == '#') || (arg3[0] == 'x')) { /* immediate */
			immed = toNum(arg3);
			if ((immed < -16) || (immed > 15))
				exit(3);
			out = 0x9020 | (r1 << 9) | (r2 << 6) | (immed & 0x1F);
			return out;
		}
		else {/* arg3 is sr2 */
			r3 = regValue(arg3);
			if (r3 == -1)
				exit(4);
			out = 0x9000 | (r1 << 9) | (r2 << 6) | r3;
			return out;
		}
	}

	/*pseudo-ops*/

	/* .orig -----------------------------------------------*/
	else if ((strcmp(opcode, ".orig")) == 0) {
		if (*decode == 1)  /* if there is a .orig without .end */
			exit(2);      /* not sure of correct exit */
	}

	/*.fill -----------------------------------------------*/
	else if ((strcmp(opcode, ".fill")) == 0) {
		if (strcmp(arg1, "") == 0)
			exit(4);
		if ((strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		if ((arg1[0] == '#') || (arg1[0] = 'x')) {
			offset = toNum(arg1);
			out = toNum(arg1);
		}
		if (offset < -32768)
			exit(3);
		else if (out > 65535)
			exit(3);
		/*fprintf(outfile, "0x%.4X\n", offset);*/
		address[outptr] = offset;
		outptr++;
	}

	/*.end -----------------------------------------------*/
	else if ((strcmp(opcode, ".end")) == 0) {
		if ((strcmp(arg1, "") != 0) && (strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
			exit(4);
		*decode = 0;
	}
	else {
		exit(2);
	}
}

/*
*	Handles the offset of branches/offsets
*/
void branch_Handler(Labels* labels, char *arg1, char *arg2, char *arg3, char *arg4, int* offset) {

	int out = 0;
	int a = 0, valid_label = 0;
	int size = currentLabel;

	if (strcmp(arg1, "") == 0)
		exit(4);
	if ((strcmp(arg2, "") != 0) && (strcmp(arg3, "") != 0) && (strcmp(arg4, "") != 0))
		exit(4);
	if((arg1[0] == 'x') || (arg1[0] == '#')) {
		exit(4);
	}

	while ((a < size) && (valid_label == 0)) {
		if (strcmp(arg1, labels[a].name) == 0) {
			*offset = labels[a].offset;
			valid_label = 1;
		}
		a++;
	}
	if (valid_label == 0) exit(1);
	*offset = *offset - (currpt + 1);
}

/*
*	Converts char* register to register number
*
*	Returns number of register from register map, -1 if invalid
*/
int regValue(char* reg) {

	int i = 0;
	for (i = 0; i < NUMREG; i++) {
		if (strcmp(reg, InstReg[i].name) == 0)
			return InstReg[i].num;
	}
	return -1;
}

/*
*	Checks valid .orig memory address
*
*	Returns 1 if valid
*/
int valid_orig(char* arg1) {
	int arg = toNum(arg1);
	if ((arg < 0) || (arg > 0xFFFF))
		return -1;
	else
		return 0;
}

/*
*   Determines if label is valid
*
*   Returns 1 if valid, 0 otherwise
*/
int checkLabel(char * label, int curLength) {

	int check = 1, a = 0;
	int size = sizeof(invalidLabels) / sizeof(invalidLabels[0]);

	/*label is not part of the identied invalids*/
	while (a < size && check == 0) {
		if (strcmp(label, invalidLabels[a]) == 0) {
			check = 0;
		}
		a++;
	}

	/*label is not an opcode*/
	if (sizeof(*label) < 21 && label != NULL && isOpcode(label) == -1 &&
		check == 1 && isLetterorNumber(label)) {
		check = 1;
	} else check = 0;

	/*label is not a register*/
	if (regValue(label) != -1) {
		check = 0;
	}

	/*no order label has the same name as current label*/
	int i = 0;
	for (i = 0; i < curLength; i++) {
		if (strcmp(labels[i].name, label) == 0)
			check = 0;
	}

	return check;
}

/*
*   Determines if label starts with a letter or number
*
*   Returns 1 if valid, 0 otherwise
*/
int isLetterorNumber(char* label) {

	int check = 1, a = 0;
	if (strlen(label) == 0) {
		check = 0;
	}

	while (a < strlen(label) && check == 1) {

		if ((label[a] < '0' || label[a] > '9') && (label[a] < 'a' || label[a] > 'z')) {
			check = 0;
		}
		a++;
	}
	if (isdigit(label[0])) check = 0;

	return check && label[0] != 'x';
}

/*
*   Converts char* to an int
*
*   Returns converted int
*/
int toNum(char * pStr) {

	char * t_ptr;
	char * orig_pStr;
	int t_length, k;
	int lNum, lNeg = 0;
	long int lNumLong;

	orig_pStr = pStr;
	if (*pStr == '#')                                /* decimal */
	{
		pStr++;
		if (*pStr == '-')                                /* dec is negative */
		{
			lNeg = 1;
			pStr++;
		}
		t_ptr = pStr;
		t_length = strlen(t_ptr);
		for (k = 0; k < t_length; k++)
		{
			if (!isdigit(*t_ptr))
			{
				printf("Error: invalid decimal operand, %s\n", orig_pStr);
				exit(4);
			}
			t_ptr++;
		}
		lNum = atoi(pStr);
		if (lNeg)
			lNum = -lNum;

		return lNum;
	}
	else if (*pStr == 'x')        /* hex     */
	{
		pStr++;
		if (*pStr == '-')                                /* hex is negative */
		{
			lNeg = 1;
			pStr++;
		}
		t_ptr = pStr;
		t_length = strlen(t_ptr);
		for (k = 0; k < t_length; k++)
		{
			if (!isxdigit(*t_ptr))
			{
				printf("Error: invalid hex operand, %s\n", orig_pStr);
				exit(4);
			}
			t_ptr++;
		}
		lNumLong = strtol(pStr, NULL, 16);    /* convert hex string into integer */
		lNum = (lNumLong > INT_MAX) ? INT_MAX : lNumLong;
		if (lNeg)
			lNum = -lNum;
		return lNum;
	}
	else
	{
		printf("Error: invalid operand, %s\n", orig_pStr);
		exit(4);  /* This has been changed from error code 3 to error code 4, see clarification 12 */
	}
}

/*
*   Reads the infile and sets all pointers to the appropriate locations
*
*   Returns success signal
*/
int readAndParse(FILE * pInfile, char * pLine, char ** pLabel, char
	** pOpcode, char ** pArg1, char ** pArg2, char ** pArg3, char ** pArg4) {

	char * lRet, *lPtr;
	int i;
	if (!fgets(pLine, MAX_LINE_LENGTH, pInfile))
		return(DONE);
	for (i = 0; i < strlen(pLine); i++)
		pLine[i] = tolower(pLine[i]);

	/* convert entire line to lowercase */
	*pLabel = *pOpcode = *pArg1 = *pArg2 = *pArg3 = *pArg4 = pLine + strlen(pLine);

	/* ignore the comments */
	lPtr = pLine;

	while (*lPtr != ';' && *lPtr != '\0' && *lPtr != '\n')
		lPtr++;

	*lPtr = '\0';
	if (!(lPtr = strtok(pLine, "\t\n ,")))
		return(EMPTY_LINE);

	if (isOpcode(lPtr) == -1 && lPtr[0] != '.') /* found a label */
	{
		*pLabel = lPtr;
		if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);
	}

	*pOpcode = lPtr;

	if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);

	*pArg1 = lPtr;

	if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);

	*pArg2 = lPtr;
	if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);

	*pArg3 = lPtr;

	if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);

	*pArg4 = lPtr;

	return(OK);
}

/*
*   Determines if ptr points to a valid opcode or not
*
*   Returns 1 if the ptr points to a valid opCode
*/
int isOpcode(char* ptr) {

	int check = -1, a = 0;
	int size = sizeof(opCodes) / sizeof(opCodes[0]);

	while (a < size && check == -1) {
		if (strcmp(ptr, opCodes[a]) == 0) {
			check = 1;
		}
		a++;
	}

	return check;
}
