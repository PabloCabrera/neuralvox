#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fann.h>
#include "common.h"

#define NEURONS_HIDDEN_LAYER 64

/* DATA TYPES */
struct training_item {
	char phoneme;
	fann_type *data_flat;
};

struct training_dataset {
	long num_items;
	struct training_item *items;
};

/* FUNCTION PROTOTYPES */
struct fann *get_network ();
struct training_dataset *get_training_dataset ();
void train_network (struct fann *network, struct training_dataset *dataset);


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
	return NULL;
}

void train_network (struct fann *network, struct training_dataset *dataset) {
}
