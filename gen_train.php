<?php
require ("common.php");

$CREATE_DIRS = false;
$GENERATE_WORDS = false;
$GENERATE_TESTING = true;

function main () {
	global $CREATE_DIRS, $GENERATE_WORDS, $GENERATE_TESTING;
	if ($CREATE_DIRS) {
		clean_dirs ();
		create_dirs ();
	}

	if ($GENERATE_WORDS) {
		$words = get_words ();
		generate_training_data ($words);
	}

	if ($GENERATE_TESTING) {
		generate_testing_data ();
	}
}

function get_words () {
	$words = [];
	$words_file = fopen ("wordlist_ascii.txt", "r");
	while ($line = fgets ($words_file)) {
		$word = trim ($line);
		if (strlen ($word) > 0 && strlen ($word) < 5) {
			$words[] = $word;
		}
	}
	return $words;
}

function generate_training_data ($words) {	
	global $SPECTVOX_COMMAND;
	$wavlist_file = fopen ("wav_list.txt", "w");
	$count = 0;
	
	echo "Generando archivos de audio\n";

	$pitch = 50;
	$speed = 160;
	$amplitude = 100;

	foreach ($words as $word) {
		$retval = -1;
		$found = [];
		$wav_filename = "wav/$word.wav";
		$pho_filename = "pho/$word.txt";
		generate_wav ($word, $wav_filename, $pitch, $speed, $amplitude);
		generate_pho ($word, $pho_filename, $pitch, $speed, $amplitude);
		fwrite ($wavlist_file, "$wav_filename\n");
		echo "$word ";
	}
	echo "\n";
	
	fclose ($wavlist_file);
	exec ("$SPECTVOX_COMMAND wav_list.txt");
	foreach (glob ("wav/*.png") as $filename) {
		rename ($filename, str_replace ("wav/", "png/", $filename));
	}
	foreach (glob ("wav/*.raw") as $filename) {
		rename ($filename, str_replace ("wav/", "raw/", $filename));
	}
	echo "$count\t";
	
	echo "\n";
}


function generate_testing_data () {
	global $SPECTVOX_COMMAND;

	$pitch = 50;
	$speed = 160;
	$amplitude = 100;

	$in_file = fopen ("frases.txt", "r");
	$list_file = fopen ("wav_list.txt", "w");
	$line_num = 1;
	while ($line = fgets ($in_file)) {
		$output = null;
		$wav_filename = "wav/frase_$line_num.wav";
		$raw_filename = "wav/frase_$line_num.raw";
		$pho_filename = "testing/frase_$line_num.pho";
		generate_wav ($line, $wav_filename, $pitch, $speed, $amplitude);
		generate_pho ($line, $pho_filename, $pitch, $speed, $amplitude);
		fwrite ($list_file, "$wav_filename\n");
		$line_num++;
	}

	fclose ($list_file);
	fclose ($in_file);

	exec ("$SPECTVOX_COMMAND wav_list.txt");
	foreach (glob ("wav/frase_*.wav") as $filename) {
		rename ($filename, str_replace ("wav/", "web/wav/", $filename));
	}
	foreach (glob ("wav/frase_*.png") as $filename) {
		rename ($filename, str_replace ("wav/", "web/png/", $filename));
		}
	foreach (glob ("wav/frase_*.raw") as $filename) {
		rename ($filename, str_replace ("wav/", "testing/", $filename));
	}
}

function clean_dirs () {
	foreach (["wav", "raw", "pho", "png"] as $dir) {
		if (is_dir ($dir)) {
			foreach (glob ("$dir/*") as $filename) {
				unlink ($filename);
			}
		}
	}
}

function create_dirs () {
	@mkdir ("wav");
	@mkdir ("raw");
	@mkdir ("pho");
	@mkdir ("png");
	@mkdir ("testing");
}

function generate_wav ($text, $wav_filename, $pitch, $speed, $amplitude) {
	global $SYNTHESIZER;

	if ($SYNTHESIZER == "espeak") {
		return generate_wav_espeak ($text, $wav_filename, $pitch, $speed, $amplitude);
	} elseif ($SYNTHESIZER == "festival") {
		return generate_wav_festival ($text, $wav_filename);
	} else {
		echo "Sintetizador desconocido: $SYNTHESIZER\n";
		die ();
	}
}

function generate_pho ($text, $pho_filename, $pitch, $speed, $amplitude) {
	$voice = "mb-es1";
	exec ("espeak -v $voice -p $pitch -s $speed -a $amplitude --pho --phonout=\"$pho_filename\" \"$text\"", $output);
}
function generate_wav_espeak ($text, $wav_filename, $pitch, $speed, $amplitude) {
	$voice = "mb-es1";
	exec ("espeak -v $voice -p $pitch -s $speed -a $amplitude -w $wav_filename \"$text\"", $output);
}

function generate_wav_festival ($text, $wav_filename) {
	// WARNING: audio no se sincroniza
	exec ("echo \"$text\" |iconv -f utf-8 -t iso-8859-1|text2wave /dev/stdin -o $wav_filename", $output);
	$output = [];
	return $pronunctiation;
}


main ();
