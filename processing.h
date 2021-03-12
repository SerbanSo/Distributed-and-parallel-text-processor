#ifndef PROCESSING_H
#define PROCESSING_H

#include "constants.h"
#include <iostream>
#include <vector>
#include <string>

// Paragraful care se prelucreaza, definitia se afla in "MPI_workers.cpp"
extern std::vector<std::string> current_paragraph;

// Metoda pentru procesat text de genul "horror"
void processing_horror(interval inv);

// Metoda pentru procesat text de genul "comedy"
void processing_comedy(interval inv);

// Metoda pentru procesat text de genul "fantasy"
void processing_fantasy(interval inv);

// Metoda pentru procesat text de genul "science-fiction"
void processing_science_fiction(interval inv);

#endif