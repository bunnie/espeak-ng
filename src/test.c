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

void dump_param_stack(void) {
    int i, j;
    for (i = 0; i < N_PARAM_STACK; i++) {
        printf("param_stack[%d]\n", i);
        printf("  type: %d\n", param_stack[i].type);
        printf("  parameter: ");
        for (j = 0; j < N_SPEECH_PARAM; j++) {
            printf("%02x ", param_stack[i].parameter[j]);
        }
        printf("\n");
    }
}
void dump_espeak_VOICE(espeak_VOICE *voice) {
    printf("name: %s", voice->name);
    printf("languages: %s", voice->languages);
    printf("identifier: %s", voice->identifier);
    printf("gender: %d", voice->gender);
    printf("age: %d", voice->age);
    printf("variant: %d", voice->variant);
}

int main(int argc, char **argv) {
    espeak_VOICE voice_select;

    printf("hello world!\n");
	espeak_ng_InitializePath("/usr/local/share/espeak-ng-data");
	espeak_ng_ERROR_CONTEXT context = NULL;

	int param;
	int srate = 22050; // default sample rate 22050 Hz -- looks like a small "project" to modify to 8000Hz

	// It seems that the wctype functions don't work until the locale has been set
	// to something other than the default "C".  Then, not only Latin1 but also the
	// other characters give the correct results with iswalpha() etc.
    setlocale(LC_CTYPE, "");

	espeak_ng_STATUS result = LoadPhData(&srate, context);
	if (result != ENS_OK) {
        printf("couldn't load phoneme data");
		exit(0);
    }

	WavegenInit(srate, 0);
	LoadConfig();

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

#ifdef USE_ASYNC
	fifo_init();
#endif

	option_phonemes = 0;
	option_phoneme_events = 0;

    dump_param_stack();
	if (result != ENS_OK) {
		espeak_ng_PrintStatusCodeMessage(result, stderr, context);
		espeak_ng_ClearErrorContext(&context);
		exit(1);
	}

    result = espeak_ng_InitializeOutput(0x0002, 0, NULL); // PLAYBACK_MODE -> ENOUTPUT_MODE_SPEAK_AUDIO -> 0x0002
    int samplerate = espeak_ng_GetSampleRate();
    printf("sample rate: %d\n", samplerate);

    char voicename[40];
    strcpy(voicename, ESPEAKNG_DEFAULT_VOICE);
    printf("default voice: %s\n", voicename);

    result = espeak_ng_SetVoiceByName(voicename);
	if (result != ENS_OK) {
		memset(&voice_select, 0, sizeof(voice_select));
		voice_select.languages = voicename;
        dump_espeak_VOICE(&voice_select);

        const char *voice_id;
        int voice_found;

        voice_id = SelectVoice(&voice_select, &voice_found);
        if (voice_found == 0) {
            printf("voice not found\n");
            exit(0);
        }
        LoadVoiceVariant(voice_id, 0);
        DoVoiceChange(voice);
        SetVoiceStack(&voice_select, "");
	}
	int phoneme_options = 0;
	int phonemes_separator = 0;
	FILE *f_phonemes_out = stdout;

	espeak_SetPhonemeTrace(phoneme_options | (phonemes_separator << 8), f_phonemes_out);

    char *p_text = "The quick brown fox jumps over the lazy dogs";
    int size;
	int synth_flags = espeakCHARS_AUTO | espeakPHONEMES | espeakENDPAUSE;
    size = strlen(p_text);
    espeak_Synth(p_text, size+1, 0, POS_CHARACTER, 0, synth_flags, NULL, NULL);

    printf("goodbye world!\n");
}