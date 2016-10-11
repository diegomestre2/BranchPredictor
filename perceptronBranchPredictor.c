//
// Created by Diego Gomes Tome on 20/09/16.
//teste
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define CHUNK 1024 /* read 1024 bytes at a time */
#define NUM_PERCEPTRONS 512
#define NUM_WEIGHTS 28 // Definido de forma empirica 31
#define HISTORY_LENGTH 27 // Definido de forma empirica
#define LINES_BTB 512 // Definido de forma empirica 512


#define PATH "/Users/diegogomestome/Documents/UFPR/ARQUITETURA/"

signed int perceptrons[NUM_PERCEPTRONS][NUM_WEIGHTS] = {1};
signed int branchHistory[HISTORY_LENGTH] = {1};
signed THRESHOLD = ((1.93 * HISTORY_LENGTH) + 14);

struct btb {
    unsigned long pc;
    unsigned int validBit;
    unsigned int conditionalBit;
};

void printResults(int directionMiss, int btbHit, int btbMiss, int cycles_result) {
    double miss_rate = 100.00 * directionMiss / btbHit;
    printf("Miss rate        = %.2f %% \n", miss_rate);
    printf("Direction Hit    = %d  \n", btbHit - directionMiss);
    printf("Direction Miss   = %d  \n", directionMiss);
    printf("BTB hit          = %d  \n", btbHit);
    printf("BTB miss         = %d  \n", btbMiss);
    printf("Number of cyclos = %d \n", cycles_result);
}

int
get_opcode(char *assembly, char *opcode, unsigned long *address, unsigned long *size, unsigned *is_cond, char *path) {
    char buf[CHUNK];
    char *sub_string = NULL;
    char *tmp_ptr = NULL;

    static FILE *file = NULL;

    if (file == NULL) {
        file = fopen(path, "r");
        if (file == NULL) {
            printf("Could not open file.\n");
            exit(1);
        }
    }

    if (!fgets(buf, sizeof buf, file)) {
        fclose(file);
        file = NULL;
        return (0);
    }


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

void predict(unsigned long previous_address, int *yOut, int *index) {
    //calculate index perceptrons Matrix
    int mask = 0, i;
    for (i = 0; i < log2(NUM_PERCEPTRONS); i++) {
        mask |= (1 << i);
    }
    (*index) = (previous_address & mask);

    (*yOut) = perceptrons[(*index)][0];
    for (i = 1; i < HISTORY_LENGTH; i++) {
        (*yOut) = (*yOut) + (branchHistory[i-1] * perceptrons[(*index)][i]);
    }
}

void trainPerceptor(int prediction, int outcome, int yOut, int index) {
    int j;

    if (prediction != outcome || abs(yOut) <= THRESHOLD) {
        perceptrons[index][0]+= outcome;
        for (j = 1; j < HISTORY_LENGTH; j++) {
            perceptrons[index][j] += (outcome * branchHistory[j-1]);
        }
    }
    //update branchHistory
    for (index = 0; index < HISTORY_LENGTH - 1; ++index)
        branchHistory[index] = branchHistory[index + 1];

    branchHistory[HISTORY_LENGTH - 1] = outcome;
}


int getBTBIndex(unsigned long previous_address) {
    int mask = 0, i;
    for (i = 0; i < log2(LINES_BTB); i++) {
        mask |= (1 << i);
    }
    return (previous_address & mask);
}

void simulateTrace(char *path) {

    char assembly[20], previous_assembly[20], opcode[20], previous_opcode[20];
    unsigned long address, previous_address = 0, size, previous_size = 0;
    unsigned int is_cond, previous_cond = 0, cycles = 0;
    int prediction, outcome, mask = 0, i = 0, index = 0, yOut = 0, directionMiss = 0, btbHit = 0, btbMiss = 0;
    struct btb table[LINES_BTB];

    //Leitura do traço
    while (get_opcode(assembly, opcode, &address, &size, &is_cond, path)) {
        if (previous_address == 0) {
            strcpy(previous_assembly, assembly);
            strcpy(previous_opcode, opcode);
            previous_address = address;
            previous_size = size;
            previous_cond = is_cond;
            get_opcode(assembly, opcode, &address, &size, &is_cond, path);
        }
        //Caso a instrução não seja branch incrementar ciclos
        if (strcmp(previous_opcode, "OP_BRANCH") != 0) {
            cycles++;
        } else {
            //Caso seja branch calcular index
            index = getBTBIndex(previous_address);
            //Caso o index da btb der match com o address do branch e bit de validade setado, executar BRANCH_HIT
            if (table[index].pc != 0 && table[index].pc == previous_address) {
                btbHit++;
                //Caso seja Incondicional cyclos ++
                //BRANCH_HIT
                if (table[index].conditionalBit != 1) {
                    //INCONDITIONAL
                    cycles++;
                } else {

                    //CONDITIONAL
                    //index = getPerceptronIndex(previous_address);
                    //calculate prediction
                    predict(previous_address, &yOut, &index);

                    //prediction analisys
                    if ((previous_address + previous_size) == address) {
                        if (yOut < 0) {
                            prediction = -1;
                            outcome = -1;
                            cycles++;
                        } else {
                            prediction = 1;
                            outcome = -1;
                            cycles += 4;
                            directionMiss++;
                        }
                    } else {
                        if (yOut < 0) {
                            prediction = -1;
                            outcome = 1;
                            cycles += 4;
                            directionMiss++;
                        } else {
                            prediction = 1;
                            outcome = 1;
                            cycles++;
                        }
                    }
                    // train perceptors
                    trainPerceptor(prediction, outcome, yOut, index);

                }
                //Caso BRANCH MISS
            } else {
                btbMiss++;
                cycles += 5;
                table[index].pc = previous_address;
                table[index].validBit = 1;
                table[index].conditionalBit = previous_cond;
            }
        }
        strcpy(previous_assembly, assembly);
        strcpy(previous_opcode, opcode);
        previous_address = address;
        previous_size = size;
        previous_cond = is_cond;


    }
    printResults(directionMiss, btbHit, btbMiss, cycles);
}


int main() {

    clock_t time_before;
    clock_t time_after;
    int i;
    char path[200] = PATH;
    char *files[] = {"astar.txt", "bzip2.txt", "gcc.txt", "gobmk.txt", "h264ref.txt", "hmmer.txt", "libquantum.txt",
                     "mcf.txt", "omnetpp.txt", "perlbench.txt",
                     "gamess.txt", "lbm.txt", "milc.txt", "is.txt"};

    for (i = 0; i < 14; i++) {
        printf("Simulation for: %s \n", files[i]);
        time_before = clock();
        simulateTrace(strcat(path, files[i]));
        time_after = clock();
        strcpy(path, PATH);
        printf("Simulation Time  = %.2f seconds\n\n", (double) (time_after - time_before) / CLOCKS_PER_SEC);;
    }
    return 0;

}


