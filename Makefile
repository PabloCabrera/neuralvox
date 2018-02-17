MATH_FLAGS=-lm
FANN_FLAGS=-lfann

all: bin/train bin/train2 bin/test

bin/train: src/train.c src/common.c src/common.h
	gcc src/common.c src/train.c ${MATH_FLAGS} ${FANN_FLAGS} -o bin/train

bin/train2: src/train2.c src/common.c src/common.h
	gcc src/common.c src/train2.c ${MATH_FLAGS} ${FANN_FLAGS} -o bin/train2

bin/test: src/test.c src/common.c src/common.h
	gcc src/common.c src/test.c ${MATH_FLAGS} ${FANN_FLAGS} -o bin/test

