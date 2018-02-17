<?php

@mkdir ("wav");
@mkdir ("raw");
@mkdir ("png");
@mkdir ("web/png");

$in_file = fopen ("frases.txt", "r");
$out_file = fopen ("output.txt", "w");
$list_file = fopen ("wav_list.txt", "w");
$line_num = 1;
while ($line = fgets ($in_file)) {
	$output = null;
	$wav_filename = "wav/frase_$line_num.wav";
	exec ("espeak -v es -x -w $wav_filename \"$line\"", $output);
	$out_str = implode (" ", $output);
	fwrite ($out_file, "$out_str\n");
	fwrite ($list_file, "$wav_filename\n");
	$line_num++;
}
fclose ($list_file);
fclose ($out_file);
fclose ($in_file);

exec ("spectvox -pr wav_list.txt");
foreach (glob ("wav/*.png") as $filename) {
	copy ($filename, str_replace ("wav/", "web/png/", $filename));
	rename ($filename, str_replace ("wav/", "png/", $filename));
}
foreach (glob ("wav/*.raw") as $filename) {
	rename ($filename, str_replace ("wav/", "raw/", $filename));
}
