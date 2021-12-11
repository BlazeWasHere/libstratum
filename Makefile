CFLAGS ?= -D_FORTIFY_SOURCE=2 -fstack-clash-protection -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wstrict-overflow=5 -Wswitch-default -Wundef
LDFLAGS ?= -shared -fPIC

LN ?= ln
SED ?= sed
SRC := src
OBJ := object
INC = include
EXAMPLE := example
_HEADER = $(INC)/libstratum/stratum.h

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

LIBVER_MAJOR_SCRIPT:=`$(SED) -n '/define LIBSTRATUM_VERSION_MAJOR/s/.*[[:blank:]]\([0-9][0-9]*\).*/\1/p' < $(_HEADER)`
LIBVER_MINOR_SCRIPT:=`$(SED) -n '/define LIBSTRATUM_VERSION_MINOR/s/.*[[:blank:]]\([0-9][0-9]*\).*/\1/p' < $(_HEADER)`
LIBVER_PATCH_SCRIPT:=`$(SED) -n '/define LIBSTRATUM_VERSION_PATCH/s/.*[[:blank:]]\([0-9][0-9]*\).*/\1/p' < $(_HEADER)`
LIBVER_MAJOR := $(shell echo $(LIBVER_MAJOR_SCRIPT))
LIBVER_MINOR := $(shell echo $(LIBVER_MINOR_SCRIPT))
LIBVER_PATCH := $(shell echo $(LIBVER_PATCH_SCRIPT))
LIBVER := $(LIBVER_MAJOR).$(LIBVER_MINOR).$(LIBVER_PATCH)

ifdef DEBUG
	CFLAGS += -g -DENABLE_DEBUG_LOGGING -DENABLE_CRITICAL_LOGGING
else
	CFLAGS += -O3
endif

SONAME_FLAGS = -Wl,-soname=libstratum.so.$(LIBVER_MAJOR)
SHARED_EXT_MAJOR = so.$(LIBVER_MAJOR)
SHARED_EXT_VER = so.$(LIBVER)

LIBSTRATUM = libstratum.$(SHARED_EXT_VER)

all: $(LIBSTRATUM) $(EXAMPLE)

$(LIBSTRATUM): $(OBJECTS)
	$(CC) -I$(INC) $^ $(CFLAGS) $(LDFLAGS) $(SONAME_FLAGS) -o $@
	$(LN) -sf $@ libstratum.$(SHARED_EXT_MAJOR)
	$(LN) -sf $@ libstratum.so

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(OBJ)
	$(CC) -I$(INC) -c $< -o $@ $(CFLAGS)

$(EXAMPLE): example.c $(OBJECTS)
	$(CC) -I$(INC) $(CFLAGS) -o $@ $^

clean:
	@rm -r $(OBJ)
	@mkdir $(OBJ)
	@rm -f libstratum.$(SHARED_EXT_MAJOR)
	@rm -f libstratum.so
	@rm -f $(LIBSTRATUM)
	@rm -f $(EXAMPLE)
