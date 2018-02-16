#include <fann.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 8388608 //64 MiB
#define SPECTROGRAM_OFFSET_START 0
#define SPECTROGRAM_OFFSET_END 6144
#define SPECTROGRAM_WINDOW 128
#define SLICE_WIDTH 32
#define SLICE_STEP 24
#define MEAN_WEIGHT 5
#define STDEV_WEIGHT 0.1
#define NEURONS_INPUT_LAYER 16
#define PHONEME "abdefgijJklmnopRr*stuwx"
#define THRESHOLD 0.9
#define NUM_PHRASES 8

/* FUNCTION PROTOTYPES */
void test_file (struct fann *network, char *filename);
long load_raw_file_data (char *filename, double **data_buffer);
fann_type **get_slices (double *data, unsigned num_slices, unsigned slice_width, unsigned position_step);
fann_type *flat_data (double *data, long data_length);
fann_type *get_means_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_stdev_histogram (double *data, long data_length, fann_type *means, unsigned num_freqs);
char *result_vector_to_string (fann_type *vector);



/* FUNCTIONS */
int main (int argc, char *args[]) {
	struct fann *network = fann_create_from_file ("network.fann");
	test_file (network, "raw/frase_1.raw");
	test_file (network, "raw/frase_2.raw");
	test_file (network, "raw/frase_3.raw");
	test_file (network, "raw/frase_4.raw");
	test_file (network, "raw/frase_5.raw");
	test_file (network, "raw/frase_6.raw");
	test_file (network, "raw/frase_7.raw");
	test_file (network, "raw/frase_8.raw");
}

void test_file (struct fann *network, char *filename) {
	FILE *file = fopen (filename, "r");
	double *data_buffer;
	long data_length = load_raw_file_data (filename, &data_buffer);
	long num_slices = data_length / (SPECTROGRAM_WINDOW * SLICE_STEP);
	fann_type **slices = get_slices (data_buffer, num_slices, SLICE_WIDTH, SLICE_STEP);
	unsigned i;
	for (i=0; i < num_slices; i++) {
		fann_type *result = fann_run (network, slices [i]);
		char *str_result = result_vector_to_string (result);
		printf ("[%s]", str_result);
		free (str_result);
	}
	printf ("\n");
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

fann_type **get_slices (double *data, unsigned num_slices, unsigned slice_width, unsigned position_step) {
	fann_type **slices = malloc (sizeof (fann_type*) * num_slices);
	unsigned i;
	for (i=0; i < num_slices; i++) {
		slices[i] = flat_data (data + (i*position_step*SPECTROGRAM_WINDOW), slice_width * SPECTROGRAM_WINDOW);
	}
	return slices;
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

