#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

// Tipul worker-ilor
#define MASTER 0
#define HORROR 1
#define COMEDY 2
#define FANTASY 3
#define SF 4

// Thread-ul din nodurile worker care se ocupa cu preluarea paragrafelor
#define READ_THREAD 0

// Valoare care semnifica ca nu se mai trimit paragrafe la workeri
#define STOP_SEND -1
// Valoare care semnifica ca trebuie trimis inapoi paragraful procesar
#define SEND_BACK 1

// Numarul de linii la care se porneste un thread nou
#define LINE_LIMIT 20

// Numarul de thread-uri rulate de master
#define MASTER_THREADS 4

// Structura pentru delimitarea liniilor procesare de un thread
typedef struct {
	int start;
	int end;
} interval;

// Argumentele trimise de nodurile worker catre thread-uri
typedef struct {
	int id;
	int genre;
	void (*processing_function) (interval);
} thread_func_args;

// Genurile de paragrafe
const std::string genre_horror = "horror";
const std::string genre_comedy = "comedy";
const std::string genre_fantasy = "fantasy";
const std::string genre_science_fiction = "science-fiction";

#endif