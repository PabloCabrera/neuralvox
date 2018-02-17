#include <stdio.h>
#include <stdlib.h>
#include <fann.h>
#include <string.h>
#include "common.h"

long load_raw_file_data (char *filename, double **buffer, long buffer_size) {
	FILE *file = fopen (filename, "r");
	unsigned stop = 0;
	long total_readed = 0;

	if (file == NULL) {
		fprintf (stderr, "Error: No se puede leer el archivo %s\n", filename);
		return -1;
	}

	double *tmp_data = malloc (sizeof (double) * buffer_size);
	memset (tmp_data, '\0', buffer_size * sizeof (double));

	/* Skip offset pixels */
	fseek (file, SPECTROGRAM_OFFSET_START * sizeof (double), SEEK_SET);

	while (!stop) {

		long readed = fread (tmp_data + total_readed, sizeof (double), SPECTROGRAM_WINDOW, file);
		total_readed += readed;

		if (feof (file)) {
			stop = 1;
		} else if (total_readed + SPECTROGRAM_WINDOW > buffer_size) {
			stop = 1;
			//fprintf (stderr, "Warning: Archivo demasiado grande: %s.\n", filename);
		}
	}
	
	fclose (file);
	*buffer = tmp_data;
	return (total_readed) - (SPECTROGRAM_OFFSET_END);
}

fann_type *flat_data (double *data, long data_length) {
	unsigned num_freqs = NEURONS_INPUT_LAYER;
	fann_type *flatted_data = malloc (NEURONS_INPUT_LAYER * sizeof (fann_type));
	//fann_type *means = get_means_histogram (data, data_length, num_freqs);
	fann_type *sharp = get_sharp_histogram (data, data_length, num_freqs);

	//memcpy (flatted_data, means, num_freqs * sizeof (fann_type));
	memcpy (flatted_data, sharp, num_freqs * sizeof (fann_type));

	//free (means);
	free (sharp);
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

fann_type *get_sharp_histogram (double *data, long data_length, unsigned num_freqs) {

	long i;
	unsigned freq;
	fann_type *sharp = malloc (num_freqs * sizeof (fann_type));

	for (freq=0; freq < num_freqs; freq++) {
		sharp [freq] = 0;
	}

	double prev_pixel = 0.5;
	for (i=0; i < data_length; i++) {
		freq = (i % SPECTROGRAM_WINDOW) / (SPECTROGRAM_WINDOW/num_freqs);
		sharp [freq] += fabsf (prev_pixel - data [i]);
		prev_pixel = data [i];
	}

	for (freq=0; freq < num_freqs; freq++) {
		sharp [freq] = SHARP_WEIGHT * (sharp [freq] / (data_length/SPECTROGRAM_WINDOW));
	}

	return sharp;
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

char *result_vector_to_string (fann_type *vector, fann_type threshold) {
	int num_phoneme_symbols = strlen (PHONEME);
	char *phoneme = malloc (sizeof (char) * (num_phoneme_symbols +1));
	int out_pos = 0;
	int pos;
	for (pos=0; pos < num_phoneme_symbols; pos++) {
		if (vector [pos] > threshold) {
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