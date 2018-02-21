<?php
require ("config.php");
	
function main () {
	$words = get_words ();
	$syllables = get_syllables ();
	$all_words = array_merge ($words, $syllables);
	generate_data ($all_words);
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

	
function generate_data ($words) {	
	global $SPECTVOX_COMMAND;
	create_dirs ();
	$pronunctiation_file = fopen ("pronunctiation.txt", "w");
	$wavlist_file = fopen ("wav_list.txt", "w");
	$count = 0;
	
	echo "Generando archivos de audio\n";
	foreach ($words as $word) {
		$retval = -1;
		$found = [];
		$wav_filename = "wav/$word.wav";
		$output = [];
		/*
		$pitch = rand (33, 66);
		$speed = rand (135, 170);
		$amplitude = rand (50, 150);
		*/
		$pitch = 50;
		$speed = 160;
		$amplitude = 100;
		$voice = "mb-es" . rand(1, 2);
		exec ("espeak -v $voice -p $pitch -s $speed -a $amplitude -x -w $wav_filename $word", $output);
		$pronunctiation = $output [0];
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

function create_dirs () {
	@mkdir ("wav");
	@mkdir ("raw");
	@mkdir ("png");
}

main ();
