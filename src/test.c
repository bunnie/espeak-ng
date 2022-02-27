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

#include "ffi.h"

int samplerate;
bool quiet = false;
unsigned int samples_total = 0;
unsigned int samples_split = 0;
unsigned int samples_split_seconds = 0;
unsigned int wavefile_count = 0;

/*
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
};*/

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
            printf("EVENT: setting synth samplerate to %d\n", events->id.number);
			samplerate = events->id.number;
			samples_split = samples_split_seconds * samplerate;
		} else if (events->type == espeakEVENT_SENTENCE) {
            printf("EVENT: got to end of sentence\n");
			// start a new WAV file when the limit is reached, at this sentence boundary
			if ((samples_split > 0) && (samples_total > samples_split)) {
                printf("EVENT: closing wav file due to end of sentence\n");
				CloseWavFile();
				samples_total = 0;
				wavefile_count++;
			}
		} else {
            printf("EVENT: ignored %d\n", events->type);
        }
		events++;
	}

	if (f_wavfile == NULL) {
        printf("wavfile was null, opening test1.wav\n");
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

/*
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
}*/

/*
Some notes:

- phoneme tables need to be compiled. Original espeak-ng was patched to produce phonemes at 8000 Hz.
  This requires `sox` to be installed (it is available as an ubuntu package)
- voice needs loading. this is done by patching voices.c to skip searching for the entire voices directory,
  and then setting up translator/language using a hard-code english dictionary. This routine is a little
  tricky to patch because it is called on multiple occassions redundantly for different purposes (such as
  listing all the voices, versus just loading one of them) and then patching the loading routine itself is
  a dance between the effectiver lexer and the assumptions of the loaded data.
- a "dictionary" needs to be loaded. This is a a file pulled in within dictionary.c, and we're able to
  patch it with en_dict.h by bodging over the LoadDictionary() API call plus removing the corresponding free() calls
  that would improperly attemp to free up hard-coded/static data.
- next tasks:
   - in addition to creating the WAV file, copy the data into a buffer and confirm its output
   - create a core FFI from these simplifications
   - convert to a build.rs script that can pull in all the files
   - try to link it in as an FFI in Xous.
*/
int main(int argc, char **argv) {
    // test the simplified API
    espeak_ffi_setup(SynthCallback);

    // some setup for the wav output
    samplerate = espeak_ng_GetSampleRate();
    printf("sample rate: %d\n", samplerate);
    samples_split = 0;
    samples_split_seconds = 30 * 60;
    samples_split = samplerate * samples_split_seconds;
    // done with wav output setup

    char *p_text = "I am going to type more than one sentence. Welcome to Precursor!";
    int size;
    size = strlen(p_text) + 1;
    printf("calling espeak_synth\n");
    espeak_ffi_synth(p_text, size);

    printf("synchronize\n");
	espeak_ffi_sync();

    //// these are specific to the main environment
	CloseWavFile();
	espeak_ng_Terminate();
	return 0;
}