/***************************
 * @file proj2.c
 * @brief Main file for proj2
 * @details
 * @author Dalibor Detko
***************************/

#include "main.h"
#include <stdarg.h>

/* SHARED MEMORY */
int *numberOfCodeLines;
int *currentBusStop;
int *numberOfGoneSkiers;
int *numberOfPeopleOnEachBusStop;
/* SEMAPHORES */
sem_t *mutex = NULL;
sem_t *bus = NULL;
sem_t *boarded = NULL;
sem_t *generator = NULL;
sem_t *printing = NULL;
sem_t *leave = NULL;
/*FILE*/
FILE *f;

typedef struct {
    int numberOfSkiers;
    int numberOfBusStops;
    int busCapacity;
    int maxSkierWaitTime;
    int maxBusDriveTime;
} ARGUMENTS_T;

ARGUMENTS_T Arguments;

void checkSemaphoreError(int isOpen) {
    if (!isOpen) {
        fprintf(stderr, "Could not initialize MAIN_PROCESS_SEM");
        exit(1);
    }
}

void fprintf_flush(FILE *stream, const char *format, ...) {
    sem_post(printing);
    (*numberOfCodeLines)++;
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
    fflush(stream);
    sem_trywait(printing);
}

void init_semaphores(void) {
    currentBusStop = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    numberOfGoneSkiers = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    numberOfPeopleOnEachBusStop = (int *) mmap(NULL, sizeof(int) * Arguments.numberOfBusStops, PROT_READ | PROT_WRITE,
                                               MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    numberOfCodeLines = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (currentBusStop == MAP_FAILED || numberOfGoneSkiers == MAP_FAILED) {
        fprintf(stderr, "Could not initialize shared memory\n");
        exit(1);
    }
    for (int i = 0; i < Arguments.numberOfBusStops + 1; i++) {
        numberOfPeopleOnEachBusStop[i] = 0;
    }
    *currentBusStop = 1;
    *numberOfGoneSkiers = 0;
    *numberOfCodeLines = 0;
    mutex = sem_open(MAIN_PROCESS_SEM, O_CREAT, 0666, 1);

    bus = sem_open(BUS_SEM, O_CREAT, 0666, 0);
    boarded = sem_open("boarded", O_CREAT, 0666, 0);
    printing = sem_open("printing", O_CREAT, 0666, 0);
    leave = sem_open("leave", O_CREAT, 0666, 0);

    if (mutex == SEM_FAILED || bus == SEM_FAILED || boarded == SEM_FAILED || generator == SEM_FAILED) {
        fprintf(stderr, "Could not initialize semaphores\n");
        exit(1);
    }

}

void cleanup(void) {
    /* Close and unlink semaphores */
//    munmap(currentBusStop, sizeof(int));
//    munmap(numberOfGoneSkiers, sizeof(int));
//    if (munmap(MAIN_PROCESS_SEM, sizeof(int)) == -1 || munmap(BUS_SEM, sizeof(int)) == -1 ||
//        munmap(MULTIPLEX_SEM, sizeof(int)) == -1 || munmap(ALL_ABOARD_SEM, sizeof(int)) == -1) {
//        fprintf(stderr, "Could not delete shared memory\n");
//        exit(1);
//    }

    sem_close(mutex);
    sem_close(bus);
    sem_close(boarded);
    sem_close(generator);
    sem_close(printing);
    sem_close(leave);


    sem_unlink(MAIN_PROCESS_SEM);
    sem_unlink(BUS_SEM);
    sem_unlink(MULTIPLEX_SEM);
    sem_unlink(ALL_ABOARD_SEM);
    fclose(f);
}


void randusleep(int min, int max) {
    usleep(rand() % (max - min + 1) + min);
}


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

void process_bus(void) {
    fprintf_flush(f, "BUS: started\n");
    while (*numberOfGoneSkiers != Arguments.numberOfSkiers) {
        usleep(100);
        fprintf_flush(f, "BUS: arrived to %d\n", *currentBusStop);
        sem_wait(mutex);
        for (int i = 0; i < numberOfPeopleOnEachBusStop[*currentBusStop]; i++) {
            sem_post(bus);
            sem_wait(boarded);
            if (*currentBusStop == Arguments.numberOfBusStops) {
                sem_post(leave);
            }
        }
        numberOfPeopleOnEachBusStop[*currentBusStop] = 0;
        sem_post(mutex);
        fprintf_flush(f, "BUS: leaving %d\n", *currentBusStop);
        *currentBusStop += 1;
        if (*currentBusStop > Arguments.numberOfBusStops) {
            *currentBusStop = 1;
        }
    }
    fprintf_flush(f, "BUS: finished\n");
}


void process_skier(int skierID) {
    int generatedSkierBusStop = (rand() % ((Arguments.numberOfBusStops - 1))) + 1;

    fprintf_flush(f, "L %d: arrived to %d\n", skierID, generatedSkierBusStop);
    numberOfPeopleOnEachBusStop[generatedSkierBusStop] += 1;
    sem_wait(bus);
    if (generatedSkierBusStop == *currentBusStop) {
        fprintf_flush(f, "L %d: boarding\n", skierID);
    }
    sem_post(boarded);
    sem_trywait(leave);
    if (*currentBusStop == Arguments.numberOfBusStops) {
        printf("this is triggered");
        fflush(stdout);
        fprintf_flush(f, "L %d: is going to ski\n", skierID);
        *numberOfGoneSkiers += 1;
    }

}

void generateSkiers(void) {
    for (int i = 1; i <= Arguments.numberOfSkiers; i++) {
        pid_t SKIER = fork();
        if (SKIER == 0) {
            process_skier(i);
        } else if (SKIER < 0) {
            fprintf(stderr, "Could not fork process\n");
            kill(0, SIGKILL);
//            cleanUp();
            exit(1);
        }
        int waitTime = rand() % Arguments.maxSkierWaitTime;

        randusleep(0, waitTime);
        usleep(100);
    }
}


int main(int argc, char *argv[]) {
    srand(time(NULL));
    //  OPEN | CREATE file
    if ((f = fopen(OUTPUT_FILENAME, "w")) == NULL) {
        fprintf(stderr, "File can't be opened: ");
        exit(1);
    }

    validateInput(argc, argv);
    init_semaphores();
    pid_t BUS = fork();

    if (BUS == 0) {
        process_bus();
    }else if(BUS < 0){
        fprintf(stderr, "Could not fork process\n");
        kill(0, SIGKILL);
        exit(1);
    }
    else {
        generateSkiers();
    }
    while (wait(NULL) > 0);
    cleanup();
    return 0;
}


