#include <fann.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

#define TESTING_BUFFER_SIZE 8388608 //64 MiB
#define TESTING_THRESHOLD 0.97
#define SLICE_WIDTH 18
#define SLICE_STEP 14
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
fann_type **get_slices (double *data, unsigned num_slices, unsigned slice_width, unsigned position_step);
void generate_js_info (char *raw_filename, long data_length, unsigned num_slices, char **results, FILE *jsfile);
char *get_png_filename (char *raw_filename);
char *get_wav_filename (char *raw_filename);
void write_spectrogram_info (struct spectrogram_info *info, FILE *file);
char *get_slice_info_text (struct slice_info *info);

/* FUNCTIONS */
int main (int argc, char *args[]) {
	FILE *jsfile = fopen ("web/data.js", "w");
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
	long data_length = load_raw_file_data (filename, &data_buffer, TESTING_BUFFER_SIZE);
	long num_slices = (data_length - SPECTROGRAM_OFFSET_START - SPECTROGRAM_OFFSET_END) / (SPECTROGRAM_WINDOW * SLICE_STEP);
	fann_type **slices = get_slices (data_buffer, num_slices, SLICE_WIDTH, SLICE_STEP);
	unsigned i;
	char **str_results = malloc (num_slices * sizeof (char*));
	for (i=0; i < num_slices; i++) {
		fann_type *result = fann_run (network, slices [i]);
		str_results [i] = result_vector_to_string (result, TESTING_THRESHOLD);
		printf ("%s ", str_results [i]);
	}
	printf ("\n");

	generate_js_info (filename, data_length, num_slices, str_results, jsfile);

	for (i=0; i < num_slices; i++) {
		free (str_results[i]);
	}
}

fann_type **get_slices (double *data, unsigned num_slices, unsigned slice_width, unsigned position_step) {
	fann_type **slices = malloc (sizeof (fann_type*) * num_slices);
	unsigned i;
	for (i=0; i < num_slices; i++) {
		slices[i] = flat_data (data + (i*position_step*SPECTROGRAM_WINDOW), slice_width * SPECTROGRAM_WINDOW);
	}
	return slices;
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

