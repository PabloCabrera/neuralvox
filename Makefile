MATH_FLAGS=-lm
FANN_FLAGS=-lfann

all: bin/train bin/test

bin/train: src/train.c
	gcc src/train.c ${MATH_FLAGS} ${FANN_FLAGS} -o bin/train

bin/test: src/test.c
	gcc src/test.c ${MATH_FLAGS} ${FANN_FLAGS} -o bin/test

