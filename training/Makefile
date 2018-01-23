FFTW_FLAGS=-lfftw3
MATH_FLAGS=-lm

bin/train: src/train.c src/spectrogram.c src/spectrogram.h 
	gcc src/spectrogram.c src/train.c ${FFTW_FLAGS} ${MATH_FLAGS} -o bin/train

