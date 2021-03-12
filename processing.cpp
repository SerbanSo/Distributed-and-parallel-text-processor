#include "processing.h"

void processing_horror(interval inv) {
	for(int i = inv.start; i <= inv.end; i++) {
		char process_buffer[2 * current_paragraph[i].size()];
		int index = 0;

		for(int j = 0; j < current_paragraph[i].size(); j++) {

			// Se copiaza caracterul curent
			process_buffer[index] = current_paragraph[i][j];
			index++;

			// Daca caracterul curent este o consoana, acesta se mai adauga odata
			if(isalpha(current_paragraph[i][j])
				&& current_paragraph[i][j] != 'a' 
				&& current_paragraph[i][j] != 'e'
			 	&& current_paragraph[i][j] != 'i' 
			 	&& current_paragraph[i][j] != 'o'
			 	&& current_paragraph[i][j] != 'u'
				&& current_paragraph[i][j] != 'A'
			 	&& current_paragraph[i][j] != 'E' 
			 	&& current_paragraph[i][j] != 'I'
			 	&& current_paragraph[i][j] != 'O'
			 	&& current_paragraph[i][j] != 'U')
			{
				process_buffer[index] = tolower(current_paragraph[i][j]);
				index++;
			}
		}

		process_buffer[index] = '\0';
		current_paragraph[i] = std::string(process_buffer, index);
	}
}

void processing_comedy(interval inv) {
	for(int i = inv.start; i <= inv.end; i++) {
		int counter = 0;

		for(int j = 0; j < current_paragraph[i].size(); j++) {
			if(current_paragraph[i][j] != ' ') {
				counter++;

				// Se modifica din 2 in 2 literele din acelasi cuvant
				if(counter % 2 == 0) {
					current_paragraph[i][j] = toupper(current_paragraph[i][j]);
				}
			} else {
				counter = 0;
			}
		}
	}
}

void processing_fantasy(interval inv) {
	for(int i = inv.start; i <= inv.end; i++) {
		bool isFirst = true;

		for(int j = 0; j < current_paragraph[i].size(); j++) {
			if(isalpha(current_paragraph[i][j])) {
				// Doar prima litera din cuvant se transforma in majuscula
				if(isFirst) {
					isFirst = false;
					current_paragraph[i][j] = toupper(current_paragraph[i][j]);
				}
			} else {
				isFirst = true;
			}
		}
	}
}

void processing_science_fiction(interval inv) {
	for(int i = inv.start; i <= inv.end; i++) {
		int pos = -1;
		int counter = 0;
		bool firstChar = true;
		int startRev;
		int endRev;

		for(int j = 0; j < current_paragraph[i].size(); j++) {
			if(current_paragraph[i][j] != ' '
				&& current_paragraph[i][j] != '\n') 
			{
				if(firstChar) {
					firstChar = false;
					counter++;
				}

				if(counter == 7) {
					startRev = j;
					endRev = current_paragraph[i].find(' ', j);
					if(endRev == std::string::npos)
						endRev = current_paragraph[i].find('\n', j);

					endRev--;

					int middleRev = (endRev - startRev) / 2;
					for(int j = 0; j <= middleRev; j++) {
						char tmp = current_paragraph[i][startRev + j];
						current_paragraph[i][startRev + j] = current_paragraph[i][endRev - j];
						current_paragraph[i][endRev - j] = tmp;
					}

					counter = 0;
				}
			} else {
				firstChar = true;
			}

		}

	}
}