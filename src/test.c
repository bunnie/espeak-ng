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

int samplerate;
bool quiet = false;
unsigned int samples_total = 0;
unsigned int samples_split = 0;
unsigned int samples_split_seconds = 0;
unsigned int wavefile_count = 0;

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

FILE *f_wavfile = NULL;
void testWrite4Bytes(FILE *f, int value)
{
	// Write 4 bytes to a file, least significant first
	int ix;

	for (ix = 0; ix < 4; ix++) {
		fputc(value & 0xff, f);
		value = value >> 8;
	}
}

int OpenWavFile(char *path, int rate)
{
	static unsigned char wave_hdr[44] = {
		'R', 'I', 'F', 'F', 0x24, 0xf0, 0xff, 0x7f, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
		0x10, 0, 0, 0, 1, 0, 1, 0,  9, 0x3d, 0, 0, 0x12, 0x7a, 0, 0,
		2, 0, 0x10, 0, 'd', 'a', 't', 'a',  0x00, 0xf0, 0xff, 0x7f
	};

	if (path == NULL)
		return 2;

	while (isspace(*path)) path++;

	f_wavfile = NULL;
	if (path[0] != 0) {
		if (strcmp(path, "stdout") == 0) {
#ifdef PLATFORM_WINDOWS
			// prevent Windows adding 0x0d before 0x0a bytes
			_setmode(_fileno(stdout), _O_BINARY);
#endif
			f_wavfile = stdout;
		} else
			f_wavfile = fopen(path, "wb");
	}

	if (f_wavfile == NULL) {
		fprintf(stderr, "Can't write to: '%s'\n", path);
		return 1;
	}

	fwrite(wave_hdr, 1, 24, f_wavfile);
	testWrite4Bytes(f_wavfile, rate);
	testWrite4Bytes(f_wavfile, rate * 2);
	fwrite(&wave_hdr[32], 1, 12, f_wavfile);
	return 0;
}

void CloseWavFile(void)
{
	unsigned int pos;

	if ((f_wavfile == NULL) || (f_wavfile == stdout))
		return;

	fflush(f_wavfile);
	pos = ftell(f_wavfile);

	if (fseek(f_wavfile, 4, SEEK_SET) != -1)
		testWrite4Bytes(f_wavfile, pos - 8);

	if (fseek(f_wavfile, 40, SEEK_SET) != -1)
		testWrite4Bytes(f_wavfile, pos - 44);

	fclose(f_wavfile);
	f_wavfile = NULL;
}


static int SynthCallback(short *wav, int numsamples, espeak_EVENT *events)
{
	while (events->type != 0) {
		if (events->type == espeakEVENT_SAMPLERATE) {
			samplerate = events->id.number;
			samples_split = samples_split_seconds * samplerate;
		} else if (events->type == espeakEVENT_SENTENCE) {
			// start a new WAV file when the limit is reached, at this sentence boundary
			if ((samples_split > 0) && (samples_total > samples_split)) {
				CloseWavFile();
				samples_total = 0;
				wavefile_count++;
			}
		}
		events++;
	}

	if (f_wavfile == NULL) {
		if (samples_split > 0) {
			if (OpenWavFile("test1.wav", samplerate) != 0)
				return 1;
		} else if (OpenWavFile("test1.wav", samplerate) != 0)
			return 1;
	}

	if (numsamples > 0) {
		samples_total += numsamples;
		fwrite(wav, numsamples*2, 1, f_wavfile);
	}
	return 0;
}


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

	// fifo_init();

	option_phonemes = 0;
	option_phoneme_events = 0;

    dump_param_stack();
	if (result != ENS_OK) {
		espeak_ng_PrintStatusCodeMessage(result, stderr, context);
		espeak_ng_ClearErrorContext(&context);
		exit(1);
	}

    // setup output stream
    result = espeak_ng_InitializeOutput(0x0001, 0, NULL);
    // PLAYBACK_MODE -> ENOUTPUT_MODE_SPEAK_AUDIO -> 0x0002
    // FILE -> ENOUTPUT_MODE_SYNCHRONOUS -> 0x0001
    samplerate = espeak_ng_GetSampleRate();
    printf("sample rate: %d\n", samplerate);
    samples_split = 0;
    samples_split_seconds = 30 * 60;
    samples_split = samplerate * samples_split_seconds;
    espeak_SetSynthCallback(SynthCallback);

    /// setup voices
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

    char *p_text = "The quick brown fox jumps over the lazy dogs";
    int size;
	int synth_flags = espeakCHARS_AUTO | espeakPHONEMES | espeakENDPAUSE;
    size = strlen(p_text);
    espeak_Synth(p_text, size+1, 0, POS_CHARACTER, 0, synth_flags, NULL, NULL);

    printf("goodbye world!\n");
	espeak_ng_Synchronize();
	CloseWavFile();
	espeak_ng_Terminate();
	return 0;
}