#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fann.h>
#include "common.h"

#define NEURONS_HIDDEN_LAYER 64
#define NUM_TRAINING_EXAMPLES 8000
#define SLICE_WIDTH 18

/* DATA TYPES */
struct training_item {
	char phoneme;
	fann_type *data_flat;
};

struct training_dataset {
	long num_items;
	struct training_item *items;
};

/* GLOBAL VARIABLES */

struct training_dataset *global_dataset;

/* FUNCTION PROTOTYPES */
struct fann *get_network ();
struct training_dataset *get_training_dataset ();
void train_network (struct fann *network, struct training_dataset *dataset);
long generate_train_items (double *raw_data, long data_length, struct training_item *output_start, char phoneme);
void callback_training (unsigned num, unsigned num_input, unsigned num_output, fann_type *input , fann_type *output);


int main (int arg_count, char *args[]) {
	struct fann *network = get_network ();
	fprintf (stderr, "Cargando datos...\n");
	struct training_dataset *dataset = get_training_dataset ();
	fprintf (stderr, "Entrenando red...\n");
	train_network (network, dataset);
	fprintf (stderr, "Entrenamiento completo.\n");
	fann_save (network, "network.fann");
	fann_destroy (network);
}

struct fann *get_network () {
	struct fann *network = fann_create_from_file ("network.fann");
	if (network == NULL) {
		fprintf (stderr, "Creando red...\n");
		network = fann_create_shortcut (
			3,
			NEURONS_INPUT_LAYER,
			NEURONS_HIDDEN_LAYER,
			strlen (PHONEME));
		fann_set_training_algorithm (network, FANN_TRAIN_INCREMENTAL);
		fann_set_activation_function_hidden (network, FANN_ELLIOT_SYMMETRIC);
		fann_set_activation_function_output (network, FANN_ELLIOT);
	} else {
		fprintf (stderr, "Red cargada desde archivo network.fann\n");
	}
	return network;
}

struct training_dataset *get_training_dataset () {
	struct training_dataset *dataset = malloc (sizeof (struct training_dataset));

	dataset-> num_items = 0;
	dataset-> items = malloc (sizeof (struct training_item) * NUM_TRAINING_EXAMPLES);

	char *raw_filename = malloc (sizeof (char) * strlen ("raw/sample_X.raw$"));

	unsigned i;
	for (i=0; i < strlen (PHONEME); i++) {
		char phoneme = PHONEME [i];
		char phon_tr = (phoneme == '*')? 'Q': phoneme;
		sprintf (raw_filename, "raw/sample_%c.raw", phon_tr);
		double *raw_data;
		long data_length = load_raw_file_data (raw_filename, &raw_data);
		long offset = dataset-> num_items;
		long num_generated = generate_train_items (raw_data, data_length, dataset-> items + offset, phoneme);
		dataset-> num_items += num_generated;
		free (raw_data);
	}
	free (raw_filename);

	return dataset;
}

long generate_train_items (double *raw_data, long data_length, struct training_item *output_start, char phoneme) {
	long spectrogram_width = data_length / SPECTROGRAM_WINDOW;
	long num_slices = spectrogram_width / SLICE_WIDTH;
	unsigned i;
	for (i=0; i < num_slices; i++) {
		double *slice_start = raw_data + SLICE_WIDTH * SPECTROGRAM_WINDOW;
		fann_type *flatted_data = flat_data (slice_start, SLICE_WIDTH * SPECTROGRAM_WINDOW);
		struct training_item *item = output_start + i;
		item-> phoneme = phoneme;
		item-> data_flat = flatted_data;
	}

	return num_slices;
}

void train_network (struct fann *network, struct training_dataset *dataset) {
	global_dataset = dataset;
	struct fann_train_data *train_data = fann_create_train_from_callback (dataset-> num_items, NEURONS_INPUT_LAYER, strlen (PHONEME), callback_training);
	fann_train_on_data (network, train_data, 1000000, 1000, 0.00001);
	fann_save (network, "network.fann");
}

void callback_training (unsigned num, unsigned num_input, unsigned num_output, fann_type *input , fann_type *output) {
	char phoneme_str [2] = {'X', '\0'};
	phoneme_str [0] = global_dataset-> items [num].phoneme;
	fann_type *expected_vector = get_result_vector (phoneme_str);
	memcpy (input, global_dataset-> items [num].data_flat, sizeof (fann_type) * num_input);
	memcpy (output, expected_vector, sizeof (fann_type) * num_output);
	free (expected_vector);
}
