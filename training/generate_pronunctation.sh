#!/bin/bash

cd transcription

for FILENAME in *; do
	echo "$FILENAME|$(cat "$FILENAME")" >> ../pronunctiation.txt;
done

cd ..
