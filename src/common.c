#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <fann.h>
#include <string.h>
#include "common.h"

long load_raw_file_data (char *filename, double **buffer) {
	FILE *file = fopen (filename, "r");

	fseek (file, 0L, SEEK_END);
	long filesize = ftell (file);
	fseek (file, 0L, SEEK_SET);
	long buffer_size = filesize / sizeof (double) ;

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


fann_type *flat_data (double *data, long data_length, unsigned out_length) {
	if (SPECTROGRAM_COLOR) {
		return flat_data_color (data, data_length, out_length);
	} else {
		return flat_data_grayscale (data, data_length, out_length);
	}
}

fann_type *flat_data_color (double *data, long data_length, unsigned out_length) {
	if (out_length % 3 != 0) {
		fprintf (stderr, "Warning: flat_data_color: output length is not divisible by 3\n");
	}
	double *data_current = malloc ((data_length/3) * sizeof (double));
	fann_type *flat_data_current;
	fann_type *flatted_data = malloc (sizeof (fann_type) * out_length);
	long channel;
	for (channel=0; channel<3; channel++) {
		long i;
		for (i=0; i < (data_length/3); i++) {
			data_current [i] = data [3*i + channel];
		}
		flat_data_current = flat_data_grayscale (data_current, data_length/3, NEURONS_INPUT_LAYER/3);
		memcpy (flatted_data + channel * out_length/3, flat_data_current, sizeof (fann_type) * out_length/3);
		free (flat_data_current);
	}
	free (data_current);
	return flatted_data;
}

fann_type *flat_data_grayscale (double *data, long data_length, unsigned out_length) {
	unsigned num_freqs = out_length;
	fann_type *flatted_data = malloc (out_length * sizeof (fann_type));
	fann_type *sharp = get_sharp_histogram (data, data_length, num_freqs);
	//fann_type *means = get_means_histogram (data, data_length, num_freqs);
	//fann_type *max = get_max_histogram (data, data_length, num_freqs);

	memcpy (flatted_data, sharp, num_freqs * sizeof (fann_type));
	//memcpy (flatted_data, means, num_freqs * sizeof (fann_type));
	//memcpy (flatted_data, max, num_freqs * sizeof (fann_type));

	free (sharp);
	//free (means);
	//free (max);
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
		means [freq] = tanh (5* (means [freq] / (data_length/SPECTROGRAM_WINDOW)));
		//means [freq] = (means [freq] / (data_length/SPECTROGRAM_WINDOW));
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
		sharp [freq] = tanh (sharp [freq] / (data_length/SPECTROGRAM_WINDOW));
	}

	return sharp;
}

fann_type *get_max_histogram (double *data, long data_length, unsigned num_freqs) {

	long i;
	unsigned freq;
	fann_type *max = malloc (num_freqs * sizeof (fann_type));

	for (freq=0; freq < num_freqs; freq++) {
		max [freq] = 0;
	}

	for (i=0; i < data_length; i++) {
		freq = (i % SPECTROGRAM_WINDOW) / (SPECTROGRAM_WINDOW/num_freqs);
		if (data[i] > max[freq]) {
			max[freq] = data[i];
		}
	}

	return max;
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
	char *ret = malloc (((length + 1)+(length/8)) * sizeof (char));
	int i;
	int out_pos = 0;
	for (i=0; i < length; i++) {
		char car = '0';
		int digit = (10 * flatted_data[i]);
		if (digit < -25) {
			car = '_';
		} else if (digit < 0) {
			car = 'z' - digit;
		} else if (digit < 10) {
			car = '0' + digit;
		} else if (digit < 35) {
			car = 'A' -10 + digit;
		} else {
			car = '+';
		}
		ret [out_pos] = car;
		out_pos++;
		if (i % 8 ==7 && i+1 < length) {
			ret [out_pos] = ' ';
			out_pos++;
		}
	}

	ret [out_pos] = '\0';
	return ret;
}

fann_type *biflat_data (double *data, long data_length, unsigned out_length, double offset) {
	if (out_length % 2 != 0) {
		fprintf (stderr, "Warning: biflat_data: output length is not divisible by 2\n");
	}
	long cut_pos = biflat_best_cut (data, data_length, offset);
	fann_type *f1 = flat_data (data, cut_pos, out_length/2);
	fann_type *f2 = flat_data (data + cut_pos, data_length - cut_pos, out_length/2);
	bool ordered = biflat_compare (data, cut_pos, data + cut_pos, data_length - cut_pos);
	fann_type *combined_data = biflat_combine (f1, f2, out_length/2, ordered);
	free (f1);
	free (f2);

	return combined_data;
}

long biflat_best_cut (double *data, long data_length, double offset) {
	return data_length / 2;
	/*
	if (offset < 0.0 || offset > 0.5) {
		fprintf (stderr, "Error: biflat_best_cut: offset outside range [0.0 - 0.5]\n");
		return (data_length/2);
	} else if (SPECTROGRAM_COLOR) {
		return biflat_best_cut_color (data, data_length, offset);
	} else {
		return biflat_best_cut_grayscale (data, data_length, offset);
	}
	*/
}

long biflat_best_cut_color (double *data, long data_length, double offset) {
	fprintf (stderr, "Warning: biflat_best_cut_color unimplemented\n");
	return data_length /2; //FIXME
}

long biflat_best_cut_grayscale (double *data, long data_length, double offset) {
	double *blurred_data = blur_data (data, data_length);
	long num_cols = data_length / SPECTROGRAM_WINDOW;
	long first_col = num_cols * offset;
	long last_col = num_cols * (1.0 - offset) -1;
	long col;
	double max_diff = 0;
	long cut_col = first_col;
	for (col = first_col; col < last_col; col++) {
		double actual_diff = 0;
		double current_col = 0;
		double next_col = 0;
		unsigned row;
		for (row=0; row < SPECTROGRAM_WINDOW; row++) {
			current_col += blurred_data [col*SPECTROGRAM_WINDOW+row];
			next_col += blurred_data [((col+1)*SPECTROGRAM_WINDOW)+row];
			actual_diff += fabs (current_col - next_col);
		}
		if (actual_diff > max_diff) {
			max_diff = actual_diff;
			cut_col = col;
		}
	}

	if (DEBUG_MODE) {
		printf ("best_cut: %ld ", cut_col);
	}

	free (blurred_data);
	return (cut_col * SPECTROGRAM_WINDOW);
}

double center_of_mass (double *data, long data_length) {
	double mass = 0;
	double mass_pos = 0;
	long i;
	for (i=0; i < data_length; i++) {
		mass += data[i];
		mass_pos += data[i] * (1+i);
	}
	return mass_pos/mass;
}

fann_type center_of_mass_ft (fann_type *data, long data_length) {
	fann_type mass = 0;
	fann_type mass_pos = 0;
	long i;
	for (i=0; i < data_length; i++) {
		mass += data[i];
		mass_pos += data[i] * (1+i);
	}
	return mass_pos/mass;
}

fann_type *biflat_combine (fann_type *d1, fann_type *d2, long length, bool ordered) {
	fann_type *combined_data = malloc (sizeof (fann_type) * 2 * length);
	fann_type *first = ordered? d1: d2;
	fann_type *second = ordered? d2: d1;
	memcpy (combined_data, first, length * sizeof (fann_type));
	memcpy (combined_data + length, second, length * sizeof (fann_type));
	return combined_data;
}

bool biflat_compare_grayscale (double *d1, long d1_length, double *d2, long d2_length) {
	fann_type *h1 = get_sharp_histogram (d1, d1_length, 8);
	fann_type *h2 = get_sharp_histogram (d2, d2_length, 8);
	
	double center1 = center_of_mass_ft (h1, 8);
	double center2 = center_of_mass_ft (h2, 8);

	free (h1);
	free (h2);

	if (DEBUG_MODE) {
		printf ("biflat_compare: %d, (%.3f|%.3f) \n", center1 < center2, center1, center2);
	}

	return center1 < center2;
}

bool biflat_compare_color (double *d1, long d1_length, double *d2, long d2_length) {
	fprintf (stderr, "Warning: biflat_compare_color unimplemented\n");
	return true;
}
 
bool biflat_compare (double *d1, long d1_length, double *d2, long d2_length) {
	if (SPECTROGRAM_COLOR) {
		return biflat_compare_color (d1, d1_length, d2, d2_length);
	} else {
		return biflat_compare_grayscale (d1, d1_length, d2, d2_length);
	}
}
double *blur_data (double *data, long data_length) {
	double *blurred = malloc (sizeof (double) * data_length);
	long width = data_length/SPECTROGRAM_WINDOW;
	long height = SPECTROGRAM_WINDOW;

	long x, y;
	for (x=2; x < width-2; x++) {
		for (y=0; y < height; y++) {
			long pos = (x*SPECTROGRAM_WINDOW)+y;
			if (x<2 || width-x<3) {
				blurred [pos] = data [pos];
			} else {
				blurred [pos] = 
					0.2 * data [pos]
					+ 0.2 * data [pos +1*SPECTROGRAM_WINDOW]
					+ 0.2 * data [pos -1*SPECTROGRAM_WINDOW]
					+ 0.2 * data [pos +2*SPECTROGRAM_WINDOW]
					+ 0.2 * data [pos -2*SPECTROGRAM_WINDOW];
			}
			
		}
	}
	return blurred;
}
