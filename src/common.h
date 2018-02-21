#define NEURONS_INPUT_LAYER 32
#define PHONEME "abdefgijklmnopRr*stTuwx"
#define SPECTROGRAM_WINDOW 128
#define SPECTROGRAM_OFFSET_START 0
#define SPECTROGRAM_OFFSET_END 4096
#define SPECTROGRAM_COLOR 0
#define MEAN_WEIGHT 5
#define SHARP_WEIGHT 1

long load_raw_file_data (char *filename, double **data_buffer);
fann_type *flat_data (double *data, long data_length);
fann_type *flat_data_color (double *data, long data_length);
fann_type *flat_data_grayscale (double *data, long data_length, unsigned out_length);
fann_type *get_means_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_sharp_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_max_histogram (double *data, long data_length, unsigned num_freqs);
fann_type *get_result_vector (char *phoneme);
char *result_vector_to_string (fann_type *vector, fann_type threshold);
char *flat_data_to_string (fann_type *flatted_data, unsigned length);
