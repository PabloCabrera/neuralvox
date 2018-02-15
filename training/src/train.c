#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fann.h>

#define TRAINING_PRONUNCTATION_FILE "pronunctiation.txt"
#define TRAINING_RAW_DIR "raw"
#define BUFFER_SIZE 14000
#define NEURONS_INPUT_LAYER 16
#define NEURONS_HIDDEN_LAYER 64
#define PHONEME "^;*abBdeEfgiIjJklmnoOprRstTuUwx"
#define THRESHOLD 0.5
#define TRAINING_SET_SIZE 3030
#define SPECTROGRAM_OFFSET_START 256
#define SPECTROGRAM_OFFSET_END 6528
#define SPECTROGRAM_WINDOW 128
#define MEAN_WEIGHT 5
#define STDEV_WEIGHT 0.1

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
unsigned global_training_item_count = 0;

/* FUNCTION PROTOTYPES */
char *get_phoneme (char *pronunctiation);
void bubble_sort (char *word);
unsigned load_training_data (FILE *list);
long load_word_data (char *word, double **buffer);
long load_raw_file_data (char *filename, double **buffer);
fann_type *flat_data (double *data, long data_length);
fann_type *get_means_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_stdev_histogram (double *data, long data_length, fann_type *means, unsigned num_freqs);
void training_data_callback (unsigned num, unsigned num_input, unsigned num_output, fann_type *input , fann_type *output);
void clean_buffer ();
fann_type *get_result_vector (char *phoneme);
char *result_vector_to_string (fann_type *vector);
char *flat_data_to_string (fann_type *flatted_data, unsigned length);
void train_network (struct fann *network, struct fann_train_data *training_data);
void train_network_iteration (struct fann *network, struct training_item *item);
void test_network (struct fann *network);
void show_results (struct fann *network, struct training_item item);


int main (int arg_count, char *args[]) {
	struct fann *network;
	network = fann_create_from_file ("network.fann");
	if (network == NULL) {
		fprintf (stderr, "Creando red...\n");
		network = fann_create_standard (3, NEURONS_INPUT_LAYER, NEURONS_HIDDEN_LAYER, strlen (PHONEME));
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
		
		global_training_set [num_word] = info;

		free (tmp_data);
		free (line);
		line = NULL;
		num_word++;
	}
	global_training_item_count = num_word;
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

void training_data_callback (unsigned num, unsigned num_input, unsigned num_output, fann_type *input , fann_type *output) {
	//memcpy (input, global_training_set [num].data, num_input);
	memcpy (input, global_training_set [num].data_flatted, sizeof (fann_type) * num_input);
	memcpy (output, global_training_set [num].expected_result, sizeof (fann_type) * num_output);
}

void train_network (struct fann *network, struct fann_train_data *train_data) {
	fprintf (stderr, "\nEstado inicial:\n");
	test_network (network);
	unsigned num_word;
	struct training_item *item;
	unsigned num_iteration = 0;
	while (num_iteration < 500000) {
		fann_train_epoch (network, train_data);
		num_iteration++;
		if (num_iteration % 500 == 0) {
			printf ("\nIteración %d:\n", num_iteration);
			fann_save (network, "network.fann");
			test_network (network);
		}
	}
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

long load_raw_file_data (char *filename, double **buffer) {
	FILE *file = fopen (filename, "r");
	unsigned stop = 0;
	long total_readed = 0;

	if (file == NULL) {
		fprintf (stderr, "Error: No se puede leer el archivo %s\n", filename);
		return -1;
	}

	double *tmp_data = malloc (sizeof (double) * BUFFER_SIZE);
	memset (tmp_data, '\0', BUFFER_SIZE * sizeof (double));

	/* Skip offset pixels */
	fseek (file, SPECTROGRAM_OFFSET_START * sizeof (double), SEEK_SET);

	while (!stop) {

		long readed = fread (tmp_data + total_readed, sizeof (double), SPECTROGRAM_WINDOW, file);
		total_readed += readed;

		if (feof (file)) {
			stop = 1;
		} else if (total_readed + SPECTROGRAM_WINDOW > BUFFER_SIZE) {
			stop = 1;
			//fprintf (stderr, "Warning: Archivo demasiado grande: %s.\n", filename);
		}
	}
	
	fclose (file);
	*buffer = tmp_data;
	return (total_readed) - (SPECTROGRAM_OFFSET_END);
}

fann_type *flat_data (double *data, long data_length) {
	unsigned num_freqs = NEURONS_INPUT_LAYER/2;
	fann_type *flatted_data = malloc (NEURONS_INPUT_LAYER * sizeof (fann_type));
	fann_type *means = get_means_histogram (data, data_length, num_freqs);
	fann_type *stdevs = get_stdev_histogram (data, data_length, means, num_freqs);

	memcpy (flatted_data, means, num_freqs * sizeof (fann_type));
	memcpy (flatted_data + num_freqs, stdevs, num_freqs * sizeof (fann_type));

	free (means);
	free (stdevs);
	return flatted_data;
}

fann_type *get_means_histogram (double *data, long data_length, unsigned num_freqs) {
	long i;
	unsigned freq;
	fann_type *means = malloc (num_freqs * sizeof (fann_type));

	for (freq=0; freq < num_freqs; freq++) {
		means [freq] = 0;
	}

	if (data_length == 0) {
		fprintf (stderr, "ERROR: data_length es cero\n");
		return means;
	}

	for (i=0; i < data_length; i++) {
		freq = (i % SPECTROGRAM_WINDOW) / (SPECTROGRAM_WINDOW/num_freqs);
		means [freq] += (data [i]) / (SPECTROGRAM_WINDOW/num_freqs);

	}

	for (freq=0; freq < num_freqs; freq++) {
		means [freq] = MEAN_WEIGHT * (means [freq] / (data_length/SPECTROGRAM_WINDOW));
	}

	return means;
}

fann_type *get_stdev_histogram (double *data, long data_length, fann_type *means, unsigned num_freqs) {

	long i;
	unsigned freq;
	fann_type *stdevs = malloc (num_freqs * sizeof (fann_type));

	for (freq=0; freq < num_freqs; freq++) {
		stdevs [freq] = 0;
	}

	for (i=0; i < data_length; i++) {
		freq = (i % SPECTROGRAM_WINDOW) / (SPECTROGRAM_WINDOW/num_freqs);
		stdevs [freq] += fabsf (data [i] - means [freq]);
	}

	for (freq=0; freq < num_freqs; freq++) {
		stdevs [freq] = STDEV_WEIGHT * (stdevs [freq] / (data_length/SPECTROGRAM_WINDOW));
	}

	return stdevs;
}


fann_type *get_result_vector (char *phoneme) {
	fann_type *vector = malloc (strlen (PHONEME) * sizeof (fann_type));
	int pos;
	int num_phoneme = strlen (PHONEME);
	for (pos = 0; pos < strlen (PHONEME); pos++) {
		if (index (phoneme, PHONEME [pos]) != NULL) {
			vector [pos] = 1.0;
		} else {
			vector [pos] = 0.0;
		}
	}
	return vector;
}

char *result_vector_to_string (fann_type *vector) {
	int num_phoneme_symbols = strlen (PHONEME);
	char *phoneme = malloc (sizeof (char) * (num_phoneme_symbols +1));
	int out_pos = 0;
	int pos;
	for (pos=0; pos < num_phoneme_symbols; pos++) {
		if (vector [pos] > THRESHOLD) {
			phoneme [out_pos] = PHONEME[pos];	
			out_pos++;
		}
	}
	phoneme [out_pos] = '\0';
	return phoneme;
}

char *flat_data_to_string (fann_type *flatted_data, unsigned length) {
	char *ret = malloc ((length + 1) * sizeof (char));
	int i;
	for (i=0; i < length; i++) {
		int digit = (10 * flatted_data[i]);
		if (digit < 0) {
			digit = 0;
		} else if (digit > 9) {
			digit = 9;
		}
		char car = '0' + digit;
		ret [i] = car;
	}

	ret [length] = '\0';
	return ret;
}

void test_network (struct fann *network) {
	unsigned long primes [7] = {541, 7919, 104729, 1299709, 15485863, 2038074743};
	int i;
	for (i = 0; i < 7; i++) {
		show_results (network, global_training_set [primes [i] % global_training_item_count]);
	}
}

void show_results (struct fann *network, struct training_item item) {
	fann_type *result = fann_run (network, item.data_flatted);
	char *result_string = result_vector_to_string (result);
	char *input_string = flat_data_to_string (item.data_flatted, NEURONS_INPUT_LAYER);
	char *expected_string = result_vector_to_string (item.expected_result);
	char *success_string = (strcmp (result_string, expected_string) == 0)? "OK!": "";
	printf ("%s:\t [%s]  =>  (%s | %s)\t%s\n", item.word, input_string, result_string, expected_string, success_string);
	free (expected_string);
	free (input_string);
	free (result_string);
}
