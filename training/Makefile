FFTW_FLAGS=-lfftw3
MATH_FLAGS=-lm
SND_FLAGS=-lsndfile
FANN_FLAGS=-lfann

bin/train: src/train.c src/spectrogram.c src/spectrogram.h src/load_wav.c src/load_wav.h
	gcc src/spectrogram.c src/train.c src/load_wav.c ${FFTW_FLAGS} ${MATH_FLAGS} ${SND_FLAGS} ${FANN_FLAGS} -o bin/train



