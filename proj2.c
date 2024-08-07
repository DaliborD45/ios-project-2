/***************************
 * @file proj2.c
 * @brief Main file for proj2
 * @details
 * @author Dalibor Detko
***************************/

#include "proj2.h"
#include <stdarg.h>

/* SHARED MEMORY */
int *numberOfCodeLines;
int *currentBusStop;
int *numberOfGoneSkiers;
int *numberOfPeopleOnEachBusStop;
int *numberOfBoardedPeople;
int *boardedPeople;
/* SEMAPHORES */
sem_t *mutex = NULL;
sem_t *bus = NULL;
sem_t *boarded = NULL;
sem_t *printing = NULL;
/*FILE*/
FILE *f;


/*Structure that stores all my parameters*/
typedef struct {
    int numberOfSkiers;
    int numberOfBusStops;
    int busCapacity;
    int maxSkierWaitTime;
    int maxBusDriveTime;
} ARGUMENTS_T;

ARGUMENTS_T Arguments;

/*Helper function that prints to file and flushes it*/
void fprintf_flush(FILE *stream, const char *format, ...) {
    sem_wait(printing);
    (*numberOfCodeLines)++;
    va_list args;
    va_start(args, format);
    fprintf(stream, "%d: ", *numberOfCodeLines);
    vfprintf(stream, format, args);
    va_end(args);
    fflush(stream);
    sem_post(printing);
}

/*
 * @brief function that initializes semaphores and allocated shared memory
 */

void init_semaphores(void) {
    currentBusStop = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    numberOfGoneSkiers = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    numberOfPeopleOnEachBusStop = (int *) mmap(NULL, sizeof(int) * Arguments.numberOfBusStops, PROT_READ | PROT_WRITE,
                                               MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    numberOfBoardedPeople = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    boardedPeople = (int *) mmap(NULL, sizeof(int) * Arguments.busCapacity, PROT_READ | PROT_WRITE,
                                 MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    numberOfCodeLines = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if (currentBusStop == MAP_FAILED || numberOfGoneSkiers == MAP_FAILED) {
        fprintf(stderr, "Could not initialize shared memory\n");
        exit(1);
    }
    /* assigns initial values to shared memory */

    for (int i = 0; i < Arguments.numberOfBusStops + 1; i++) {
        numberOfPeopleOnEachBusStop[i] = 0;
    }

    *currentBusStop = 1;
    *numberOfGoneSkiers = 0;
    *numberOfCodeLines = 0;
    *numberOfBoardedPeople = 0;
    mutex = sem_open(MAIN_PROCESS_SEM, O_CREAT, 0666, 1);
    bus = sem_open(BUS_SEM, O_CREAT, 0666, 0);
    boarded = sem_open(BOARDING_SEM, O_CREAT, 0666, 0);
    printing = sem_open(PRINTING_SEM, O_CREAT, 0666, 1);

    if (mutex == SEM_FAILED || bus == SEM_FAILED || boarded == SEM_FAILED) {
        fprintf(stderr, "Could not initialize semaphores\n");
        exit(1);
    }

}
/*
 * @brief Close and unlink semaphores
 */

void cleanup(void) {
    munmap(currentBusStop, sizeof(int));
    munmap(numberOfGoneSkiers, sizeof(int));
    munmap(numberOfPeopleOnEachBusStop, sizeof(int) * Arguments.numberOfBusStops);
    munmap(numberOfBoardedPeople, sizeof(int));
    munmap(boardedPeople, sizeof(int) * Arguments.busCapacity);
    munmap(numberOfCodeLines, sizeof(int));


    sem_close(mutex);
    sem_close(bus);
    sem_close(boarded);
    sem_close(printing);


    sem_unlink(MAIN_PROCESS_SEM);
    sem_unlink(BUS_SEM);
    sem_unlink(BOARDING_SEM);
    sem_unlink(PRINTING_SEM);
    fclose(f);
}

/*
 * @brief Function that generates random number between min and max
 * @param min minimum number
 * @param max maximum number
 */
void randusleep(int min, int max) {
    usleep(rand() % (max - min + 1) + min);
}

/*
 * @brief Function that validates input arguments
 * @param numberOfArguments number of arguments
 * @param arguments array of arguments
 */
void validateInput(int numberOfArguments, char *arguments[]) {
    if (numberOfArguments != 6) {
        fprintf(stderr, "you have provided wrong number of arguments\n");
        exit(1);
    }
    for (int i = 1; i < numberOfArguments; i++) {
        if (!isdigit(*arguments[i])) {
            fprintf(stderr, "argument %c is in wrong format\n", *arguments[i]);
            exit(1);
        }
    }

    Arguments.numberOfSkiers = atoi(arguments[1]);
    Arguments.numberOfBusStops = atoi(arguments[2]);
    Arguments.busCapacity = atoi(arguments[3]);
    Arguments.maxSkierWaitTime = atoi(arguments[4]);
    Arguments.maxBusDriveTime = atoi(arguments[5]);

    if (Arguments.numberOfSkiers <= 0 || Arguments.numberOfSkiers >= 20000) {
        fprintf(stderr, "the maximum number of skiers is 20000");
        exit(1);
    }
    if (Arguments.numberOfBusStops <= 0 || Arguments.numberOfBusStops > 10) {
        fprintf(stderr, "the number of bus stops ranges between 0 and 10");
        exit(1);
    }
    if (Arguments.busCapacity < 10 || Arguments.busCapacity > 100) {
        fprintf(stderr, "the number of bus capacity ranges between 10 and 100");
        exit(1);
    }
    if (Arguments.maxSkierWaitTime < 0 || Arguments.maxSkierWaitTime > 10000) {
        fprintf(stderr, "the number of maximum skier wait time ranges between 0 and 10000");
        exit(1);
    }

    if (Arguments.maxBusDriveTime < 0 || Arguments.maxBusDriveTime > 1000) {
        fprintf(stderr, "the number of maximum time bus can drive between two stops rangese between 0 and 1000");
        exit(1);
    }

}
/*
 * @brief This function creates a buss process and handles the bus arrival and departure
 * it also handles the skiers departure to the ski
 */

void process_bus(void) {
    fprintf_flush(f, "BUS: started\n");
    while (*numberOfGoneSkiers != Arguments.numberOfSkiers) {
        int waitTime = rand() % Arguments.maxBusDriveTime;
        randusleep(0, waitTime);;
        sem_wait(mutex);
        fprintf_flush(f, "BUS: arrived to %d\n", *currentBusStop);
        int waiting = 0;
        for (int i = 0; i < numberOfPeopleOnEachBusStop[*currentBusStop]; i++) {
            sem_post(bus);
            sem_wait(boarded);

            waiting++;
        }

        numberOfPeopleOnEachBusStop[*currentBusStop] -= waiting;
        fflush(stdout);
        sem_post(mutex);
        fprintf_flush(f, "BUS: leaving %d\n", *currentBusStop);
        *currentBusStop += 1;
        if (*currentBusStop > Arguments.numberOfBusStops) {
            fprintf_flush(f,"BUS: arrived to final\n");
            for (int i = 0; i < *numberOfBoardedPeople; i++) {
                fprintf_flush(f, "L %d: going to ski\n", boardedPeople[i]);
                (*numberOfGoneSkiers) += 1;

            }
            fprintf_flush(f, "BUS: leaving final\n");
            (*numberOfBoardedPeople) = 0;
            *currentBusStop = 1;
        }
    }
    fprintf_flush(f, "BUS: finish\n");
    exit(0);

}


void process_skier(int skierID) {
    sem_wait(mutex);
    int generatedSkierBusStop = (rand() % ((Arguments.numberOfBusStops))) + 1;
    fprintf_flush(f, "L %d: arrived to %d\n", skierID, generatedSkierBusStop);
    numberOfPeopleOnEachBusStop[generatedSkierBusStop] += 1;
    sem_post(mutex);

    sem_wait(bus);
    if (generatedSkierBusStop == *currentBusStop && (*currentBusStop) <= Arguments.busCapacity) {
        fprintf_flush(f, "L %d: boarding\n", skierID);
        boardedPeople[*numberOfBoardedPeople] = skierID;
        *numberOfBoardedPeople += 1;
    }
    sem_post(boarded);

}

/*
 * @brief This function generates skier processes, sleep function ensures that the skiers are generated in random order
 */

void generateSkiers(void) {
    for (int i = 1; i <= Arguments.numberOfSkiers; i++) {
        pid_t SKIER = fork();
        if (SKIER == 0) {
            fprintf_flush(f, "L %d: started\n", i);
            process_skier(i);
            randusleep(0, Arguments.maxSkierWaitTime);
            exit(0);
        } else if (SKIER < 0) {
            fprintf(stderr, "Could not fork process\n");
            kill(0, SIGKILL);
            cleanup();
            exit(1);
        }
    }
}

/*
 * @brief Main function that handles the main process
 */

int main(int argc, char *argv[]) {
    srand(time(NULL));
    /* open file  for output*/
    if ((f = fopen(OUTPUT_FILENAME, "w")) == NULL) {
        fprintf(stderr, "File can't be opened: ");
        exit(1);
    }

    validateInput(argc, argv);
    init_semaphores();
    pid_t BUS = fork();

    if (BUS == 0) {
        process_bus();
    } else if (BUS < 0) {
        fprintf(stderr, "Could not fork process\n");
        kill(0, SIGKILL);
        exit(1);
    } else {
        generateSkiers();
    }
    /* wait for all processes to finish */
    while (wait(NULL) > 0);

    cleanup();
    return 0;
}


