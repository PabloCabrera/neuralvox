#define PHONEME "abdefgHiJklLmnoprstTuwYx"
#define SPECTROGRAM_COLOR 0
#define NUM_CHANNELS (1+2*(SPECTROGRAM_COLOR))
#define SPECTROGRAM_WINDOW 128
#define NEURONS_INPUT_LAYER (16*2*NUM_CHANNELS)
#define USE_BIFLAT 0
#define DEBUG_MODE 0

long load_raw_file_data (char *filename, double **data_buffer);
fann_type *flat_data (double *data, long data_length, unsigned out_length);
fann_type *flat_data_color (double *data, long data_length, unsigned out_length);
fann_type *flat_data_grayscale (double *data, long data_length, unsigned out_length);
fann_type *get_means_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_sharp_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_max_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_result_vector (char *phoneme);
char *result_vector_to_string (fann_type *vector, fann_type threshold);
char *flat_data_to_string (fann_type *flatted_data, unsigned length);
fann_type *biflat_data (double *data, long data_length, unsigned out_length, double offset);
long biflat_best_cut (double *data, long data_length, double offset);
long biflat_best_cut_color (double *data, long data_length, double offset);
long biflat_best_cut_grayscale (double *data, long data_length, double offset);
double center_of_mass (double *data, long data_length);
fann_type center_of_mass_ft (fann_type *data, long data_length);
fann_type *biflat_combine (fann_type *d1, fann_type *d2, long length, bool ordered);
bool biflat_compare (double *d1, long d1_length, double *d2, long d2_length);
double *blur_data (double *data, long data_length);
