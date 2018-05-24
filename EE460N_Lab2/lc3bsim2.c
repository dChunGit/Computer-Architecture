/*
        Name 1:  David Chun
        Name 2:  Chioma Okorie
        UTEID 1: dc37875
        UTEID 2: coo279
*/

/***************************************************************/
/*                                                             */
/*   LC-3b Instruction Level Simulator                         */
/*                                                             */
/*   EE 460N                                                   */
/*   The University of Texas at Austin                         */
/*                                                             */
/***************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************/
/*                                                             */
/* Files: isaprogram   LC-3b machine language program file     */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void process_instruction();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
     MEMORY[A][1] stores the most significant byte of word at word address A 
*/

#define WORDS_IN_MEM    0x08000 
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;  /* run bit */


typedef struct System_Latches_Struct{

    int PC,   /* program counter */
        N,    /* n condition bit */
        Z,    /* z condition bit */
        P;    /* p condition bit */
    int REGS[LC_3b_REGS]; /* register file. */
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int INSTRUCTION_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands                    */
/*                                                             */
/***************************************************************/
void help() {                                                    
    printf("----------------LC-3b ISIM Help-----------------------\n");
    printf("go               -  run program to completion         \n");
    printf("run n            -  execute program for n instructions\n");
    printf("mdump low high   -  dump memory from low to high      \n");
    printf("rdump            -  dump the register & bus values    \n");
    printf("?                -  display this help menu            \n");
    printf("quit             -  exit the program                  \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                

    process_instruction();
    CURRENT_LATCHES = NEXT_LATCHES;
    INSTRUCTION_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles                 */
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
/* Purpose   : Simulate the LC-3b until HALTed                 */
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
/* Purpose   : Dump current register and bus values to the     */   
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {                               
    int k; 

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Instruction Count : %d\n", INSTRUCTION_COUNT);
    printf("PC                : 0x%.4x\n", CURRENT_LATCHES.PC);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
        printf("%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Instruction Count : %d\n", INSTRUCTION_COUNT);
    fprintf(dumpsim_file, "PC                : 0x%.4x\n", CURRENT_LATCHES.PC);
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
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
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

/************************************************************/
/*                                                          */
/* Procedure : initialize                                   */
/*                                                          */
/* Purpose   : Load machine language program                */ 
/*             and set up initial state of the machine.     */
/*                                                          */
/************************************************************/
void initialize(char *program_filename, int num_prog_files) { 
    int i;

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
        load_program(program_filename);
        while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;  
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
    if (argc < 2) {
        printf("Error: usage: %s <program_file_1> <program_file_2> ...\n",
                     argv[0]);
        exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argc - 1);

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

     MEMORY

     CURRENT_LATCHES
     NEXT_LATCHES

     You may define your own local/global variables and functions.
     You may use the functions to get at the control bits defined
     above.

     Begin your code here                  */

/***************************************************************/


int currentInst; int instrArray[16] ={0};
void process_instruction(){
    /*  function: process_instruction
     *  
     *    Process one instruction at a time  
     *       -Fetch one instruction
     *       -Decode 
     *       -Execute
     *       -Update NEXT_LATCHES
     */     
     
     void br_instruction(); void add_instruction();void ldb_instruction();
     void stb_instruction();void jsr_instruction();void and_instruction();
     void ldw_instruction();void stw_instruction(); void xor_instruction();
     void jmp_instruction();void shf_instruction();  void lea_instruction();
     void trap_instruction();
     
     
     int lowerByteInst = MEMORY[CURRENT_LATCHES.PC>>1][0]; 
     int upperByteInst = MEMORY[CURRENT_LATCHES.PC>>1][1];
     int opcode= (upperByteInst >> 4); 
     currentInst = (upperByteInst <<8)|lowerByteInst;
     int tempInstr = currentInst;
     int ind;

     for(ind =0; ind <16; ind++){
        instrArray[ind] = currentInst & 0x01;
        currentInst >>=1;
     }

     currentInst = tempInstr;
     switch(opcode){
        case 0:  br_instruction();   break;
        case 1:  add_instruction();  break;
        case 2:  ldb_instruction();  break;
        case 3:  stb_instruction();  break;
        case 4:  jsr_instruction();  break;  /*handle jsr and jsrr instruction*/
        case 5:  and_instruction();  break; 
        case 6:  ldw_instruction();  break;
        case 7:  stw_instruction();  break;
        case 9:  xor_instruction();  break;  /*handle not and xor instruction*/
        case 12: jmp_instruction();  break;  /*handle jmp and ret instruction*/
        case 13: shf_instruction();  break;
        case 14: lea_instruction();  break;
        case 15: trap_instruction(); break;
        default: break;
    }
}


void setCC(int DR){

    if((NEXT_LATCHES.REGS[DR] >> 15) == 1){
        NEXT_LATCHES.N = 1; NEXT_LATCHES.Z =0; NEXT_LATCHES.P =0;
    }

    else if (NEXT_LATCHES.REGS[DR]==0)
    {
     NEXT_LATCHES.N =0; NEXT_LATCHES.Z = 1;  NEXT_LATCHES.P =0;
    }

    else{
     NEXT_LATCHES.N =0; NEXT_LATCHES.Z =0; NEXT_LATCHES.P = 1;
    }

}

void add_instruction(){
 int DR, SR1, OP2;

 DR = (instrArray[11] <<2)| (instrArray[10]<<1)| instrArray[9];
 SR1 = (instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6];

 if(instrArray[5] ==0){
        OP2 = (currentInst & 0x0007);
        NEXT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[SR1]+CURRENT_LATCHES.REGS[OP2]);
    }

    else{ /*5-bit immediate*/
        OP2 = (currentInst& 0x001F);
        if(instrArray[4] == 1)
            OP2 = OP2 | 0xFFE0;
        NEXT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[SR1]+ OP2);
    }

    setCC(DR);
    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);

}

void and_instruction(){
    int DR, SR1, OP2;
 
 DR = (instrArray[11] <<2)| (instrArray[10]<<1)| instrArray[9];
 SR1 = (instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6];

 if(instrArray[5] ==0){
        OP2 = (currentInst & 0x0007);
        NEXT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[SR1]&CURRENT_LATCHES.REGS[OP2]);
    }
    else{ /*5-bit immediate*/
        OP2 = (currentInst& 0x001F);
        if(instrArray[4] == 1)
            OP2 = OP2 | 0xFFE0;
        NEXT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[SR1]& OP2);
    }

    setCC(DR);
    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);

}

void br_instruction(){
    int PCoffset9 = 0;

    if((instrArray[11]&&CURRENT_LATCHES.N)||(instrArray[10]&&CURRENT_LATCHES.Z)||(instrArray[9]&&CURRENT_LATCHES.P)){
        PCoffset9 = (currentInst&0x01FF);

        if(instrArray[8]==1)
            PCoffset9 |= 0xFE00;
        PCoffset9 = PCoffset9 << 1;
        NEXT_LATCHES.PC = Low16bits((CURRENT_LATCHES.PC +2)+PCoffset9);
    } else {

        NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);
    }

}

void jmp_instruction(){
    int BaseR;

    BaseR = ((instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6]);
    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.REGS[BaseR]);

}

void jsr_instruction() {
	int BaseR, PCoffset11, tempR;

	tempR = (CURRENT_LATCHES.PC + 2);
	if (instrArray[11] == 0) { /*jsrr*/
		BaseR = ((instrArray[8] << 2) | (instrArray[7] << 1) | instrArray[6]);
		NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.REGS[BaseR]);
	}

	else {/*jsr*/
		PCoffset11 = (currentInst & 0x07FF);
		if (instrArray[10] == 1)
			PCoffset11 |= 0xF800;
		PCoffset11 <<= 1;
		NEXT_LATCHES.PC = Low16bits((CURRENT_LATCHES.PC + 2) + PCoffset11);
	}

	NEXT_LATCHES.REGS[7] = Low16bits(tempR);
}

void ldb_instruction(){
    int DR, BaseR, boffset6, tempR;

    DR = (instrArray[11]<<2)|(instrArray[10]<<1)|instrArray[9];
    BaseR = (instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6];
    boffset6 = currentInst & 0x003F;

    if(instrArray[5] == 1)
        boffset6 = boffset6 | 0xFFC0;

    /*  no need to sign extend the output before storing in dr*/
    tempR = Low16bits(CURRENT_LATCHES.REGS[BaseR] + boffset6);
    NEXT_LATCHES.REGS[DR] = Low16bits(MEMORY[tempR/2][tempR%2]);

    setCC(DR);
    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);
}

void ldw_instruction(){
    int DR, BaseR, boffset6, tempR;

    DR = (instrArray[11]<<2)|(instrArray[10]<<1)|instrArray[9];
    BaseR = (instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6];
    boffset6 = currentInst & 0x003F;

    if(instrArray[5] == 1)
         boffset6 = boffset6 | 0xFFC0;
    tempR = CURRENT_LATCHES.REGS[BaseR]+(boffset6 << 1);
    NEXT_LATCHES.REGS[DR] = Low16bits(MEMORY[tempR/2][0] | (MEMORY[tempR/2][1] <<8));

    setCC(DR);
    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);

}

void lea_instruction(){
    int DR, PCoffset9;

    DR = (instrArray[11]<<2)|(instrArray[10]<<1)|instrArray[9];
    PCoffset9 = currentInst & 0x01FF;

    if(instrArray[8] == 1)
        PCoffset9 |= 0xFE00;
    NEXT_LATCHES.REGS[DR] = Low16bits((CURRENT_LATCHES.PC +2) + (PCoffset9 << 1));

    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);
}

void xor_instruction(){
    int DR, SR1, OP2;
    DR = (instrArray[11] <<2)| (instrArray[10]<<1)| instrArray[9];
    SR1 = (instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6];
    if(instrArray[5]==0){
        OP2 = currentInst & 0x0007;
        NEXT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[SR1] ^ CURRENT_LATCHES.REGS[OP2]);
    }
    else{
        OP2 = (currentInst& 0x001F);
        if(instrArray[4] == 1)
            OP2 = OP2 | 0xFFE0;
        NEXT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[SR1]^ OP2);
    }
    setCC(DR);
    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);
}

void shf_instruction(){
    int DR, SR, amount4;
    DR = (instrArray[11] <<2)| (instrArray[10]<<1)| instrArray[9];
    SR = (instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6];
    amount4 = currentInst &0x000F;

    if(instrArray[4] == 0) /*LSHFL*/
        NEXT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[SR]<<amount4);
    
    else{
        if(instrArray[5]==0) /*RSHFL*/ {
            NEXT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[SR] >> amount4);
        }
        else{ /*RSHFA*/
            int i;
            for(i=0;i<amount4;i++) {
                NEXT_LATCHES.REGS[DR] = Low16bits((CURRENT_LATCHES.REGS[SR] >> 1) | (CURRENT_LATCHES.REGS[SR]&0x8000));
            }
        }
    }

    setCC(DR);
    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);
}

void stb_instruction(){
    int SR, BaseR, boffset6, tempR;
    SR = (instrArray[11]<<2)|(instrArray[10]<<1)|instrArray[9];
    BaseR = (instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6];
    boffset6 = currentInst & 0x003F;

    if(instrArray[5] == 1)
        boffset6 = boffset6 | 0xFFC0;

    tempR = Low16bits(CURRENT_LATCHES.REGS[BaseR] + boffset6);
    MEMORY[tempR/2][tempR%2] = CURRENT_LATCHES.REGS[SR]&0x00FF;

    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);
}

void stw_instruction(){
    int SR, BaseR, boffset6, tempR;
    SR = (instrArray[11]<<2)|(instrArray[10]<<1)|instrArray[9];
    BaseR = (instrArray[8]<<2)|(instrArray[7]<<1)|instrArray[6];
    boffset6 = currentInst & 0x003F;

    if(instrArray[5] == 1)
         boffset6 = boffset6 | 0xFFC0;

    boffset6  = boffset6 << 1;
    tempR = Low16bits(CURRENT_LATCHES.REGS[BaseR] + boffset6);
    MEMORY[tempR/2][0] = (CURRENT_LATCHES.REGS[SR]&0x00FF);
    MEMORY[tempR/2][1] = (CURRENT_LATCHES.REGS[SR] >>8);

    NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC +2);

}

void trap_instruction(){
    int trapvect8;
    NEXT_LATCHES.REGS[7] = Low16bits(CURRENT_LATCHES.PC+2);
    trapvect8 = Low16bits((currentInst & 0x00FF) << 1);
    NEXT_LATCHES.PC = Low16bits(MEMORY[trapvect8/2][0]|(MEMORY[trapvect8/2][1]<<8));
}
