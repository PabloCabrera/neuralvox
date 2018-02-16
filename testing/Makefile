MATH_FLAGS=-lm
FANN_FLAGS=-lfann

all: bin/test

bin/test: src/test.c
	gcc src/test.c ${FFTW_FLAGS} ${MATH_FLAGS} ${FANN_FLAGS} -o bin/test

