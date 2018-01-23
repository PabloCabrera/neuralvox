#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRAINING_PRONUNCTATION_FILE "pronunctiation.txt"
#define TRAINING_WAV_DIR "wav"

/* FORWARD DEFINE */
char *get_phoneme (char *pronunctiation);
void bubble_sort (char *word);


int main (int arg_count, char *args[]) {
	FILE *list = fopen (TRAINING_PRONUNCTATION_FILE, "r");
	char *word = NULL;
	char *line = NULL;
	size_t line_length;
	while (getline (&line, &line_length, list) > 0) {
		word = line;
		char *delimiter = index (line, '|');
		delimiter[0] = '\0';
		char *pronunctiation = delimiter + 2;
		char *phoneme = get_phoneme (pronunctiation);
		printf ("%s %s\n", word, phoneme);
		free (phoneme);
		free (line);
		line = NULL;
	}
}

char *get_phoneme (char *pronunctiation) {
	int length = strlen (pronunctiation);
	char *phoneme = malloc (sizeof (char) * (1 + strlen (pronunctiation)));
	int pos;
	phoneme [0] = '\0';
	for (pos = 0; pos < length; pos++) {
	 	if (
			(index (phoneme, pronunctiation [pos]) == NULL) 
			&& (pronunctiation [pos] != '2')
			&& (pronunctiation [pos] != '\'')
			&& (pronunctiation [pos] != ',')
			&& (pronunctiation [pos] != '\r')
			&& (pronunctiation [pos] != '\n')
		) {
			strncat (phoneme, pronunctiation + pos, 1 * sizeof (char));
		}
	}
	bubble_sort (phoneme);
	return phoneme;
}

void bubble_sort (char *word) {
	int sorted = 0;
	int pos = 0;
	int length = strlen (word);
	while (sorted < length-1) {
		pos = 1;
		while (length - pos -1 > sorted) {
			char current = word [pos];
			char next = word [pos+1];
			if (current > next) {
				word [pos] = next;
				word [pos+1] = current;
			}
			pos++;
		}
		sorted++;
	}
}
