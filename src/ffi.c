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
// #include "ssml.h"
#include "espeak-ng/speak_lib.h"
#include "libespeak-ng/readclause.h"
#include "libespeak-ng/voice.h"

#include "libespeak-ng/fifo.h"
#include "libespeak-ng/wavegen.h"
#include "libespeak-ng/synthesize.h"
#include "libespeak-ng/synthdata.h"
#ifdef NO_STD
#include "libc.h"
#endif

static espeak_ERROR status_to_espeak_error(espeak_ng_STATUS status)
{
	switch (status)
	{
	case ENS_OK:                     return EE_OK;
	case ENS_SPEECH_STOPPED:         return EE_OK;
	case ENS_VOICE_NOT_FOUND:        return EE_NOT_FOUND;
	case ENS_MBROLA_NOT_FOUND:       return EE_NOT_FOUND;
	case ENS_MBROLA_VOICE_NOT_FOUND: return EE_NOT_FOUND;
	case ENS_FIFO_BUFFER_FULL:       return EE_BUFFER_FULL;
	default:                         return EE_INTERNAL_ERROR;
	}
}

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

void ffi_sanity() {
	printf("hello world from C land!\n");
}

int espeak_ffi_setup(t_espeak_callback* SynthCallback) {
	int param;
	int srate = 8000; // default sample rate 22050 Hz -- looks like a small "project" to modify to 8000Hz

	// It seems that the wctype functions don't work until the locale has been set
	// to something other than the default "C".  Then, not only Latin1 but also the
	// other characters give the correct results with iswalpha() etc.
    // setlocale(LC_CTYPE, "");
	printf("hello world from C land!\n");

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
espeak_ng_STATUS espeak_ffi_synth(char *text, unsigned int size, void *user_data) {
	int synth_flags = espeakCHARS_AUTO | espeakPHONEMES | espeakENDPAUSE;
    return espeak_Synth(text, size, 0, POS_CHARACTER, 0, synth_flags, NULL, user_data);
}

espeak_ng_STATUS espeak_ffi_sync(void) {
    return espeak_ng_Synchronize();
}

ESPEAK_API espeak_ERROR espeak_Synth(const void *text, size_t size,
                                     unsigned int position,
                                     espeak_POSITION_TYPE position_type,
                                     unsigned int end_position, unsigned int flags,
                                     unsigned int *unique_identifier, void *user_data)
{
	return status_to_espeak_error(espeak_ng_Synthesize(text, size, position, position_type, end_position, flags, unique_identifier, user_data));
}
