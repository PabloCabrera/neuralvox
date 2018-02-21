<?php
require ("config.php");

@mkdir ("raw");
@mkdir ("web/wav");
@mkdir ("web/png");

$in_file = fopen ("frases.txt", "r");
$out_file = fopen ("output.txt", "w");
$list_file = fopen ("wav_list.txt", "w");
$line_num = 1;
while ($line = fgets ($in_file)) {
	$output = null;
	$wav_filename = "wav/frase_$line_num.wav";
	exec ("espeak -v mb-es1 -x -w $wav_filename \"$line\"", $output);
	$out_str = implode (" ", $output);
	fwrite ($out_file, "$out_str\n");
	fwrite ($list_file, "$wav_filename\n");
	$line_num++;
}
foreach (glob ("dataset/wav/*.wav") as $filename) {
	$wav_filename = "wav/frase_$line_num.wav";
	copy ($filename, $wav_filename);
	fwrite ($list_file, "$wav_filename\n");
	$line_num++;
}

fclose ($list_file);
fclose ($out_file);
fclose ($in_file);

exec ("$SPECTVOX_COMMAND wav_list.txt");
foreach (glob ("wav/*.wav") as $filename) {
	rename ($filename, str_replace ("wav/", "web/wav/", $filename));
}
foreach (glob ("wav/*.png") as $filename) {
	rename ($filename, str_replace ("wav/", "web/png/", $filename));
}
foreach (glob ("wav/*.raw") as $filename) {
	rename ($filename, str_replace ("wav/", "raw/", $filename));
}
