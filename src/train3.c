#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fann.h>

#include "common.h"

#define DATASET_MAX_SIZE 12000
#define TESTSET_MAX_SIZE 1200
#define MAX_PHONEMES_PHO 32
#define SAMPLE_WIDTH 16
#define LEARNING_RATE_INITIAL 0.2
#define LEARNING_RATE_EPOCH_MULTIPLIER 0.999
#define LEARNING_RATE_MIN 0.001
#define ACCURACY_STOP_TRAINING 0.3
#define EPOCHS_BETWEEN_REPORT 100
#define NUM_VALIDATION_SAMPLES_SHOW 6
#define TRAINING_THRESHOLD 0.8

struct training_item {
	fann_type *input;
	fann_type *expected_output;
};

struct pron_timing;

struct pron_timing {
	char *phonemes;
	long start_time;
	long duration;
	struct pron_timing *next;
};

// Prototypes
int main (int argc, char **argv);
void load_training_data (char *raw_dir_path, char *pho_dir_path);
void dataset_add_raw (struct training_item dataset[], char *raw_filename, FILE *pho_file, long *dataset_size);
struct pron_timing *get_pron_timings (FILE *pho_file);
struct pron_timing *get_relevant_timings (struct pron_timing *timings, long from_time, long to_time);
char *get_relevant_phonemes (struct pron_timing *timings);
bool is_relevant_timming (long start1, long end1, long start2, long end2);
long get_pron_last_time (struct pron_timing *timing);
void fill_training_item (struct training_item *item, double *raw_data, long start, long end, char *relevant_phonemes);
void pron_timings_free (struct pron_timing *timings);
struct fann *create_network ();
void training_fann_callback (unsigned num, unsigned num_input, unsigned num_output, fann_type *input , fann_type *output);
void train_network (struct fann *network, struct fann_train_data *train_data);
float test_network (struct fann *network);
bool test_item (struct fann *network, struct training_item item);
void show_results (struct fann *network, struct training_item item);

struct training_item dataset [DATASET_MAX_SIZE];
struct training_item testset [TESTSET_MAX_SIZE];
long dataset_size = 0;
long testset_size = 0;

int main (int argc, char **argv) {
	load_training_data ("raw", "pho");

	struct fann *network = create_network ();
	struct fann_train_data *train_data = fann_create_train_from_callback (dataset_size, NEURONS_INPUT_LAYER, strlen (PHONEME), training_fann_callback);
	fann_shuffle_train_data (train_data);
	train_network (network, train_data);
	fann_destroy (network);
}

struct fann *create_network () {
	struct fann *network = fann_create_standard (
		4,
		NEURONS_INPUT_LAYER,
		32,
		32,
		strlen (PHONEME)
	);
	fann_set_training_algorithm (network, FANN_TRAIN_INCREMENTAL);
	fann_set_activation_function_hidden (network, FANN_ELLIOT_SYMMETRIC);
	fann_set_activation_function_output (network, FANN_ELLIOT);

	return network;
}


void training_fann_callback (unsigned num, unsigned num_input, unsigned num_output, fann_type *input , fann_type *output) {
	memcpy (input, dataset [num].input, sizeof (fann_type) * num_input);
	memcpy (output, dataset [num].expected_output, sizeof (fann_type) * num_output);
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
	double previous_MSE = 0.0;
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
			test_network (network);
			double current_MSE = fann_get_MSE (network);
			if (fabs (current_MSE - previous_MSE) < 0.0001 ) {
				stop = true;
			} else {
				previous_MSE = current_MSE;
				printf ("Learning Rate = %.8f\n", learning_rate);
			}
		}
	}

	fclose (log);
	
	fann_destroy_train (train_data);
}

void load_training_data (char *raw_dir_path, char *pho_dir_path) {

	DIR *raw_dir = opendir (raw_dir_path);
	struct dirent *file;
	long file_count = 0;
	while (((file = readdir (raw_dir)) != NULL) && (dataset_size < DATASET_MAX_SIZE)) {
		if (file-> d_type == DT_REG) {
			char *filename = file-> d_name;
			char *raw_filename = malloc (sizeof (char) * (strlen (raw_dir_path) + strlen (filename)+2));
			char *pho_filename = malloc (sizeof (char) * (strlen (raw_dir_path) + strlen (filename)+2));
			sprintf (raw_filename, "%s/%s", raw_dir_path, filename);
			sprintf (pho_filename, "%s/%s", pho_dir_path, filename);
			unsigned last = strlen (pho_filename);
			strcpy ((pho_filename+last-4), ".txt");
			FILE *pho_file = fopen (pho_filename, "r");	
			if (pho_file != NULL) {
				if (rand () % 10 > 0) {
					dataset_add_raw (dataset, raw_filename, pho_file, &dataset_size);
				} else {
					dataset_add_raw (testset, raw_filename, pho_file, &testset_size);
				}
			}
			fclose (pho_file);
			file_count++;
		}
	}

	if (dataset_size >= DATASET_MAX_SIZE) {
		printf ("WARNING: Se ha llenado el espacio reservado para el dataset de entrenamiento\n");
	}
	if (testset_size >= TESTSET_MAX_SIZE) {
		printf ("WARNING: Se ha llenado el espacio reservado para el dataset de pruebas\n");
	}

	printf ("Ejemplos cargados (entrenamiento: %d, pruebas: %d). Archivos: %d\n", dataset_size, testset_size, file_count);
}

void dataset_add_raw (struct training_item dataset[], char *raw_filename, FILE *pho_file, long *dataset_size) {
	struct pron_timing *timings = get_pron_timings (pho_file);
	long sample_rate = 44100;
	long step = 50;
	long from_time = 0;
	struct pron_timing *relevant_timings = NULL;
	long last_time = get_pron_last_time (timings);
	double *raw_data;
	long data_length = load_raw_file_data (raw_filename, &raw_data);

	for (from_time=0; from_time + step < last_time; from_time += step) {
		if (*dataset_size < DATASET_MAX_SIZE) {
			relevant_timings = get_relevant_timings (timings, from_time, from_time + step);
			char *relevant_phonemes = get_relevant_phonemes (relevant_timings);
			struct training_item *item = &(dataset[*dataset_size]);
			fill_training_item (item, raw_data, from_time, from_time+step, relevant_phonemes);

			(*dataset_size)++;
			pron_timings_free (relevant_timings);
		}
	}
	pron_timings_free (timings);
}


struct pron_timing *get_pron_timings (FILE *pho_file) {
	struct pron_timing *first, *current, *previous;
	size_t line_length;
	char *line_buffer = malloc (sizeof (char) * 256);
	long acummulated = 0;
	char phoneme_chars [256];
	int phoneme_duration;
	first = NULL;
	current = NULL;

	while (getline ((&line_buffer), &line_length, pho_file) > -1) {
		previous = current;
		phoneme_chars[0] = '\0';
		sscanf (line_buffer, "%s\t%d", phoneme_chars, &phoneme_duration);
		if (phoneme_chars[0] != '_' && phoneme_chars[0] != '\0') {
			current = malloc (sizeof (struct pron_timing));
			current-> phonemes = strdup (phoneme_chars);
			current-> start_time = acummulated;
			current-> duration = phoneme_duration;
			current-> next = NULL;
			acummulated += current-> duration;
			if (first == NULL) {
				first = current;
			} else {
				previous-> next = current;
			}
		}
	}

	free (line_buffer);
	return first;
}

struct pron_timing *get_relevant_timings (struct pron_timing *timings, long from_time, long to_time) {
	struct pron_timing *current, *relevant, *first_relevant, *last_relevant;
	first_relevant = NULL;
	current = timings;
	int count = 0;

	while (current != NULL) {
		if (is_relevant_timming (current-> start_time, (current->start_time + current->duration), from_time, to_time)) {
			relevant = malloc (sizeof (struct pron_timing));
			relevant-> phonemes = strdup (current-> phonemes);
			relevant-> start_time = current-> start_time;
			relevant-> duration= current-> duration;
			relevant-> next = NULL;
			count++;

			if (first_relevant == NULL) {
				first_relevant = relevant;
				last_relevant = relevant;
			} else {
				last_relevant-> next = relevant;
				last_relevant = relevant;
			}
		}
		current = current-> next;
	}

	return first_relevant;
}

bool is_relevant_timming (long start1, long end1, long start2, long end2) {
	return (
		((start1 <= start2) && (start2 <= end1))
		|| ((start2 <= start1) && (start1 <= end2))
	);
}

char *get_relevant_phonemes (struct pron_timing *timings) {
	char phonemes[256];
	phonemes[0] = '\0';
	int count = 0;
	struct pron_timing *current = timings;
	while (current != NULL) {
		if (strstr (phonemes, current-> phonemes) == NULL) {
			strcat (phonemes, current-> phonemes);
		}
		current = current-> next;
	}
	return strdup (phonemes);
}

long get_pron_last_time (struct pron_timing *timing) {
	long last = 0;
	struct pron_timing *previous = NULL;
	struct pron_timing *current = timing;

	while (current != NULL) {
		previous = current;
		current = current-> next;
	}
	if (previous != NULL) {
		last = (previous-> start_time) + (previous-> duration);
	}
	return last;
}

void fill_training_item (struct training_item *item, double *raw_data, long start, long end, char *relevant_phonemes) {
	unsigned sample_rate = 16000;
	unsigned overlap = 4;
	unsigned spectrogram_window = 128;

	unsigned slice_width = (sample_rate*overlap*(end-start))/(spectrogram_window*1000);
	unsigned slice_length = slice_width * spectrogram_window/2;
	long offset = (sample_rate * overlap * start) / (2000);
	double *slice_data = raw_data + offset;

	fann_type *neural_input = flat_data (slice_data, slice_length, NEURONS_INPUT_LAYER);
	item-> input = neural_input;
	item-> expected_output = get_result_vector (relevant_phonemes);
}

void pron_timings_free (struct pron_timing *timings) {
	if (timings != NULL) {
		if (timings-> next != NULL) {
			pron_timings_free (timings-> next);
		}
		if (timings-> phonemes != NULL) {
			free (timings-> phonemes);
		}
		free (timings);
	}
}

float test_network (struct fann *network) {
	unsigned long primes [7] = {541, 7919, 104729, 1299709, 15485863, 2038074743};
	unsigned i;
	unsigned num_ok = 0;
	float percentage_ok;

	for (i = 0; i < testset_size; i++) {
		if (test_item (network, testset [i])) {
			num_ok++;
		}
	}

	float accuracy = (((float) num_ok) / testset_size);

	percentage_ok = 100 * accuracy;
	printf ("MSE: %.4f   Validación: %.3f%% (%d/%d)\n", fann_get_MSE (network), percentage_ok, num_ok, testset_size);

	for (i = 0; i < NUM_VALIDATION_SAMPLES_SHOW; i++) {
		show_results (network, testset [primes [i] % testset_size]);
	}
	return accuracy;
}

bool test_item (struct fann *network, struct training_item item) {
	bool ok = false;
	fann_type *result = fann_run (network, item.input);
	char *result_string = result_vector_to_string (result, TRAINING_THRESHOLD);
	char *expected_string = result_vector_to_string (item.expected_output, TRAINING_THRESHOLD);
	ok = (strcmp (result_string, expected_string) == 0);
	free (result_string); free (expected_string);
	return ok;
}

void show_results (struct fann *network, struct training_item item) {
	fann_type *result = fann_run (network, item.input);
	char *result_string = result_vector_to_string (result, TRAINING_THRESHOLD);
	char *input_string = flat_data_to_string (item.input, NEURONS_INPUT_LAYER);
	char *expected_string = result_vector_to_string (item.expected_output, TRAINING_THRESHOLD);
	char *success_string = (strcmp (result_string, expected_string) == 0)? "OK!": "";
	printf ("[%s]  =>  (%s | %s)\t%s\n", input_string, result_string, expected_string, success_string);
	free (expected_string);
	free (input_string);
	free (result_string);
}
