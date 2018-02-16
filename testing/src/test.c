#include <fann.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 8388608 //64 MiB
#define SPECTROGRAM_OFFSET_START 0
#define SPECTROGRAM_OFFSET_END 4480
#define SPECTROGRAM_WINDOW 128
#define NEURONS_INPUT_LAYER 12
#define SLICE_WIDTH 18
#define SLICE_STEP 14
#define MEAN_WEIGHT 5
#define SHARP_WEIGHT 1
#define PHONEME "abdefgijJklmnopRr*stuwx"
#define THRESHOLD 0.97
#define NUM_EXAMPLES 8

/* DATA TYPES */
struct slice_info {
	char *text;
	unsigned width;
	unsigned start;
};

struct spectrogram_info {
	char *image_filename;
	char *wav_filename;
	unsigned spectrogram_width;
	unsigned spectrogram_height;
	unsigned num_slices;
	struct slice_info *slices;
};


/* FUNCTION PROTOTYPES */
void test_file (struct fann *network, char *filename, FILE *jsfile);
long load_raw_file_data (char *filename, double **data_buffer);
fann_type **get_slices (double *data, unsigned num_slices, unsigned slice_width, unsigned position_step);
fann_type *flat_data (double *data, long data_length);
fann_type *get_means_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_sharp_histogram (double *data, long data_length, unsigned num_freqs);
char *result_vector_to_string (fann_type *vector);
void generate_js_info (char *raw_filename, long data_length, unsigned num_slices, char **results, FILE *jsfile);
char *get_png_filename (char *raw_filename);
char *get_wav_filename (char *raw_filename);
void write_spectrogram_info (struct spectrogram_info *info, FILE *file);
char *get_slice_info_text (struct slice_info *info);

/* FUNCTIONS */
int main (int argc, char *args[]) {
	FILE *jsfile = fopen ("data.js", "w");
	fprintf (jsfile, "Neural = [\n");
	struct fann *network = fann_create_from_file ("network.fann");
	unsigned i;

	char *filename = malloc (strlen ("raw/frase_xxxx.raw$"));
	for (i=1; i <= NUM_EXAMPLES; i++) {
		sprintf (filename, "raw/frase_%d.raw", i);
		test_file (network, filename, jsfile);
		if (i < NUM_EXAMPLES) {
			fprintf (jsfile, ",\n");
		}
	}
	free (filename);
	fprintf (jsfile, "\n]\n");
}

void test_file (struct fann *network, char *filename, FILE *jsfile) {
	FILE *file = fopen (filename, "r");
	double *data_buffer;
	long data_length = load_raw_file_data (filename, &data_buffer);
	long num_slices = (data_length - SPECTROGRAM_OFFSET_START - SPECTROGRAM_OFFSET_END) / (SPECTROGRAM_WINDOW * SLICE_STEP);
	fann_type **slices = get_slices (data_buffer, num_slices, SLICE_WIDTH, SLICE_STEP);
	unsigned i;
	char **str_results = malloc (num_slices * sizeof (char*));
	for (i=0; i < num_slices; i++) {
		fann_type *result = fann_run (network, slices [i]);
		str_results [i] = result_vector_to_string (result);
		printf ("%s ", str_results [i]);
	}
	printf ("\n");

	generate_js_info (filename, data_length, num_slices, str_results, jsfile);

	for (i=0; i < num_slices; i++) {
		free (str_results[i]);
	}
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

void generate_js_info (char *raw_filename, long data_length, unsigned num_slices, char **results, FILE *jsfile) {
	char *png_filename = get_png_filename (raw_filename);
	char *wav_filename = get_wav_filename (raw_filename);
	struct spectrogram_info *info = malloc (sizeof (struct spectrogram_info));
	info-> image_filename = png_filename;
	info-> wav_filename = wav_filename;
	info-> spectrogram_width = (data_length / SPECTROGRAM_WINDOW);
	info-> spectrogram_height = SPECTROGRAM_WINDOW;
	info-> num_slices = num_slices;
	info-> slices = malloc (num_slices * sizeof (struct slice_info));

	unsigned i;
	for (i=0; i < num_slices; i++) {
		struct slice_info *slice = &(info->slices[i]);
		slice-> text = results [i];
		slice-> width = SLICE_WIDTH;
		slice-> start = (SPECTROGRAM_OFFSET_START / SPECTROGRAM_WINDOW) + (i * SLICE_STEP);
	}
	
	write_spectrogram_info (info, jsfile);

	free (info-> slices);
	free (info);
	free (png_filename);
}

char *get_png_filename (char *raw_filename) {
	unsigned filename_length = strlen (raw_filename);
	char *png_filename = malloc (filename_length +1);
	strcpy (png_filename, "png/");
	strcat (png_filename, raw_filename + 4);
	strcpy (png_filename + filename_length -4, ".png");
	return png_filename;
}

char *get_wav_filename (char *raw_filename) {
	unsigned filename_length = strlen (raw_filename);
	char *png_filename = malloc (filename_length +1);
	strcpy (png_filename, "wav/");
	strcat (png_filename, raw_filename + 4);
	strcpy (png_filename + filename_length -4, ".wav");
	return png_filename;
}


void write_spectrogram_info (struct spectrogram_info *info, FILE *file) {
	fprintf (file, "{\n\timage:\"%s\",\n\twav: \"%s\",\n\tspectrogram_width: %d,\n\tslices: [\n", info-> image_filename, info-> wav_filename, info-> spectrogram_width);
	unsigned i;
	for (i=0; i < (info->num_slices); i++) {
		char *slice_info_text = get_slice_info_text (&(info-> slices [i]));
		char *separator = (i+1 < info->num_slices)? ",": "";
		fprintf (file, "\t\t%s%s\n", slice_info_text, separator);
		free (slice_info_text);
	}

	fprintf (file, "\t]\n}");
}

char *get_slice_info_text (struct slice_info *info) {
	char *text = malloc (sizeof (char) * 256);
	sprintf (text, "{text: \"%s\", width: %d, start: %d}", info-> text, info-> width, info-> start);
	return text;
}

