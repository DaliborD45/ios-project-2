#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#define OUTPUT_FILENAME "proj2.out"
#define MAIN_PROCESS_SEM "xdetkod00_mutex"
#define MULTIPLEX_SEM "xdetkod00_multiplex"
#define BUS_SEM "xdetkod00_bus"
#define ALL_ABOARD_SEM "xdetkod00_all_aboard"
