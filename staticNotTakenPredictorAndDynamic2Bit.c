#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define CHUNK 1024 /* read 1024 bytes at a time */
#define LINES 64
#define LINES_ 128
#define PATH "/Users/diegogomestome/Documents/UFPR/ARQUITETURA/milc.txt"


struct btb {
    int pc;
    int valid;
    int cond;
};


struct btb2 {
    int pc;
    int valid;
    int cond;
    int bitCounter;
};

int get_opcode(char *assembly, char *opcode, unsigned long *address, unsigned long *size, unsigned *is_cond) {
    char buf[CHUNK];
    size_t nread;
    char *sub_string = NULL;
    char *tmp_ptr = NULL;

    static FILE *file = NULL;

    if (file == NULL) {
        file = fopen(PATH, "r");
        if (file == NULL) {
            printf("Could not open file.\n");
            exit(1);
        }
    }

    if (!fgets(buf, sizeof buf, file))
        return (0);

    // Check trace file
    int i = 0, count = 0;
    while (buf[i] != '\0') {
        count += (buf[i] == ';');
        i++;
    }
    if (count != 4) {
        printf("Error reading trace (Wrong  number of fields)\n");
        printf("%s", buf);
        exit(2);
    }

    //printf("%s\n", buf);

    // ASM
    sub_string = strtok_r(buf, ";", &tmp_ptr);
    strcpy(assembly, sub_string);

    // Operation
    sub_string = strtok_r(NULL, ";", &tmp_ptr);
    strcpy(opcode, sub_string);

    // Address
    sub_string = strtok_r(NULL, ";", &tmp_ptr);
    *address = strtoul(sub_string, NULL, 10);

    // Size
    sub_string = strtok_r(NULL, ";", &tmp_ptr);
    *size = strtoul(sub_string, NULL, 10);

    // Conditional
    sub_string = strtok_r(NULL, ";", &tmp_ptr);
    *is_cond = (sub_string[0] == 'C') ? 1 : 0;

    return 1;
}

unsigned int simulateTraceStatic() {

    char assembly[20], previous_assembly[20];
    char opcode[20], previous_opcode[20];
    unsigned long address, previous_address = 0;
    unsigned long size, previous_size = 0;
    unsigned is_cond, previous_cond = 0;
    unsigned int cycles = 0;

    struct btb table[64];
    int mask = 0, i = 0;

    while (get_opcode(assembly, opcode, &address, &size, &is_cond)) {
        if (previous_address == 0) {
            strcpy(previous_assembly, assembly);
            strcpy(previous_opcode, opcode);
            previous_address = address;
            previous_size = size;
            previous_cond = is_cond;
            get_opcode(assembly, opcode, &address, &size, &is_cond);
        }
        //Caso a instrução não seja branch incrementar ciclos + 1
        if (strcmp(previous_opcode, "OP_BRANCH") != 0) {
            cycles++;
            //Caso seja branch calcular index
        } else {
            mask = 0;
            for (i = 0; i < log2(LINES); i++) {
                mask |= 1 << i;
            }
            int index = (previous_address & mask);
            //Caso o index da btb der match com o address do branch executar BRANCH_HIT
            if (table[index].pc != 0 && table[index].pc == previous_address) {
                //Caso seja Incondicional cyclos ++
                //printf("BRANCH_HIT ");
                if (table[index].cond != 1) {
                    //printf(" INCONDITIONAL \n");
                    cycles++;
                } else {
                    //printf(" CONDITIONAL ");
                    if ((table[index].pc + previous_size) == address) {
                        //printf(" CORRECTED PREDICTED \n");
                        cycles++;
                    } else {
                        //printf(" MISS PREDICTION \n");
                        cycles += 4;
                    }
                }
                //Caso BRANCH_MISS
            } else {
                // printf("BTB MISS \n");
                cycles += 5;
                table[index].pc = previous_address;
                table[index].valid = 1;
                table[index].cond = previous_cond;
            }
        }
        strcpy(previous_assembly, assembly);
        strcpy(previous_opcode, opcode);
        previous_address = address;
        previous_size = size;
        previous_cond = is_cond;


    }
    return cycles;
}

unsigned int simulateTraceDynamic() {

    char assembly[20], previous_assembly[20];
    char opcode[20], previous_opcode[20];
    unsigned long address, previous_address = 0;
    unsigned long size, previous_size = 0;
    unsigned is_cond, previous_cond = 0;
    unsigned int cycles = 0;

    struct btb2 table[LINES_];
    int mask = 0, i = 0;

    while (get_opcode(assembly, opcode, &address, &size, &is_cond)) {
        if (previous_address == 0) {
            strcpy(previous_assembly, assembly);
            strcpy(previous_opcode, opcode);
            previous_address = address;
            previous_size = size;
            previous_cond = is_cond;
            get_opcode(assembly, opcode, &address, &size, &is_cond);
        }
        //Caso a instrução não seja branch incrementar ciclos + 1
        if (strcmp(previous_opcode, "OP_BRANCH") != 0) {
            cycles++;
            //Caso seja branch calcular index
        } else {
            mask = 0;
            for (i = 0; i < log2(LINES_); i++) {
                mask |= 1 << i;
            }
            int index = (previous_address & mask);
            //Caso o index da btb der match com o address do branch executar BRANCH_HIT
            if (table[index].pc != 0 && table[index].pc == previous_address) {
                //Caso seja Incondicional cyclos ++
                //printf("BRANCH_HIT ");
                if (table[index].cond != 1) {
                    //printf(" INCONDITIONAL \n");
                    cycles++;
                } else {
                    //printf(" CONDITIONAL ");

                    if ((table[index].pc + previous_size) == address) {
                        if (table[index].bitCounter < 2) {
                            if (table[index].bitCounter == 1) {
                                table[index].bitCounter--;
                            }
                            cycles++;
                        } else {
                            cycles += 4;
                            table[index].bitCounter--;
                        }
                    } else {
                        if (table[index].bitCounter < 2) {
                            table[index].bitCounter++;
                            cycles += 4;
                        } else {
                            cycles++;
                            if (table[index].bitCounter < 3) {
                                table[index].bitCounter++;
                            }
                        }

                    }
                }
                //Caso BRANCH_MISS
            } else {
                // printf("BTB MISS \n");
                cycles += 5;
                table[index].pc = previous_address;
                table[index].valid = 1;
                table[index].cond = previous_cond;
                table[index].bitCounter = 0;
            }
        }
        strcpy(previous_assembly, assembly);
        strcpy(previous_opcode, opcode);
        previous_address = address;
        previous_size = size;
        previous_cond = is_cond;


    }

    return cycles;
}

int main() {

    clock_t time_before;
    clock_t time_after;
    unsigned int cycles_result = 0;

    time_before = clock();
    cycles_result = simulateTraceStatic();
   // cycles_result = simulateTraceDynamic();
    time_after = clock();

    printf("Simulation Time  = %f \n", (double) (time_after - time_before) / CLOCKS_PER_SEC);
    printf("Number of cyclos = %d \n", cycles_result);



}



