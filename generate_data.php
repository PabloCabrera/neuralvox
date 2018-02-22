<?php
require ("common.php");
	
function main () {
	global $GENERATE_WORDS, $GENERATE_SYLLABLES, $GENERATE_TESTING;
	//clean_dirs ();
	create_dirs ();
	$words = $GENERATE_WORDS? get_words (): [];
	$syllables = $GENERATE_SYLLABLES? get_syllables (): [];
	$all_words = array_merge ($words, $syllables);

	generate_training_data ($all_words);
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

function get_syllables () {
	$syllables = [];
	$pre = ["", "b", "bl", "br", "cl", "cr", "d", "dr", "f", "fl", "fr", "g", "gl", "gr", "j", "k", "l", "ll", "m", "n", "p", "r", "s", "t", "tl", "tr", "w", "x", "y", "z"];
	$in = ["a", "ai", "au", "e", "ei", "eu", "i", "ia", "ie", "io", "iu", "o", "oi", "ou", "u", "ua", "ue", "ui", "uo"];
	$post = ["", "b", "d", "f", "g", "j", "k", "l", "m", "n", "p", "r", "s", "t", "w", "x", "y", "z"];
	foreach ($in as $middle) {
		foreach ($pre as $start) {
			$syllables[] = $start . $middle;
		}
		foreach ($post as $end) {
			$syllables[] = $middle . $end;
		}
	}
	return $syllables;
}

	
function generate_training_data ($words) {	
	global $SPECTVOX_COMMAND;
	$pronunctiation_file = fopen ("pronunctiation.txt", "w");
	$wavlist_file = fopen ("wav_list.txt", "w");
	$count = 0;
	
	echo "Generando archivos de audio\n";
	foreach ($words as $word) {
		$retval = -1;
		$found = [];
		$wav_filename = "wav/$word.wav";
		$pronunctiation = generate_wav ($word, $wav_filename);
		
		fwrite ($pronunctiation_file, "$word|$pronunctiation\n");
		fwrite ($wavlist_file, "$wav_filename\n");
		if ($count++ % 1000 == 0) {
			fclose ($wavlist_file);
			fclose ($pronunctiation_file);
			exec ("$SPECTVOX_COMMAND wav_list.txt");
			$pronunctiation_file = fopen ("pronunctiation.txt", "a");
			$wavlist_file = fopen ("wav_list.txt", "w");
			foreach (glob ("wav/*.png") as $filename) {
				rename ($filename, str_replace ("wav/", "png/", $filename));
			}
			foreach (glob ("wav/*.raw") as $filename) {
				rename ($filename, str_replace ("wav/", "raw/", $filename));
			}
			echo "$count\t";
		}
	}
	
	fclose ($wavlist_file);
	fclose ($pronunctiation_file);
	exec ("$SPECTVOX_COMMAND wav_list.txt");
	$pronunctiation_file = fopen ("pronunctiation.txt", "a");
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
	$in_file = fopen ("frases.txt", "r");
	$out_file = fopen ("output.txt", "w");
	$list_file = fopen ("wav_list.txt", "w");
	$line_num = 1;
	while ($line = fgets ($in_file)) {
		$output = null;
		$wav_filename = "wav/frase_$line_num.wav";
		$out_str = generate_wav ($line, $wav_filename);
		fwrite ($out_file, "$out_str\n");
		fwrite ($list_file, "$wav_filename\n");
		$line_num++;
	}

	fclose ($list_file);
	fclose ($out_file);
	fclose ($in_file);

	exec ("$SPECTVOX_COMMAND wav_list.txt");
	foreach (glob ("wav/frase_*.wav") as $filename) {
		rename ($filename, str_replace ("wav/", "web/wav/", $filename));
	}
	foreach (glob ("wav/frase_*.png") as $filename) {
		rename ($filename, str_replace ("wav/", "web/png/", $filename));
		}
	foreach (glob ("wav/frase_*.raw") as $filename) {
		rename ($filename, str_replace ("wav/", "raw/", $filename));
	}
}

function clean_dirs () {
	foreach (["wav", "raw", "png"] as $dir) {
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
	@mkdir ("png");
}

function generate_wav ($text, $wav_filename) {
	global $SYNTHESIZER;

	if ($SYNTHESIZER == "espeak") {
		return generate_wav_espeak ($text, $wav_filename);
	} elseif ($SYNTHESIZER == "festival") {
		return generate_wav_festival ($text, $wav_filename);
	} else {
		echo "Sintetizador desconocido: $SYNTHESIZER\n";
		die ();
	}

}

function generate_wav_espeak ($text, $wav_filename) {
		$output = [];
		$pitch = 50;
		$speed = 160;
		$amplitude = 100;
		$voice = "mb-es1";
		exec ("espeak -v $voice -p $pitch -s $speed -a $amplitude -x -w $wav_filename \"$text\"", $output);
		$pronunctiation = $output [0];
		return $pronunctiation;
}

function generate_wav_festival ($text, $wav_filename) {
		exec ("echo \"$text\" |iconv -f utf-8 -t iso-8859-1|text2wave /dev/stdin -o $wav_filename", $output);

		$output = [];
		exec ("espeak -q -x $text", $output);
		$pronunctiation = $output [0];
		return $pronunctiation;
}


main ();
