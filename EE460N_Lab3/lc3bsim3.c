/*
    Name 1: David Chun
    UTEID 1: dc37875
*/

/***************************************************************/
/*                                                             */
/*   LC-3b Simulator                                           */
/*                                                             */
/*   EE 460N                                                   */
/*   The University of Texas at Austin                         */
/*                                                             */
/***************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_BUS_drivers();
void drive_BUS();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the BUS.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {                                                  
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); } 
int GetLSHF1(int *x)         { return(x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x08000 
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the BUS */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it 
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */ 
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {                                                    
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & BUS values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                

  eval_micro_sequencer();   
  cycle_memory();
  eval_BUS_drivers();
  drive_BUS();
  latch_datapath_values();

  CURRENT_LATCHES = NEXT_LATCHES;

  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {                                      
    int i;

    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (CURRENT_LATCHES.PC == 0x0000) {
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {                                                     
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/ 
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {          
    int address; /* this is a byte address */

    printf("\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%.4x (%d) : 0x%.2x%.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%.4x (%d) : 0x%.2x%.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and BUS values to the     */   
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {                               
    int k; 

    printf("\nCurrent register/BUS values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%.4x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%.4x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%.4x\n", BUS);
    printf("MDR          : 0x%.4x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%.4x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/BUS values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%.4x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%.4x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%.4x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%.4x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */  
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {                         
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */ 
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {                 
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/************************************************************/
/*                                                          */
/* Procedure : init_memory                                  */
/*                                                          */
/* Purpose   : Zero out the memory array                    */
/*                                                          */
/************************************************************/
void init_memory() {                                           
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {                   
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */ 
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) { 
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {                              
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);

}

/***************************************************************/
/* Do not modify the above code.
   You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/


void eval_micro_sequencer() {

  /* 
   * Evaluate the address of the next state according to the 
   * micro sequencer logic. Latch the next microinstruction.
   */
    /*get microinstruction*/
    int* minst = CURRENT_LATCHES.MICROINSTRUCTION;
    /*get IRD bit*/
    if(GetIRD(minst) == 1) {
        /*use 0,0,[15,12]*/
        int irbits = (CURRENT_LATCHES.IR >> 12) & (0x0F);
        memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[0x00 | irbits], sizeof(int)*CONTROL_STORE_BITS);
        NEXT_LATCHES.STATE_NUMBER = 0x00 | irbits;
    } else {
        /*evaluate j bits*/
        /*check cond0 and cond1*/
        /*check branch, ready, addressmode*/
        int cond = GetCOND(minst);
        int jbits = GetJ(minst);
        if(cond == 0) {
            /*no checks*/
        } else if(cond == 1) {
            /*check ready bit*/
            if(CURRENT_LATCHES.READY == 1) {
                /*j1 = 1*/
                jbits = jbits | 0x02;
            }
        } else if(cond == 2) {
            /*check branch bit*/
            if(CURRENT_LATCHES.BEN == 1) {
                /*j2 = 1*/
                jbits = jbits | 0x04;
            }
        } else if(cond == 3) {
            /*check ir[11];*/
            int ir11 = (CURRENT_LATCHES.IR >> 11) & (0x01);
            if(ir11 == 1) {
                /*j0 = 1*/
                jbits = jbits | 0x01;
            }
        }
        memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[jbits], sizeof(int)*CONTROL_STORE_BITS);
        NEXT_LATCHES.STATE_NUMBER = jbits;
    }
}

int memen = 0;
int memV;


void cycle_memory() {
    memV = 0;
  /* 
   * This function emulates memory and the WE logic. 
   * Keep track of which cycle of MEMEN we are dealing with.  
   * If fourth, we need to latch Ready bit at the end of 
   * cycle to prepare microsequencer for the fifth cycle.  
   */
    int* mcinst = CURRENT_LATCHES.MICROINSTRUCTION;

    /*check mio.en*/
    if(GetMIO_EN(mcinst) == 1) {
        /*if mio.en check r/w*/
        if(CURRENT_LATCHES.READY == 1) {
            NEXT_LATCHES.READY = 0;
            memen = 0;
            /*ready to read/load data*/
            if(GetR_W(mcinst) == 0) {
                /*read -> set memV*/
                if(GetDATA_SIZE(mcinst) == 1) {
                    /*word*/
                    memV = Low16bits(MEMORY[CURRENT_LATCHES.MAR/2][0] | 
                        (MEMORY[CURRENT_LATCHES.MAR/2][1] << 8));
                } else { /*sign extend?*/
                    /*byte*/
                    if((CURRENT_LATCHES.MAR & 0x01) == 1) {
                        memV = Low16bits(MEMORY[CURRENT_LATCHES.MAR/2][1] << 8);
                    } else {
                        memV = Low16bits(MEMORY[CURRENT_LATCHES.MAR/2][0]);
                    }

                }
            } else {
                /*write*/
                if(GetDATA_SIZE(mcinst) == 1) {
                    /*word*/
                    MEMORY[CURRENT_LATCHES.MAR/2][1] = Low16bits((CURRENT_LATCHES.MDR >> 8) & 0x00FF);
                    MEMORY[CURRENT_LATCHES.MAR/2][0] = Low16bits(CURRENT_LATCHES.MDR & 0x00FF);
                
                } else { /*already sign extended?*/
                    /*byte*/
                    if((CURRENT_LATCHES.MAR & 0x01) == 1) {
                        MEMORY[CURRENT_LATCHES.MAR/2][1] = Low16bits((CURRENT_LATCHES.MDR >> 8) & 0x00FF);
                    } else {
                        MEMORY[CURRENT_LATCHES.MAR/2][0] = Low16bits(CURRENT_LATCHES.MDR & 0x00FF);
                    }

                }

            }
        } else {
            /*cycle memory cycle*/
            memen++;
            if(memen == 4) {
                memen = 0;
                NEXT_LATCHES.READY = 1;
            }
        }
        /*if start of memory access -> initialize to 0 else latch*/
        /*if read -> calculate address from data.size and mar, send to mdr*/
        /*if write -> calculate address from data.size and we right memories, check size, store to mar address*/


    }
}

int marmuxV;
int addrAdd;
int aluV;
int shfV;
int mdrV;
int pcV;
int sr1, sr2;

void eval_BUS_drivers() {
    marmuxV = addrAdd = aluV = shfV = mdrV = pcV = sr1 = sr2 = 0;
  /* 
   * Datapath routine emulating operations before driving the BUS.
   * Evaluate the input of tristate drivers 
   *             Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */
    int* mcinst = CURRENT_LATCHES.MICROINSTRUCTION;
    /*
    * PC
    */
    if(GetGATE_PC(mcinst) == 1) {
        pcV = CURRENT_LATCHES.PC;
    }

    /*
    * Register out evaluation
    */
    int sr1muxS = GetSR1MUX(mcinst);

    switch(sr1muxS) {
        case 0: {
            sr1 = Low16bits((CURRENT_LATCHES.IR >> 9) & 0x0007);
        } break;
        case 1: {
            sr1 = Low16bits((CURRENT_LATCHES.IR >> 6) & 0x0007);
        }
    }

    sr2 = Low16bits(CURRENT_LATCHES.IR & 0x0007);

    /*
    * MARMUX
    */
    /*addr2mux value*/
    int addr2mux, addr2muxS = GetADDR2MUX(mcinst);
    switch(addr2muxS) {
        case 3:  {
            /*[10:0]*/
            int sext = Low16bits((CURRENT_LATCHES.IR >> 10) & 0x0001);
            if(sext == 1) {
                addr2mux = Low16bits((CURRENT_LATCHES.IR & 0x07FF) | 0xF800);
            } else {
                addr2mux = Low16bits(CURRENT_LATCHES.IR & 0x07FF);
            }
        } break;
        case 2: {
            /*[8:0]*/
            int sext = Low16bits((CURRENT_LATCHES.IR >> 8) & 0x0001);
            if(sext == 1) {
                addr2mux = Low16bits((CURRENT_LATCHES.IR & 0x01FF) | 0xFE00);
            } else {
                addr2mux = Low16bits(CURRENT_LATCHES.IR & 0x01FF);
            }
        } break;
        case 1: {
            /*[5:0]*/
            int sext = Low16bits((CURRENT_LATCHES.IR >> 5) & 0x0001);
            if(sext == 1) {
                addr2mux = Low16bits((CURRENT_LATCHES.IR & 0x03F) | 0xFFC0);
            } else {
                addr2mux = Low16bits(CURRENT_LATCHES.IR & 0x003F);
            }
        } break;
        case 0: {
            addr2mux = Low16bits(0);
        }
    }
    if(GetLSHF1(mcinst) == 1) {
        addr2mux = Low16bits(addr2mux << 1);
    }
    
    /*addr1mux value*/
    int addr1mux, addr1muxS = GetADDR1MUX(mcinst);
    switch(addr1muxS) {
        case 1: {
            /*sr1out*/
            addr1mux = Low16bits(CURRENT_LATCHES.REGS[sr1]);
        } break;
        case 0: {
            /*pc no +2*/
            addr1mux = Low16bits(CURRENT_LATCHES.PC);
        }
    }

    /*add addr1 and addr2*/
    addrAdd = Low16bits(addr2mux + addr1mux);

    int marmuxS = GetMARMUX(mcinst);
    if(marmuxS == 0) {
        /*zext [7:0] leftsh1*/
        marmuxV = Low16bits((0x00FF & CURRENT_LATCHES.IR) << 1);
    } else {
        marmuxV = Low16bits(addrAdd);
    }

    /*
    * SR2MUX Output
    */
    int operand = (CURRENT_LATCHES.IR >> 5) & 0x0001;
    if(operand == 1) {
        /*immediate*/
        int sext = (CURRENT_LATCHES.IR >> 4) & 0x01;
        if(sext == 1) {
            operand = Low16bits((CURRENT_LATCHES.IR & 0x001F) | 0xFFE0);
        } else {
            operand = Low16bits(CURRENT_LATCHES.IR & 0x001F);
        }
    } else {
        operand = CURRENT_LATCHES.REGS[sr2];
    }

    /*
    * ALU
    */
    if(GetGATE_ALU(mcinst) == 1) {

        int aluS = GetALUK(mcinst);
        
        switch(aluS) {
            case 0: {
                /*add*/
                aluV = Low16bits(CURRENT_LATCHES.REGS[sr1] + operand);
            } break;
            case 1: {
                /*and*/
                aluV = Low16bits(CURRENT_LATCHES.REGS[sr1] & operand);
            } break;
            case 2: {
                /*xor*/
                aluV = Low16bits(CURRENT_LATCHES.REGS[sr1] ^ operand);
            } break;
            case 3: {
                /*passa*/
                aluV = Low16bits(CURRENT_LATCHES.REGS[sr1]);
            }
        }
    }

    /*
    * SHF
    */
    if(GetGATE_SHF(mcinst) == 1) {

        int shS = (CURRENT_LATCHES.IR >> 4) & 0x0003;
        int shA = (CURRENT_LATCHES.IR & 0x000F);

        switch(shS) {
            case 0: {
                /*lshf*/
                shfV = Low16bits(CURRENT_LATCHES.REGS[sr1] << shA);
            } break;
            case 1: {
                /*rshfl*/
                shfV = Low16bits(CURRENT_LATCHES.REGS[sr1] >> shA);
            } break;
            case 3: {
                /*rshfa*/
                int a, temp = CURRENT_LATCHES.REGS[sr1];
                for(a = 0; a < shA; a++) {
                    temp = Low16bits((CURRENT_LATCHES.REGS[sr1] & 0x8000) | (temp >> 1));
                }
                shfV = Low16bits(temp);
            }
        }
    }

    /*
    * MDR
    */
    if(GetGATE_MDR(mcinst) == 1) {
        int sexth = Low16bits((CURRENT_LATCHES.MDR >> 15) & 0x0001);
        int sextl = Low16bits((CURRENT_LATCHES.MDR >> 7) & 0x0001);

        if(GetDATA_SIZE(mcinst) == 1) {
            /*word access*/
            mdrV = Low16bits(CURRENT_LATCHES.MDR);
        } else {
            /*byte access*/
            if((CURRENT_LATCHES.MAR & 0x01) == 0) {
                /*low bits*/
                if(sextl == 1) {
                   mdrV = Low16bits((CURRENT_LATCHES.MDR & 0x00FF) | 0xFF00);
                } else {
                   mdrV = Low16bits(CURRENT_LATCHES.MDR & 0x00FF);
                }

            } else {
                /*high bits*/
                if(sexth == 1) {
                    mdrV = Low16bits(((CURRENT_LATCHES.MDR & 0xFF00) >> 8) | 0xFF00);
                } else {
                    mdrV = Low16bits((CURRENT_LATCHES.MDR & 0xFF00) >> 8);
                }
            }
        }
    }

}

void drive_BUS() {

  /* 
   * Datapath routine for driving the BUS from one of the 5 possible 
   * tristate drivers. 
   */
    int* mcinst = CURRENT_LATCHES.MICROINSTRUCTION;

    if(GetGATE_MARMUX(mcinst)) {

        /*assign BUS to marmux output*/
        BUS = Low16bits(marmuxV);

    } else if(GetGATE_PC(mcinst)) {

        /*assign BUS to pc*/
        BUS = Low16bits(CURRENT_LATCHES.PC);

    } else if(GetGATE_ALU(mcinst)) {

        /*assign BUS to alu output*/
        BUS = Low16bits(aluV);

    } else if(GetGATE_SHF(mcinst)) {

        /*assign BUS to shf output*/
        BUS = Low16bits(shfV);

    } else if(GetGATE_MDR(mcinst)) {

        /*assign BUS to mdr value*/
        BUS = Low16bits(mdrV);
    } else {
        BUS = 0;
    }

}


void latch_datapath_values() {

  /* 
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the BUS; therefore, this routine has to come 
   * after drive_BUS.
   */       
    /*registers, cc, pcmux, mar, mdr, IR, ben*/
    int* mcinst = CURRENT_LATCHES.MICROINSTRUCTION;
    /*
    * Registers
    */
    if(GetLD_REG(mcinst) == 1) {
        int drS = GetDRMUX(mcinst);
        int drV;

        if(drS == 0) {
            /*ir[11:9]*/
            drV = Low16bits((CURRENT_LATCHES.IR >> 9) & 0x07);
        } else {
            drV = 7;
        }
        NEXT_LATCHES.REGS[drV] = Low16bits(BUS);
    }

    /*
    * IR
    */
    if(GetLD_IR(mcinst) == 1) {
        NEXT_LATCHES.IR = Low16bits(BUS);
    }

    /*
    * PC
    */
    if(GetLD_PC(mcinst) == 1) {
        int pcmuxS = GetPCMUX(mcinst);
        switch(pcmuxS) {
            case 0: {
                /*+2*/
                NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC + 2);
            } break;
            case 2: {
                /*addr1/2 add*/ 
                NEXT_LATCHES.PC = Low16bits(addrAdd);
            } break;
            case 1: {
                /*from BUS*/
                NEXT_LATCHES.PC = Low16bits(BUS);
            }
        }
    }

    /*
    * CC
    */
    if(GetLD_CC(mcinst) == 1) {
        if((((BUS) >> 15) & 0x01) == 1) {
            NEXT_LATCHES.N = 1;
            NEXT_LATCHES.P = 0;
            NEXT_LATCHES.Z = 0;
        }
        if((((BUS) >> 15) & 0x01) == 0) {
            NEXT_LATCHES.N = 0;
            NEXT_LATCHES.P = 1;
            NEXT_LATCHES.Z = 0;
        }
        if(BUS == 0) {
            NEXT_LATCHES.N = 0;
            NEXT_LATCHES.P = 0;
            NEXT_LATCHES.Z = 1;
        }
    }

    /*
    * BEN
    */
    int p = (CURRENT_LATCHES.IR >> 9) & 0x01;
    int z = (CURRENT_LATCHES.IR >> 10) & 0x01;
    int n = (CURRENT_LATCHES.IR >> 11) & 0x01;
    NEXT_LATCHES.BEN = (CURRENT_LATCHES.N && n) || 
        (CURRENT_LATCHES.Z && z) || (CURRENT_LATCHES.P && p);

    /*if(GetLD_BEN(mcinst) == 1) {
        NEXT_LATCHES.BEN = (CURRENT_LATCHES.N && n) || 
            (CURRENT_LATCHES.Z && z) || (CURRENT_LATCHES.P && p);
    }*/

    /*
    * MDR
    */
    if(GetLD_MDR(mcinst) == 1) {
        if(GetMIO_EN(mcinst) == 1) {
            /*from memory*/
            NEXT_LATCHES.MDR = Low16bits(memV);
        } else {
            /*from BUS*/
            if(GetDATA_SIZE(mcinst) == 1) {
                /*word*/
                NEXT_LATCHES.MDR = Low16bits(BUS);
            } else {
                /*byte*/
                if((CURRENT_LATCHES.MAR & 0x01) == 1) {
                    /*high bits*/
                    /*shift BUS val 8 to the left*/
                    NEXT_LATCHES.MDR = Low16bits((BUS << 8) & 0xFF00);
                } else {
                    /*low bits*/
                    NEXT_LATCHES.MDR = Low16bits(BUS & 0x00FF);
                }
            }
        }
    }

    /*
    * MAR
    */
    if(GetLD_MAR(mcinst) == 1) {
        NEXT_LATCHES.MAR = Low16bits(BUS);
    }

}