#ifndef MPI_WORKERS_H
#define MPI_WORKERS_H

#include "mpi.h"
#include "constants.h"
#include <iostream>
#include <queue>
#include <string>
#include <unistd.h>
#include <semaphore.h>

// Functia fiecarui fir de executie pornit de un nod worker de tip MPI
// argumentul dat este de tipul "struct thread_func_args" (vezi "constants.h")
void *workers_thread_func(void *arg);

// Functie apelata de nodurile worker de tip MPI, al doilea argument reprezinta
// o functie din "processing.h"
void MPI_workers_main(int genre, void (*processing_function) (interval));

#endif