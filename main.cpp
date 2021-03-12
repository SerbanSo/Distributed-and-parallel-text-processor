#include "mpi.h"
#include <stdlib.h>
#include <fstream>
#include "pthread.h"
#include "queue"

#include "constants.h"
#include "MPI_workers.h"
#include "processing.h"

// Coada folosita pentru a pastra ordinea paragrafelor din fisierul de intrare
std::queue<int> paragraph_order;

std::string inputFile;
std::string outputFile;

void *master_thread_funct(void *arg){
	int id = *(int*)arg;

	// Se deschide fisierul pentru citire
    std::ifstream file (inputFile);

	if(!file.is_open())
	{
        std::cout << "Eroare la deschiderea fisierului de pe thread-ul" << id << " din master\n";
	}

    std::string line;
    std::vector<std::string> paragraph;
    int lines;
    bool canSend = false;

    // Se determina paragrafele si se trimit la fiecare worker
    // Thread-ul desemnat worker-ului HORROR se ocupa si de
    // pastrarea ordinii paragrafelor
    // (ordinea in care trebuie afisate)
    while(getline(file, line)) {
        if(id == HORROR && line == genre_horror) {
            canSend = true;
            paragraph.clear();
            lines = 0;

            paragraph_order.push(HORROR);
        } else if(line == genre_fantasy) {
            if(id == FANTASY) {
                canSend = true;
                paragraph.clear();
                lines = 0;
            } else if (id == HORROR)
                paragraph_order.push(FANTASY);
        } else if(line == genre_comedy) {
            if(id == COMEDY) {
                canSend = true;
                paragraph.clear();
                lines = 0;
            }
            else if (id == HORROR)
                paragraph_order.push(COMEDY);
        } else if(line == genre_science_fiction) {
            if(id == SF) {
                canSend = true;
                paragraph.clear();
                lines = 0;                
            } else if (id == HORROR)
                paragraph_order.push(SF);
        } else if (line == ""){    // S-a terminat de citit un paragraf
            // Se trimite catre worker numarul de linii din paragraf si paragraful, linie cu linie
            // id-ul thread-ului = nodul worker corespunzator
            if(canSend) {
                MPI_Send(&lines, 1, MPI_INT, id, 0, MPI_COMM_WORLD);

                for(int i = 0; i < lines; i++) {
                    MPI_Send(paragraph[i].c_str(), paragraph[i].length(), MPI_CHAR, id, 0, MPI_COMM_WORLD);
                }
            }

            // Se reseteaza canSend
            canSend = false;
        } else if (canSend) {    // Se adauga linia in vector numai daca paragraful urmeaza sa fie trimis
            paragraph.push_back(line + "\n");

            // Se incrementeaza numarul de linii
            lines++;
        }
    }

    // Trimite ultimul paragraf din fisier, daca este cazul
    if(canSend) {
        MPI_Send(&lines, 1, MPI_INT, id, 0, MPI_COMM_WORLD);

        for(int i = 0; i < lines; i++) {
            MPI_Send(paragraph[i].c_str(), paragraph[i].length(), MPI_CHAR, id, 0, MPI_COMM_WORLD);
        }
    }

    // Trimite o valoarea pentru a transmite worker-ului ca nu o sa mai primeasca paragrafe
    lines = STOP_SEND;
    MPI_Send(&lines, 1, MPI_INT, id, 0, MPI_COMM_WORLD);

    // Inchide fisierul
    file.close();
	
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int numtasks, rank;

    if(argc < 2) {
        std::cout << "Usage: mpirun -np 5 main <input_file>\n";
        exit(-1);
    }

    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    if (rank == MASTER){ 			// master

        inputFile = std::string(argv[1]);
        outputFile = inputFile.substr(0, inputFile.find_last_of(".")) + ".out";

        // Se deschid 4 thread-uri pentru procesarea fisierului de input
        pthread_t threads[MASTER_THREADS];
        int arguments[MASTER_THREADS];
        int r;
        void *status;

        for(int i = 0; i < MASTER_THREADS; i++) {
        	arguments[i] = i+1;
        	r = pthread_create(&threads[i], NULL, master_thread_funct, (void *)&arguments[i]);

        	if(r) {
                std::cout << "Eroare la crearea thread-ului " << i << "\n";
				MPI_Finalize();
				exit(-1);
        	}
        }

        // Se asteapta terminarea executiei thread-urilor
        for(int i = 0; i < MASTER_THREADS; i++) {
        	r = pthread_join(threads[i], &status);

        	if(r) {
                std::cout << "Eroare la asteptarea thread-ului " << i << "\n";
        		MPI_Finalize();
        		exit(-1);
        	}
        }

    } else if (rank == HORROR) {	// horror
        MPI_workers_main(HORROR, processing_horror);
    } else if (rank == COMEDY) {	// comedy
        MPI_workers_main(COMEDY, processing_comedy);
    } else if (rank == FANTASY) { 	// fantasy
        MPI_workers_main(FANTASY, processing_fantasy);
    } else if (rank == SF) {		// SF
        MPI_workers_main(SF, processing_science_fiction);
    }

    // Nodul master se ocupa de afisarea paragrafelor prelucrate
    if(rank == MASTER) {
        int sendBack = SEND_BACK;
        MPI_Status mpiStatus;
        int count;

        std::ofstream file (outputFile);

        // Se trimite un semnal catre urmatorul worker in functie
        // de ordinea de intrare a paragrafelor, urmand sa se
        // primeasca paragraful modificat si sa fie afisat
        while(!paragraph_order.empty()) {
            int next = paragraph_order.front();
            paragraph_order.pop();

            // Se trimite la urmatorul worker semnal ca trebuie trimis inapoi paragraful prelucrat
            MPI_Send(&sendBack, 1, MPI_INT, next, 0, MPI_COMM_WORLD);

            // Verifica mesajul primit pentru a determina dimensiunea datelor primite
            MPI_Probe(next, 0, MPI_COMM_WORLD, &mpiStatus);

            // Determina numarul de caractere din mesaj
            MPI_Get_count(&mpiStatus, MPI_CHAR, &count);

            // Buffer temporal pentru paragraf
            char buffer[count];

            // Se primeste paragraful
            MPI_Recv(buffer, count, MPI_CHAR, next, 0, MPI_COMM_WORLD, &mpiStatus);

            // Coverteste sirul primit din char[] in std::string
            std::string tmp_str(buffer, count);

            // Se scrie antetul paragrafului
            if(next == HORROR) {
                file << genre_horror << "\n";
            } else if (next == COMEDY) {
                file << genre_comedy << "\n";
            } else if (next == FANTASY) {
                file << genre_fantasy << "\n";
            } else if (next == SF) {
                file << genre_science_fiction << "\n";
            }

            // Se scrie paragraful
            file << tmp_str << "\n";
        }
            file.close();
    }

	// Terminare MPI
	MPI_Finalize();

	return 0;
}