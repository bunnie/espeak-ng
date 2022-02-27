#include "../config.h"
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>

#include <espeak-ng/espeak_ng.h>
#include <espeak-ng/speak_lib.h>
#include <ssml.h>
#include <speak_lib.h>
#include <readclause.h>
#include <voice.h>

#include <fifo.h>
#include <wavegen.h>
#include <synthesize.h>
#include <synthdata.h>

extern int saved_parameters[N_SPEECH_PARAM]; // Parameters saved on synthesis start
const int ffi_param_defaults[N_SPEECH_PARAM] = {
	0,   // silence (internal use)
	espeakRATE_NORMAL, // rate wpm
	100, // volume
	50,  // pitch
	50,  // range
	0,   // punctuation
	0,   // capital letters
	0,   // wordgap
	0,   // options
	0,   // intonation
	0,
	0,
	0,   // emphasis
	0,   // line length
	0,   // voice type
};

int espeak_ffi_setup(t_espeak_callback* SynthCallback) {
	int param;
	int srate = 8000; // default sample rate 22050 Hz -- looks like a small "project" to modify to 8000Hz

	// It seems that the wctype functions don't work until the locale has been set
	// to something other than the default "C".  Then, not only Latin1 but also the
	// other characters give the correct results with iswalpha() etc.
    setlocale(LC_CTYPE, "");

	espeak_ng_STATUS result = LoadPhData(&srate, NULL);
	if (result != ENS_OK) {
        return 0;
    }

	WavegenInit(srate, 0);
	//LoadConfig();

    // setup parameters
	memset(&current_voice_selected, 0, sizeof(current_voice_selected));
	SetVoiceStack(NULL, "");
	SynthesizeInit();
	InitNamedata();

	VoiceReset(0);

	for (param = 0; param < N_SPEECH_PARAM; param++)
		param_stack[0].parameter[param] = saved_parameters[param] = ffi_param_defaults[param];

	SetParameter(espeakRATE, espeakRATE_NORMAL, 0);
	SetParameter(espeakVOLUME, 100, 0);
	SetParameter(espeakCAPITALS, option_capitals, 0);
	SetParameter(espeakPUNCTUATION, option_punctuation, 0);
	SetParameter(espeakWORDGAP, 0, 0);

	option_phonemes = 0;
	option_phoneme_events = 0;


    // setup output stream
    result = espeak_ng_InitializeOutput(0x0001, 0, NULL);
    // PLAYBACK_MODE -> ENOUTPUT_MODE_SPEAK_AUDIO -> 0x0002
    // FILE -> ENOUTPUT_MODE_SYNCHRONOUS -> 0x0001
    espeak_SetSynthCallback(SynthCallback);

    /// setup voices
    char voicename[40];
    strcpy(voicename, "gmw/en");
    printf("default voice: %s\n", voicename);

    return 1;
}

// size includes trailing \0
espeak_ng_STATUS espeak_ffi_synth(char *text, unsigned int size) {
	int synth_flags = espeakCHARS_AUTO | espeakPHONEMES | espeakENDPAUSE;
    return espeak_Synth(text, size, 0, POS_CHARACTER, 0, synth_flags, NULL, NULL);
}

espeak_ng_STATUS espeak_ffi_sync(void) {
    return espeak_ng_Synchronize();
}
