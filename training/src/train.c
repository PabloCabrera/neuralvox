#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <fftw3.h>
#include <sndfile.h>
#include <doublefann.h>
#include "load_wav.h"
#include "spectrogram.h"

#define TRAINING_PRONUNCTATION_FILE "pronunctiation.txt"
#define TRAINING_RAW_DIR "raw"
#define BUFFER_SIZE 16384
#define PHONEME_SYMBOLS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz*^"
#define THRESHOLD 0.9
#define SPECTROGRAM_WINDOW_SIZE 128
#define TRAINING_SET_SIZE 7000

/* DATA TYPES */
struct training_item {
	char *word;
	char *phoneme;
	double *data;
	long data_length;
	double *expected_result;
};

/* GLOBAL VARIABLES */
double global_training_buffer [BUFFER_SIZE];
struct training_item global_training_set [TRAINING_SET_SIZE];

/* FUNCTION PROTOTYPES */
char *get_phoneme (char *pronunctiation);
void bubble_sort (char *word);
unsigned load_training_data (FILE *list);
long load_word_data (char *word, double **buffer);
long load_raw_file_data (char *filename, double **buffer);
void dump_to_training_buffer (double *data, int length);
void clean_buffer ();
double *get_result_vector (char *phoneme);
char *result_vector_to_string (double *vector);
void train_network (struct fann *network, struct training_item *item);
void test_network (struct fann *network);
void show_results (struct fann *network, char *word);


int main (int arg_count, char *args[]) {
	struct fann *network = fann_create_standard (4, BUFFER_SIZE, 256, 128, strlen (PHONEME_SYMBOLS));
	fann_set_training_algorithm (network, FANN_TRAIN_INCREMENTAL);
	fann_set_learning_momentum (network, 0.8);
	FILE *list = fopen (TRAINING_PRONUNCTATION_FILE, "r");
	fprintf (stderr, "Cargando datos...\n");
	load_training_data (list);
	fclose (list);
	/*
	fprintf (stderr, "Entrenando...\n");
	train_network ();
	test_network (network);
	*/
	fann_destroy (network);
}

unsigned load_training_data (FILE *list) {
	char *word = NULL;
	char *line = NULL;
	size_t line_length;
	struct training_item info;
	unsigned num_word = 0;

	while (getline (&line, &line_length, list) > 0 && num_word < TRAINING_SET_SIZE) {
		word = line;
		char *delimiter = index (line, '|');
		delimiter[0] = '\0';
		char *pronunctiation = delimiter + 2;
		info.phoneme = get_phoneme (pronunctiation);
		info.expected_result = get_result_vector (info.phoneme);
		info.data_length = load_word_data (word, &(info.data));
		info.word = strdup (word);
		
		global_training_set [num_word] = info;

		line = NULL;
		num_word++;
		free (line);
	}
	return num_word;
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

void train_network (struct fann *network, struct training_item *item) {
	dump_to_training_buffer (item-> data, item-> data_length);
	fann_train  (network, global_training_buffer, item-> expected_result);
}

long load_word_data (char *word, double **buffer, long *size) {
	char *raw_filename = malloc (sizeof (char) * (11) + strlen (TRAINING_RAW_DIR) + strlen (word));
	strcpy (raw_filename, TRAINING_RAW_DIR);
	strcat (raw_filename, "/");
	strcat (raw_filename, word);
	strcat (raw_filename, ".half.raw");
	long total_readed = load_raw_file_data (raw_filename, buffer);

	free (raw_filename);
	return total_readed;
}

long load_raw_file_data (char *filename, double **buffer, long *size) {
	double *tmp_data = malloc (sizeof (double) * BUFFER_SIZE);
	FILE *file = fopen (filename, "r");
	unsigned stop = 0;
	long total_readed = 0;

	if (file == NULL) {
		fprintf (stderr, "Error: No se puede leer el archivo %s\n", filename);
		return -1;
	}

	while (!stop) {

		long readed = fread (tmp_data + total_readed, sizeof (double), 64, file);
		total_readed += readed;

		if (feof (file)) {
			stop = 1;
		} else if (total_readed + 64 > BUFFER_SIZE) {
			stop = 1;
			fprintf (stderr, "Warning: Archivo demasiado grande: %s.\n", filename);
		}
	}
	
	fclose (file);
	*buffer = tmp_data;
	return total_readed;
}

void dump_to_training_buffer (double *data, int length) {
	clean_buffer ();
	memcpy (global_training_buffer, data, sizeof (double) * length);
}

void clean_buffer () {
	memset (global_training_buffer, '\0', BUFFER_SIZE * sizeof (double));
}

double *get_result_vector (char *phoneme) {
	double *vector = malloc (strlen (PHONEME_SYMBOLS) * sizeof (double));
	int pos;
	int num_phoneme = strlen (PHONEME_SYMBOLS);
	for (pos = 0; pos < strlen (PHONEME_SYMBOLS); pos++) {
		if (index (phoneme, PHONEME_SYMBOLS [pos]) != NULL) {
			vector [pos] = 1.0;
		} else {
			vector [pos] = 0.0;
		}
	}
	return vector;
}

char *result_vector_to_string (double *vector) {
	int num_phoneme_symbols = strlen (PHONEME_SYMBOLS);
	char *phoneme = malloc (num_phoneme_symbols);
	int out_pos = 0;
	int pos;
	for (pos=0; pos < num_phoneme_symbols; pos++) {
		if (vector [pos] > THRESHOLD) {
			phoneme [out_pos] = PHONEME_SYMBOLS[pos];	
			out_pos++;
		}
	}
	phoneme [out_pos] = '\0';
	return phoneme;
}

void test_network (struct fann *network) {
	/*
	show_results (network, "probar");
	show_results (network, "embajada");
	show_results (network, "labio");
	show_results (network, "juramentar");
	show_results (network, "trapecista");
	show_results (network, "idealidad");
	show_results (network, "neutral");
	show_results (network, "unicornio");
	show_results (network, "cintura");
	*/
}

void show_results (struct fann *network, char *word) {
	/*
	load_word_spectrogram (word);
	double *result = fann_run (network, global_training_buffer);
	char *result_string = result_vector_to_string (result);
	printf ("%s: %s\n", word, result_string);
	free (result_string);
	*/
}
