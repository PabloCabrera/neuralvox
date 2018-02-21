#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fann.h>
#include <png.h>

#include "common.h"

#define TRAINING_PRONUNCTATION_FILE "pronunctiation.txt"
#define TRAINING_RAW_DIR "raw"
#define BIFLAT_DATA_OFFSET 0.2

void process_file (FILE *list);
long load_word_data (char *word, double **buffer);
void generate_bicolor_spectrogram (double *data, long data_length, long cut_pos, bool ordered, char *word);
void write_png (unsigned char *data, long width, long height, FILE *file, unsigned color);

int main (int argc, char *args[]) {
		
	FILE *list = fopen (TRAINING_PRONUNCTATION_FILE, "r");
	process_file (list);
	fclose (list);
}

void process_file (FILE *list) {
	char *word = NULL;
	char *line = NULL;
	size_t line_length;
	double *tmp_data;

	while (getline (&line, &line_length, list) > 0) {
		word = line;
		char *delimiter = index (line, '|');
		delimiter[0] = '\0';
		char *pronunctiation = delimiter + 2;
		long data_length = load_word_data (word, &tmp_data);
		double *blurred = blur_data (tmp_data, data_length);
		long cut_pos = biflat_best_cut (blurred, data_length, BIFLAT_DATA_OFFSET);

		if (DEBUG_MODE) {
			printf ("DEBUG  %s\t", word);
		}

		bool ordered = biflat_compare (blurred, cut_pos, blurred + cut_pos, data_length - cut_pos);
		generate_bicolor_spectrogram (tmp_data, data_length, cut_pos, ordered, word);

		free (blurred);
		free (tmp_data);
	}
}

long load_word_data (char *word, double **buffer) {
	char raw_filename[1024];
	//char *raw_filename = malloc (strlen("/.raw$") + strlen (TRAINING_RAW_DIR) + strlen (word));
	strcpy (raw_filename, TRAINING_RAW_DIR);
	strcat (raw_filename, "/");
	strcat (raw_filename, word);
	strcat (raw_filename, ".raw");
	long total_readed = load_raw_file_data (raw_filename, buffer);

	//free (raw_filename);
	return total_readed;
}

void generate_bicolor_spectrogram (double *data, long data_length, long cut_pos, bool ordered, char *word) {
	char *filename = malloc (strlen ("png_bf/.png$") + strlen (word));
	sprintf (filename, "png_bf/%s.png", word);
	FILE *file = fopen (filename, "wb");
	long width = data_length / SPECTROGRAM_WINDOW;
	long height = SPECTROGRAM_WINDOW;
	unsigned char *pixels = malloc (3 * width * height * sizeof (unsigned char));
	long x, y;
	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			int pos_data = x * SPECTROGRAM_WINDOW + y * (((float)height)/((float)SPECTROGRAM_WINDOW));
			bool before_cut = cut_pos/SPECTROGRAM_WINDOW < x;
			bool high = ordered? !before_cut: before_cut;
			double r = high? (data[pos_data]): 0;
			double g = 0.2 * (data[pos_data]);
			double b = high? 0: (data[pos_data]);
			pixels [3*((height-y-1)*width + x)] = (unsigned char)(255 * (r));
			pixels [3*((height-y-1)*width + x)+1] = (unsigned char)(255 * (g));
			pixels [3*((height-y-1)*width + x)+2] = (unsigned char)(255 * (b));
		}
	}
	write_png (pixels, width, height, file, true);

	free (pixels);
	fclose (file);
	free (filename);
}

void write_png (unsigned char *data, long width, long height, FILE *file, unsigned color) {
	png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		fprintf (stderr, "Error, invalid png pointer");

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fprintf (stderr, "Error invalid info pointer");
	}

	png_init_io (png_ptr, file);
	int colortype = color? PNG_COLOR_TYPE_RGB: PNG_COLOR_TYPE_GRAY;

	png_set_IHDR (
		png_ptr, info_ptr,
		width, height,
		8, // color depth
		colortype,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);

	png_write_info (png_ptr, info_ptr);
	
	if (color) {
		// Allocate memory for one row (3 bytes per pixel - RGB)
		png_byte *buf = malloc(4 * width * sizeof(png_byte));

		// Write image data
		long col, row;
		for (row=0 ; row < height; row++) {
			for (col=0 ; col < width; col++) {
				buf[3*col] = data[3*(row*width + col)];
				buf[3*col + 1] = data[3*(row*width + col)+1];
				buf[3*col + 2] = data[3*(row*width + col)+2];
			}
			png_write_row(png_ptr, buf);
		}
		free (buf);

	} else {
		png_byte *buf = (png_byte*) malloc (sizeof (png_byte) * width);
		int row, col;
		for (row = 0; row < height; row++) {
			for (col = 0; col < width; col++) {
				buf[col] = data[row*width + col];
			}
			png_write_row (png_ptr, buf);
		}
		free (buf);
	}

	png_write_end (png_ptr, NULL);
	png_free_data (png_ptr, info_ptr, PNG_FREE_ALL, -1);
	png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
}
