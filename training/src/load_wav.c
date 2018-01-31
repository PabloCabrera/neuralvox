#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>

void print_wav_info (struct SF_INFO *info, double *buffer) {
	fprintf( stderr, "Sample rate: %d\n", info-> samplerate );
	fprintf( stderr, "Channels: %d\n", info-> channels );
	fprintf( stderr, "Frames: %ld\n", info-> frames );
	FILE *dump = fopen ("dump.txt", "w");
	unsigned long num_samples = info-> channels * info-> frames;
	for (unsigned long i = 0; i < num_samples; i ++) {
		char *delimiter = (i % 12)? ", ": "\n";
		fprintf (dump, "%f%s", buffer[i], delimiter);
	}
	fclose (dump);
}

/* WARNING: El puntero devuelto por esta funcion en buffer debe ser liberado */
unsigned long load_wav (char *filename, double **buffer) {
	struct SF_INFO info;
	SNDFILE *file = sf_open (filename, SFM_READ, &info);
	if (info.channels != 1) {
		fprintf (stderr, "Warning: El archivo %s tiene %d canales, se esperaba 1.\n", filename, info.channels);
	}
	sf_count_t total_samples = info.channels * info.frames;
	*buffer = malloc (sizeof (double) * total_samples);
	sf_count_t readed_samples = sf_read_double (file, *buffer, total_samples);
	#ifdef DEBUG
		print_wav_info (&info, *buffer);
		fprintf (stderr, "INFO: Leídos %d samples\n", readed_samples);
	#endif
	if (readed_samples < info.frames * info.channels) {
		fprintf (stderr, "Warning: De un total de %ld samples, se han leido solo %ld\n", info.frames, (long) 1 * readed_samples);
	}
	return (unsigned long) readed_samples;
}
