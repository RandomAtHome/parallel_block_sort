#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include "mpi.h"
/*
We are going to use random at first, for not uniform distribution,
but then try out mrand48
*/

enum DataType {Number, Stop};

const char INPUTFILE[] = "default.in";
const char RESULT_FILENAME[] = "sort_results.tsv";
const size_t REALLOC_STEP = 10000;

void safe_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

double log_time(FILE *file, double last_time) {
    double new_time = MPI_Wtime();
    if (file) fprintf(file, "%lf\t", new_time - last_time);
    return new_time;
}

void log_uint(FILE *file, unsigned int a) {
    if (file) fprintf(file, "%u\t", a);
}

void finish_log_line(FILE *file) {
    if (file) {
        fprintf(file, "\n");
        fclose(file);
    }
}

int comparator(const void * a, const void * b) {
    if (*(unsigned long*)a == *(unsigned long*)b) return 0;
    return (*(unsigned long*)a > *(unsigned long*)b) ? 1 : -1;
}

int is_sorted(unsigned long* buffer, size_t count) {
    /*true (1) if good, false (0) else*/
    while (--count) {
        if (buffer[count] < buffer[count - 1]) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char * argv[]){
    int NET_SIZE, RANK;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);
    MPI_Comm_size(MPI_COMM_WORLD, &NET_SIZE);
    const double START_TIME = MPI_Wtime();
    double last_time = START_TIME;
    const char* config_filename = argv[1] ? argv[1] : INPUTFILE;
    FILE *log_file = NULL;
    if (RANK == 0) {
        log_file = fopen(RESULT_FILENAME, "a");
    }
    FILE *config_file = fopen(config_filename, "r");
    unsigned int rseed = 0, bits_to_use, NUMBER_COUNT;
    fscanf(config_file, "%u%u%u", &NUMBER_COUNT, &rseed, &bits_to_use);
    fclose(config_file);
    if (!rseed) {
        rseed = time(NULL);
    }
    if (bits_to_use > CHAR_BIT*sizeof(long) || bits_to_use == 0) {
        if (RANK == 0) {
            fprintf(stderr, "Can't use %u bits, using %zu\n", bits_to_use, CHAR_BIT*sizeof(long)/2);
        }
        bits_to_use = CHAR_BIT*sizeof(long)/2;
    }
    log_uint(log_file, (unsigned int)NET_SIZE);
    log_uint(log_file, NUMBER_COUNT);
    log_uint(log_file, rseed);
    log_uint(log_file, bits_to_use);
    unsigned int offset = CHAR_BIT*sizeof(long) - bits_to_use;
    unsigned long block_size = -1;
    if (bits_to_use == CHAR_BIT*sizeof(long)) {
        block_size = (1ull << (bits_to_use - 1)) / (NET_SIZE >> 1); // hack to avoid going over range 
    } else {
        block_size = (1ull << bits_to_use) / NET_SIZE; //this will be needed when we send all this to different proccesses
    }
    size_t allocated_count = NUMBER_COUNT / (RANK ? NET_SIZE : 1);
    unsigned long *int_storage = malloc(allocated_count * sizeof(unsigned long)); // surprise ternary inside
    int *recvcounts = RANK ? NULL : calloc(NET_SIZE, sizeof(int));
    int *displs = RANK ? NULL : calloc(NET_SIZE, sizeof(int));
    size_t received_numbers_count = 0;
    if (RANK == 0) {
        printf("[%lf]\tStarted generating numbers... %u total to generate, Rseed = %u, bits used = %u\n", MPI_Wtime() - START_TIME, NUMBER_COUNT, rseed, bits_to_use);
        last_time = log_time(log_file, last_time);
        srandom(rseed);
        unsigned int numbers_left = NUMBER_COUNT;
        while (numbers_left--) {
            unsigned long new_number = random();
            new_number >>= offset; // this is to impose some kind of range for generated numbers
            int receiver = new_number / block_size;
            recvcounts[receiver]++;
            if (receiver == 0) {
                int_storage[received_numbers_count++] = new_number;
            } else {
                MPI_Send(&new_number, 1, MPI_UNSIGNED_LONG, receiver, Number, MPI_COMM_WORLD);
            }
        }
        printf("[%lf]\tAll numbers generated! Sending start signal\n", MPI_Wtime() - START_TIME);
        last_time = log_time(log_file, last_time);
        for (int receiver = 1; receiver < NET_SIZE; receiver++) {
            MPI_Send(&numbers_left, 1, MPI_UNSIGNED_LONG, receiver, Stop, MPI_COMM_WORLD);
        }

    } else {
        unsigned long received_number;
        MPI_Status status;
        MPI_Recv(&received_number, 1, MPI_UNSIGNED_LONG, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // we use Tag to differ types of received message
        while (status.MPI_TAG != Stop) {
            if (received_numbers_count == allocated_count) {
                unsigned long * temp = realloc(int_storage, (allocated_count += REALLOC_STEP) * sizeof(unsigned long));
                if (!temp) {
                    fprintf(stderr, "Out of memory on %d, cur element count %zu\n", RANK, received_numbers_count);
                } else {
                    int_storage = temp;
                }
            }
            int_storage[received_numbers_count++] = received_number;
            MPI_Recv(&received_number, 1, MPI_UNSIGNED_LONG, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (RANK == 0) printf("[%lf]\tSent all start signals, began sort\n", MPI_Wtime() - START_TIME);
    last_time = log_time(log_file, last_time);
    qsort(int_storage, received_numbers_count, sizeof(unsigned long), comparator); //FIXME: change to typeof()?
    // lower common part
    if (RANK == 0) {
        for (int i = 1; i < NET_SIZE; i++) {
            displs[i] = displs[i-1] + recvcounts[i-1];
        }
    }
    MPI_Gatherv(int_storage, received_numbers_count, MPI_UNSIGNED_LONG, int_storage, recvcounts, displs, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    last_time = log_time(log_file, last_time);
    if (RANK == 0) {
        printf("[%lf]\tGathered all numbers from all proccesses! Start the check of final array...\t", MPI_Wtime() - START_TIME);
        if (is_sorted(int_storage, NUMBER_COUNT)) {
            printf("Result array is sorted!\n");
        } else {
            printf("The result is wrong\n");
        }
        printf("[%lf]\tFinished! Cleaning up...\n", MPI_Wtime() - START_TIME);
    }
    log_time(log_file, last_time);
    safe_free(int_storage);
    safe_free(recvcounts);
    safe_free(displs);
    finish_log_line(log_file);
    MPI_Finalize();
    return 0;
}