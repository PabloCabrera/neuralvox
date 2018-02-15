MATH_FLAGS=-lm
FANN_FLAGS=-lfann

all: bin/train

bin/train: src/train.c
	gcc src/train.c ${FFTW_FLAGS} ${MATH_FLAGS} ${FANN_FLAGS} -o bin/train

