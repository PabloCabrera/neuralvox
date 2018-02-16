Slices = [];
SliceUpdater = null;
Audio = null;

function init () {
	init_select ();
	init_spectrogram ();
}

function init_select () {
	var select = document.querySelector ("select.spectrogram_select");
	for (var num=0; num < Neural.length; num++) {
		var option = document.createElement ("OPTION");
		option.value = num;
		option.textContent = Neural[num].wav;
		select.appendChild (option);
	}
	select.value = 0;
}

function init_spectrogram () {
	stop_slice_updater ();
	var selected = document.querySelector ("select.spectrogram_select").value;
	set_image (Neural[selected].image);
	set_audio (Neural[selected].wav);
	clear_slices ();
	create_slices (Neural[selected].slices, Neural[selected].spectrogram_width);
}

function set_image (image) {
	document.querySelector(".image_container").style.backgroundImage = "url('" + image + "')"
}

function set_audio (wav) {
	Audio = document.querySelector("audio")
	Audio.src = wav;
}

function clear_slices () {
	Slices = [];
	var slices = document.querySelectorAll (".slice");
	slices.forEach (function (slice) {
		slice.parentNode.removeChild (slice);
	});
}

function create_slices (slices, spectrogram_width) {
	slices.forEach(function (slice) {
		var ul = document.querySelector (".image_container ul");
		var li = document.createElement ("LI");
		li.className = "slice";
		li.innerHTML = format_phoneme (slice.text);
		li.style.width = scale_slice (slice.width, spectrogram_width);
		li.style.left = scale_slice (slice.start, spectrogram_width);
		ul.appendChild (li);

		var slice_js = {
			node: li,
			start: (slice.start/spectrogram_width),
			end: ((slice.start + slice.width) / spectrogram_width)
		};
		Slices.push (slice_js);
	});
}

function format_phoneme (phoneme) {
	var letters = phoneme.toUpperCase().replace("*", "R").replace("J", "Y").replace("X", "J");
	var out="";
	for (var i=0; i < letters.length; i++) {
		out += letters[i] + "<br />";
	}
	return out;
}

function scale_slice (value, spectrogram_width) {
	return (100 * value / spectrogram_width) + "%";
}

function start_slice_updater () {
	SliceUpdater = window.setInterval (update_slices, 20);
}

function stop_slice_updater () {
	if (SliceUpdater != null) {
		window.clearInterval (SliceUpdater);
		SliceUpdater = null;
	}
}

function update_slices () {
	var currentPos = Audio.currentTime / Audio.duration;
	for (var i = 0; i < Slices.length; i++) {
		if (Slices[i].start <= currentPos && Slices[i].end > currentPos) {
			Slices[i].node.classList.add ("active");
		} else {
			Slices[i].node.classList.remove ("active");
		}
	}
	
}

function onSpectrogramSelectChanged () {
	init_spectrogram ();
}

function onAudioPlay () {
	start_slice_updater ();
}


window.onload = init;
