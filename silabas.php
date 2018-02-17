<?php
$pre = [
	"",
	"b", "bl", "br",
	"cl", "cr",
	"d", "dr",
	"f", "fl", "fr",
	"g", "gl", "gr",
	"j",
	"k",
	"l", "ll",
	"m",
	"n",
	"p",
	"r",
	"s",
	"t", "tl", "tr",
	"w",
	"x",
	"y",
	"z"
];

$in = [
	"a", "ai", "au",
	"e", "ei", "eu",
	"i", "ia", "ie", "io", "iu",
	"o", "oi", "ou",
	"u", "ua", "ue", "ui", "uo"
];

$post = [
	"",
	"b",
	"d",
	"f",
	"g",
	"j",
	"k",
	"l",
	"m",
	"n",
	"p",
	"r",
	"s",
	"t",
	"w",
	"x",
	"y",
	"z"
];

$pronunctiation_file = fopen ("pronunctiation.txt", "w");
$wavlist_file = fopen ("wav_list.txt", "w");
@mkdir ("wav");
@mkdir ("raw");
@mkdir ("png");

echo "Generando archivos de audio\n";

$syllables = [];

$count = 1;
foreach ($in as $middle) {
	foreach ($pre as $start) {
		$syllables[] = $start . $middle;
	}
	foreach ($post as $end) {
		$syllables[] = $middle . $end;
	}
}

foreach ($syllables as $syllable) {
	$retval = -1;
	$found = [];
	exec ("grep -q $syllable wordlist_ascii.txt", $found, $retval);
	if ($retval == 0) {
		$wav_filename = "wav/$syllable.wav";
		$output = [];
		exec ("espeak -v mb-es1 -x -w $wav_filename $syllable", $output);
		$pronunctiation = $output [0];
		fwrite ($pronunctiation_file, "$syllable|$pronunctiation\n");
		fwrite ($wavlist_file, "$wav_filename\n");
		if ($count++ % 1000 == 0) {
			fclose ($wavlist_file);
			fclose ($pronunctiation_file);
			exec ("spectvox -pr wav_list.txt");
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
}

fclose ($wavlist_file);
fclose ($pronunctiation_file);
exec ("/home/pablo/dev/spectvox/bin/spectvox -r -p wav_list.txt");
$pronunctiation_file = fopen ("pronunctiation.txt", "a");
$wavlist_file = fopen ("wav_list.txt", "w");
foreach (glob ("wav/*.png") as $filename) {
	rename ($filename, str_replace ("wav/", "png/", $filename));
}
foreach (glob ("wav/*.raw") as $filename) {
	rename ($filename, str_replace ("wav/", "raw/", $filename));
}
echo "$count\t";

echo "\n";


