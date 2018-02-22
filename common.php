<?php

$SYNTHESIZER = "espeak";
$GENERATE_WORDS = true;
$GENERATE_SYLLABLES = true;
$GENERATE_TESTING = true;

$common_h = explode ("\n", file_get_contents ("src/common.h"));
$SPECTVOX_ARGS = "-pr";
$color_def = preg_grep ("/.*define SPECTROGRAM_COLOR.*$/", $common_h);
if (preg_match ("/^#define +SPECTROGRAM_COLOR +1.*/", $color_def[1])) {
	$SPECTVOX_ARGS = "-prc";
}

$SPECTVOX_COMMAND = "spectvox $SPECTVOX_ARGS ";

