PROJECT = espeak_min

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = yes
endif

CSRC = \
	ucd-tools/src/case.c \
	ucd-tools/src/categories.c \
	ucd-tools/src/ctype.c \
	ucd-tools/src/proplist.c \
	ucd-tools/src/scripts.c \
	ucd-tools/src/tostring.c \
	libespeak-ng/compiledata.c \
	libespeak-ng/compiledict.c \
	libespeak-ng/compilembrola.c \
	libespeak-ng/dictionary.c \
	libespeak-ng/encoding.c \
	libespeak-ng/error.c \
	libespeak-ng/espeak_api.c \
	libespeak-ng/ieee80.c \
	libespeak-ng/intonation.c \
	libespeak-ng/mnemonics.c \
	libespeak-ng/numbers.c \
	libespeak-ng/readclause.c \
	libespeak-ng/phoneme.c \
	libespeak-ng/phonemelist.c \
	libespeak-ng/setlengths.c \
	libespeak-ng/soundicon.c \
	libespeak-ng/spect.c \
	libespeak-ng/speech.c \
	libespeak-ng/ssml.c \
	libespeak-ng/synthdata.c \
	libespeak-ng/synthesize.c \
	libespeak-ng/synth_mbrola.c \
	libespeak-ng/translate.c \
	libespeak-ng/tr_languages.c \
	libespeak-ng/voices.c \
	libespeak-ng/wavegen.c

PHONEMES = \
    bin/intonations \
	bin/phondata \
	bin/phonindex \
	bin/phontab \
	bin/en_dict

PHONEME_HEADERS = \
	phondata.h \
	phonindex.h \
	phontab.h \
	intonations.h \
	en_dict.h

LDSCRIPT=

# Various directories
BUILDDIR = build
OUTFILES := $(BUILDDIR)/$(PROJECT).a \
            $(BUILDDIR)/$(PROJECT).dmp \
            $(BUILDDIR)/$(PROJECT).list
OBJDIR    := $(BUILDDIR)/obj
LSTDIR    := $(BUILDDIR)/lst
INCDIR = include/compat include/espeak include/espeak-ng ucd-tools/src/include ../ libespeak-ng

# build the object lists to compile
COBJS    := $(addprefix $(OBJDIR)/, $(notdir $(CSRC:.c=.o)))
# a thing to allow us to add more object files if we wanted to.
OBJS	  := $(COBJS)

# Paths where to search for sources
SRCPATHS  := $(sort $(dir $(CSRC)) )
VPATH     = $(SRCPATHS)
IINCDIR   := $(patsubst %,-I%,$(INCDIR))

all: PRE_MAKE_ALL_RULE_HOOK $(OBJS) $(OUTFILES) POST_MAKE_ALL_RULE_HOOK

PRE_MAKE_ALL_RULE_HOOK:

POST_MAKE_ALL_RULE_HOOK:

$(OBJS): | $(BUILDDIR) $(OBJDIR) $(LSTDIR)
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)
$(OBJDIR):
	@mkdir -p $(OBJDIR)
$(LSTDIR):
	@mkdir -p $(LSTDIR)

# cross compilation overloads
#TRGT = riscv64-unknown-elf
CC   = $(TRGT)gcc
LD   = $(TRGT)gcc
CP   = $(TRGT)objcopy
AR   = $(TRGT)ar
OD   = $(TRGT)objdump
SZ   = $(TRGT)size

# macros
DEFS =

# Define C warning options here
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes
COPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=16 -fstack-protector -flto
# collate all flags
CFLAGS    = $(MCFLAGS) $(COPT) $(CWARN) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.c=.lst)) $(DEFS)

# rule to make .o's # Makefile
$(COBJS) : $(OBJDIR)/%.o : %.c
ifeq ($(USE_VERBOSE_COMPILE),yes)
	@echo
	$(CC) -c $(CFLAGS) $(TOPT) -I. $(IINCDIR) $< -o $@
else
	@echo Compiling $(<F)
	@$(CC) -c $(CFLAGS) $(TOPT) -I. $(IINCDIR) $< -o $@
endif

# rules for debug dumps
ODFLAGS	  = -x --syms
%.dmp: %.a
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(OD) $(ODFLAGS) $< > $@
	$(SZ) $<
else
	@echo Creating $@
	@$(OD) $(ODFLAGS) $< > $@
	@echo
	@$(SZ) $<
endif
%.list: %.a
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(OD) -S $< > $@
else
	@echo Creating $@
	@$(OD) -S $< > $@
	@echo
	@echo Done
endif

# rule to make library
$(BUILDDIR)/$(PROJECT).a: $(OBJS) $(LDSCRIPT)
ifeq ($(USE_VERBOSE_COMPILE),yes)
	@echo
	$(AR) rvs $@ $(OBJS) $(LDFLAGS)
else
	@echo Linking $@
	@$(AR) rvs $@ $(OBJS) $(LDFLAGS)
endif

OUTFILES := $(BUILDDIR)/$(PROJECT).a

$(PHONEME_HEADERS): $(PHONEMES)
	./phongen.py -f bin/phondata -o phondata
	./phongen.py -f bin/phonindex -o phonindex
	./phongen.py -f bin/phontab -o phontab
	./phongen.py -f bin/intonations -o intonations
	./phongen.py -f bin/en_dict -o en_dict

# test program
test: test.c ffi.c $(BUILDDIR)/$(PROJECT).a $(PHONEMES) $(PHONEME_HEADERS)
	$(CC) -c $(CFLAGS) $(TOPT) -I. $(IINCDIR) -o test.o test.c
	$(CC) -c $(CFLAGS) $(TOPT) -I. $(IINCDIR) -o ffi.o ffi.c
	$(LD) test.o ffi.o -L./$(BUILDDIR) -lespeak -lm -o test

ffi.a: ffi.c $(OBJS) $(LDSCRIPT) $(PHONEMES) $(PHONEME_HEADERS)
	$(CC) -c $(CFLAGS) $(TOPT) -I. $(IINCDIR) -o ffi.o ffi.c
	@$(AR) rvs $@ $(OBJS) $(LDFLAGS) ffi.o
