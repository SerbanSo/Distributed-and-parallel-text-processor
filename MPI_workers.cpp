#include "MPI_workers.h"

// Vector cu liniile paragrafului curent
std::vector<std::string> current_paragraph;

// Coada pentru toate paragrafele prelucrate
std::queue<std::string> paragraphs;

// Coada pentru grupul de linii
std::queue<interval> processQueue;

pthread_mutex_t queueMutex;

sem_t items;
sem_t spaces;

int NR_MAX_THREADS;
bool workersDone = false;

void *workers_thread_func(void *arg) {
	thread_func_args tmp_struct = *(thread_func_args*)arg;
	int id = tmp_struct.id;
	int genre = tmp_struct.genre;
	void (*processing_function) (interval) = tmp_struct.processing_function;

	MPI_Status status;

	if(id == READ_THREAD) {
		int count;
		int lines;

		while(1) {
			// Se primeste numarul de linii din paragraf
			MPI_Recv(&lines, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);

			// Daca valoarea primita indica faptul ca nu se mai primesc paragrafe
			// se iese din loop
			if(lines == STOP_SEND)
				break;

			current_paragraph.clear();

			for(int i = 0; i < lines; i++) {
				// Verifica mesajul primit pentru a determina dimensiunea datelor primite
				MPI_Probe(MASTER, 0, MPI_COMM_WORLD, &status);

				// Determina numarul de caractere din mesaj
				MPI_Get_count(&status, MPI_CHAR, &count);

				// Buffer temporal pentru paragraf
				char buffer[count];

				// Se primeste paragraful
				MPI_Recv(buffer, count, MPI_CHAR, MASTER, 0, MPI_COMM_WORLD, &status);

				// Coverteste sirul primit din char[] in std::string
				current_paragraph.push_back(std::string(buffer, count));
			}
			
			// Se determina numarul de job-uri ce trebuie sa fie efectuate
			// pentru a prelucra paragraful
			int jobs = lines / LINE_LIMIT + ((lines % LINE_LIMIT == 0) ? 0 : 1);

			// Se impart liniile in grupuri de cate 20 (ultimul grup poate sa fie mai mic)
			// se adauga in coada perechile de linii (linia de unde incepe grupul si linia
			// unde se termina) si se notifica workerii pentru prelucrarea grupurilor
			// un grup = o notificare
			for(int i = 0; i < lines; i += LINE_LIMIT) {

				// Linia de start si linia de end a grupului
				interval tmp;
				tmp.start = i;
				tmp.end = std::min(i + LINE_LIMIT - 1, lines - 1);

				// Se asteapta pana cand se elibereaza memorie in coada
				// implementare doar teoretica, se pare ca e destul de greu
				// de verificat daca nu mai este spatiu disponibil intr-o coada :(
				// pthread_mutex_lock(&queueMutex);
				// 	if(processQueue.isFull()) {
				// 		pthread_mutex_unlock(&queueMutex);
				// 		sem_wait(&space);
				// 	} else 
				// 		pthread_mutex_unlock(&queueMutex);

				// Se adauga in coada
				pthread_mutex_lock(&queueMutex);
					processQueue.push(tmp);
				pthread_mutex_unlock(&queueMutex);
				
				// Se notifica workerii
				sem_post(&items);
			}

			// Se asteapta pana se termina de prelucrat paragraful
			for(int i = 0; i < jobs; i++){
				sem_wait(&spaces);
			}

			std::string tmp_str = "";
			// Se combina paragraful intr-un singur string
			for(int i = 0; i < lines; i++)
				tmp_str += current_paragraph[i];

			// Se adauga paragraful prelucrat in coada pentru a fi trimit inapoi la master
			paragraphs.push(tmp_str);
		}

		// Se notifica toti workerii ca s-au terminat de prelucrat paragrafele
		workersDone = true;
		for(int i = 0; i < NR_MAX_THREADS; i++) {
			sem_post(&items);
		}
	} else {
		while(1) {

			// Workerii asteapta notificari
			sem_wait(&items);

			// Daca s-au terminat de prelucrat toate paragrafele se iese din bucla
			if(workersDone)
				break;

			// Se extrage grupul ce trebuie prelucrat din coada
			pthread_mutex_lock(&queueMutex);
				interval tmp2 = processQueue.front();

				// Daca coada e plina, se scoate primul element
				// si se trimite o notificare care semnifica ca s-a 
				// eliberat un loc in coada
				// implementare doar teoretica
				// if(processQueue.isFull())
				// 	processQueue.pop();
				// 	sem_post(&space);
				// else
				// 	processQueue.pop();

				processQueue.pop();

			pthread_mutex_unlock(&queueMutex);

			// Procesarea grupului de linii din paragraf
			processing_function(tmp2);

			// Worker-ul trimite inapoi o notificare cand a terminat de prelucrat grupul de linii
			sem_post(&spaces);
		}
	}

	// Thread-ul care se ocupa de primirea paragrafelor este si cel care trimite 
	// inapoi la master paragrafele prelucrate
	if(id == READ_THREAD) {
		int sendNext;

		while(!paragraphs.empty()) {
			// Se asteapta valoarea care indica faptul ca trebuie trimis urmatorul paragraf din coada
			MPI_Recv(&sendNext, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);


			if(sendNext == SEND_BACK) {
				std::string tmp_str = paragraphs.front();
				paragraphs.pop();

				// Se trimite inapoi paragraful procesat
				MPI_Send(tmp_str.c_str(), tmp_str.length(), MPI_CHAR, MASTER, 0, MPI_COMM_WORLD);
			}
		}
	}

    pthread_exit(NULL);
}

void MPI_workers_main(int genre, void (*processing_function) (interval)) {
	// Se determina numarul maxim de thread-uri de pe sistemul curent
	NR_MAX_THREADS = sysconf(_SC_NPROCESSORS_CONF);

	// Initializare mutex si semafoare
	pthread_mutex_init(&queueMutex, NULL);
	sem_init(&items, 0, 0);
	sem_init(&spaces, 0, 0);

	// Se pornesc NR_MAX_THREADS thread-uri; unul pentru receptarea datelor
	// de la master, iar restul pentru procesarea datelor primite
    pthread_t threads[NR_MAX_THREADS];
    thread_func_args arguments[NR_MAX_THREADS];
    int r;
    void *status;

    for(int i = 0; i < NR_MAX_THREADS; i++) {
    	arguments[i].id = i;
    	arguments[i].genre = genre;
    	arguments[i].processing_function = processing_function;
    	r = pthread_create(&threads[i], NULL, workers_thread_func, (void *)&arguments[i]);

	    if(r) {
		    std::cout << "Eroare la crearea thread-ului " << i << "\n";
			exit(-1);
	    }
    }

    // Se asteapta terminarea executiei thread-urilor
    for(int i = 0; i < NR_MAX_THREADS; i++) {
        r = pthread_join(threads[i], &status);

        if(r) {
            std::cout << "Eroare la asteptarea thread-ului " << i << "\n";
        	exit(-1);
    	}
    }

}