#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fann.h>
#include "common.h"

#define TRAINING_PRONUNCTATION_FILE "pronunctiation.txt"
#define TRAINING_RAW_DIR "raw"
#define NEURONS_HIDDEN_LAYER 64
#define TRAINING_THRESHOLD 0.8
#define TRAINING_SET_SIZE 2200
#define LEARNING_RATE_INITIAL 0.2
#define LEARNING_RATE_EPOCH_MULTIPLIER 0.999
#define LEARNING_RATE_MIN 0.001
#define ACCURACY_STOP_TRAINING 0.3
//#define MAX_TRAIN_EPOCHS 1000000
#define EPOCHS_BETWEEN_REPORT 100
//#define DESIRED_ERROR 0.0001
#define NUM_VALIDATION_SAMPLES_SHOW 0


#define NUM_PHONEME_SYNONYMS 8
char PHONEME_SYNONYMS[][2] = {
	{'I', 'i'},
	{'O', 'o'},
	{'E', 'e'},
	{'Q', 'g'},
	{'^', 'j'},
	{'J', 'j'},
	{'B', 'b'},
	{'U', 'u'}
};

/* DATA TYPES */
struct training_item {
	char *word;
	char *phoneme;
	fann_type *data_flatted;
	long data_length;
	fann_type *expected_result;
};

/* GLOBAL VARIABLES */
struct training_item global_training_set [TRAINING_SET_SIZE];
struct training_item global_validation_set [TRAINING_SET_SIZE];
unsigned global_training_item_count = 0;
unsigned global_validation_item_count = 0;


/* FUNCTION PROTOTYPES */
char *get_phoneme (char *pronunctiation);
void bubble_sort (char *word);
unsigned load_training_data (FILE *list);
long load_word_data (char *word, double **buffer);
void replace_phoneme_synonyms (char *phoneme);
void training_data_callback (unsigned num, unsigned num_input, unsigned num_output, fann_type *input , fann_type *output);
void train_network (struct fann *network, struct fann_train_data *training_data);
void train_network_iteration (struct fann *network, struct training_item *item);
float test_network (struct fann *network);
bool test_item (struct fann *network, struct training_item item);
void show_results (struct fann *network, struct training_item item);


int main (int arg_count, char *args[]) {
	struct fann *network;
	network = fann_create_from_file ("network.fann");
	if (network == NULL) {
		fprintf (stderr, "Creando red...\n");
		network = fann_create_standard (
			5,
			NEURONS_INPUT_LAYER,
			NEURONS_HIDDEN_LAYER,
			NEURONS_HIDDEN_LAYER,
			NEURONS_HIDDEN_LAYER,
			strlen (PHONEME));
		fann_set_training_algorithm (network, FANN_TRAIN_INCREMENTAL);
		fann_set_activation_function_hidden (network, FANN_ELLIOT_SYMMETRIC);
		fann_set_activation_function_output (network, FANN_ELLIOT);
	} else {
		fprintf (stderr, "Red cargada desde archivo network.fann\n");
	}
	FILE *list = fopen (TRAINING_PRONUNCTATION_FILE, "r");
	fprintf (stderr, "Cargando datos...\n");
	load_training_data (list);
	fclose (list);
	struct fann_train_data *train_data = fann_create_train_from_callback (global_training_item_count, NEURONS_INPUT_LAYER, strlen (PHONEME), training_data_callback);
	fann_shuffle_train_data (train_data);
	fprintf (stderr, "Entrenando... (%d ejemplos)\n", global_training_item_count);
	train_network (network, train_data);
	fann_destroy (network);
}

unsigned load_training_data (FILE *list) {
	char *word = NULL;
	char *line = NULL;
	size_t line_length;
	struct training_item info;
	unsigned num_word = 0;
	double *tmp_data;

	while (getline (&line, &line_length, list) > 0 && num_word < TRAINING_SET_SIZE) {
		word = line;
		char *delimiter = index (line, '|');
		delimiter[0] = '\0';
		char *pronunctiation = delimiter + 2;
		info.phoneme = get_phoneme (pronunctiation);
		info.expected_result = get_result_vector (info.phoneme);
		info.data_length = load_word_data (word, &tmp_data);
		info.word = strdup (word);
		info.data_flatted = flat_data (tmp_data, info.data_length);
		
		if (rand () % 10 > 0) {
			global_training_set [global_training_item_count] = info;
			global_training_item_count++;
		} else {
			global_validation_set [global_validation_item_count] = info;
			global_validation_item_count++;
		}

		free (tmp_data);
		free (line);
		line = NULL;
		num_word++;
	}
	return num_word;
}

char *get_phoneme (char *pronunctiation) {
	//printf ("get_phoneme (%s)\n", pronunctiation);
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
	replace_phoneme_synonyms (phoneme);
	bubble_sort (phoneme);
	return phoneme;
}

void replace_phoneme_synonyms (char *phoneme) {
	unsigned length = strlen (phoneme);
	unsigned i, phs;
	for (i=0; i < length; i++) {
		for (phs=0; phs < NUM_PHONEME_SYNONYMS; phs++) {
			if (phoneme [i] == PHONEME_SYNONYMS [phs][0]) {
				phoneme [i] = PHONEME_SYNONYMS [phs][1];
			}
		}
	}
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

void training_data_callback (unsigned num, unsigned num_input, unsigned num_output, fann_type *input , fann_type *output) {
	//memcpy (input, global_training_set [num].data, num_input);
	memcpy (input, global_training_set [num].data_flatted, sizeof (fann_type) * num_input);
	memcpy (output, global_training_set [num].expected_result, sizeof (fann_type) * num_output);
}

void train_network (struct fann *network, struct fann_train_data *train_data) {
	fprintf (stderr, "\nEstado inicial: ");
	FILE *log = fopen ("train_log.txt", "w");
	test_network (network);
	unsigned num_word;
	struct training_item *item;
	unsigned num_iteration = 0;
	bool stop = false;
	double learning_rate = LEARNING_RATE_INITIAL;
	printf ("Learning Rate = %.8f\n", learning_rate);
	fann_set_learning_rate (network, learning_rate);
	while (!stop) {
		fann_train_epoch (network, train_data);
		num_iteration++;
		fprintf (log, "%ld\t%f\n", num_iteration, fann_get_MSE (network));
		if (learning_rate > LEARNING_RATE_MIN) {
			learning_rate = learning_rate * LEARNING_RATE_EPOCH_MULTIPLIER;
			fann_set_learning_rate (network, learning_rate);
		}
		if (num_iteration % EPOCHS_BETWEEN_REPORT == 0) {
			printf ("[Iteración %d] ", num_iteration, learning_rate);
			fflush (log);
			fann_save (network, "network.fann");
			float accuracy = test_network (network);
			if (accuracy >= ACCURACY_STOP_TRAINING) {
				stop = true;
			}
			printf ("Learning Rate = %.8f\n", learning_rate);
		}
	}

	fclose (log);
	
	//fann_train_on_data (network, train_data, MAX_TRAIN_EPOCHS, EPOCHS_BETWEEN_REPORT, DESIRED_ERROR);
	fann_destroy_train (train_data);
}

void train_network_iteration (struct fann *network, struct training_item *item) {
	fann_train  (network, item-> data_flatted, item-> expected_result);
}

long load_word_data (char *word, double **buffer) {
	char *raw_filename = malloc (strlen("/.raw$") + strlen (TRAINING_RAW_DIR) + strlen (word));
	strcpy (raw_filename, TRAINING_RAW_DIR);
	strcat (raw_filename, "/");
	strcat (raw_filename, word);
	strcat (raw_filename, ".raw");
	long total_readed = load_raw_file_data (raw_filename, buffer);

	free (raw_filename);
	return total_readed;
}

float test_network (struct fann *network) {
	unsigned long primes [7] = {541, 7919, 104729, 1299709, 15485863, 2038074743};
	unsigned i;
	unsigned num_ok = 0;
	float percentage_ok;

	for (i = 0; i < global_validation_item_count; i++) {
		if (test_item (network, global_validation_set [i])) {
			num_ok++;
		}
	}

	float accuracy = (((float) num_ok) / global_validation_item_count);

	percentage_ok = 100 * accuracy;
	printf ("MSE: %.4f   Validación: %.3f%% (%d/%d)\n", fann_get_MSE (network), percentage_ok, num_ok, global_validation_item_count);

	for (i = 0; i < NUM_VALIDATION_SAMPLES_SHOW; i++) {
		show_results (network, global_validation_set [primes [i] % global_validation_item_count]);
	}
	return accuracy;
}

bool test_item (struct fann *network, struct training_item item) {
	bool ok = false;
	fann_type *result = fann_run (network, item.data_flatted);
	char *result_string = result_vector_to_string (result, TRAINING_THRESHOLD);
	char *expected_string = result_vector_to_string (item.expected_result, TRAINING_THRESHOLD);
	ok = (strcmp (result_string, expected_string) == 0);
	free (result_string); free (expected_string);
	return ok;
}


void show_results (struct fann *network, struct training_item item) {
	fann_type *result = fann_run (network, item.data_flatted);
	char *result_string = result_vector_to_string (result, TRAINING_THRESHOLD);
	char *input_string = flat_data_to_string (item.data_flatted, NEURONS_INPUT_LAYER);
	char *expected_string = result_vector_to_string (item.expected_result, TRAINING_THRESHOLD);
	char *success_string = (strcmp (result_string, expected_string) == 0)? "OK!": "";
	printf ("%s:\t [%s]  =>  (%s | %s)\t%s\n", item.word, input_string, result_string, expected_string, success_string);
	free (expected_string);
	free (input_string);
	free (result_string);
}
