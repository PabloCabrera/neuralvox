<?php
$dataset = [
	"a" => "bala cama daga maracana canalla arrayán piara jua zarza falla gata lana salva guapa",
	"b" => "bobo blob bueba brieva baobab babub vulva bilbo bebe babob buba bubu bi be babab",
	"d" => "dado dedadi didod dradad drede dlado dudid udida dudrido dadedo dodade drada dodad",
	"e" => "ele mele tele dreme que me des tres vez mes trece ejerce mece te re el le ve",
	"f" => "fa fe fi fo fu fufa fafi fifa faf fofof faf fefi fafef fofaf fufu faf fefi fu",
	"g" => "gaga gue guigo guga ag eguig guga gague gu ga gag guegui goga guga gago gog gague",
	"i" => "si mi ni pi risi tiqui riqui misti listri trispi krispis cis mais sois lius rios",
	"j" => "cuis fiu tui pie cien miel muisien drie frie ieg ies siefui guie lie ties pius",
	"k" => "caque coca coco cac crac cococrico cra ca clo clucu co cuis caes coc quico quica",
	"l" => "la lola lela lilo luli blula flale flul loal lalo lie liu loila lalo lulile le ",
	"m" => "mamo muma mimoam mamom memamu mumima mamem memoma mumima mame meme mema mami mum",
	"n" => "nano nuna nun nano nene neni nine nenon nona nenano nunu nanoni ninina nano nun",
	"o" => "nosotros no somos como los orozco yo los conozco son ocho los monos pocho toto cholo",
	"p" => "papu pipa pepa popo pupa piepa prapo piepra propio pupi pipi pepe pepa papop pop puip",
	"R" => "rorro rey rurror horror rarra rer reirra rarru ru ra re rerrirro ru ra ra re",
	"r" => "aro ora uro ori arar era ere eri iro aru uru urar er ir or ur ar orer orer arur ar",
	"*" => "atro otra tra tre trotra tru tritro bra cra brecra frecra frutra crotra triupra",
	"s" => "sesa sos sus cesto zoso si sauce susi sis sos sus sas zas ces zis ciciso susace",
	"t" => "tateti tu ta tra treta tatitro tla tota tre tu ti tato tuti tutia tatotlo titu",
	"u" => "uru ubugu ucusu sucutru gukuru mutucu cu bunubu mukupu piu plu tru lu mu gunu",
	"w" => "wa ue iu hue eu au ouch iu ui we wo wu wax war aur aum wep wi wuim mui wan peu",
	"x" => "ja je ji jo ju jajo joja jaji juja ju gegi gige jige ju je je ja ju ji jajo ju",
];

@mkdir ("wav");
@mkdir ("raw");
@mkdir ("png");

$list_file = fopen ("wav_list.txt", "w");

foreach ($dataset as $phoneme => $sample_text) {
	$phon = ($phoneme == "*"? "Q": $phoneme);
	$wav_filename = "wav/sample_$phon.wav";
	fwrite ($list_file, "$wav_filename\n");
	$output = [];
	exec ("espeak -v mb-es1 -x -w $wav_filename \"$sample_text\"", $output);
}

fclose ($list_file);

$output = [];
exec ("spectvox -p -r wav_list.txt", $output);

foreach (glob ("wav/*.png") as $filename) {
	rename ($filename, str_replace ("wav/", "png/", $filename));
}
foreach (glob ("wav/*.raw") as $filename) {
	rename ($filename, str_replace ("wav/", "raw/", $filename));
}
