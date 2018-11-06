ifndef BASEDIR
$(error - BASEDIR is unset)
endif

ifndef TECHNO
$(error - TECHNO is unset)
endif

SHELL = /bin/bash

ifeq ($(ASPLUGIN), 1)
LINUX_SONAME           = interface_type_003.so
MACOSX_SONAME          = interface_type_003.dylib
LINUX_ASPLUGIN_CFLAGS  = -DASPLUGIN
LINUX_ASPLUGIN_LDFLAGS = -shared -Wl,--export-dynamic
MACOSX_ASPLUGIN_CFLAGS = -DASPLUGIN
MACOSX_ASPLUGIN_LDFLAGS= -dynamiclib -undefined suppress -flat_namespace
else
LINUX_SONAME           =
MACOSX_SONAME          =
LINUX_ASPLUGIN_CFLAGS  =
LINUX_ASPLUGIN_LDFLAGS = -shared
MACOSX_ASPLUGIN_CFLAGS =
MACOSX_ASPLUGIN_LDFLAGS=
endif

DEBUGFLAGS  = -D__DEBUG_ON__
ifeq ($(TECHNO), linux)
   SONAME      = $(LINUX_SONAME)
   CFLAGS      = -std=gnu99 \
                 -D_DEFAULT_SOURCE \
                 -O2 \
                 -DTECHNO_$(TECHNO) \
                 -I/usr/include/mysql \
                 -I/usr/include/python2.7 \
                 -I$(BASEDIR)/src \
                 $(DEBUGFLAGS) \
                 $(LINUX_ASPLUGIN_CFLAGS)
   LDFLAGS     = $(LINUX_ASPLUGIN_LDFLAGS)
endif
ifeq ($(TECHNO), macosx)
   SONAME      = $(MACOSX_SONAME)
   CFLAGS      = -std=c99 \
                 -O2 \
                 -DTECHNO_$(TECHNO) \
                 -I/usr/local/mysql/include \
                 -I/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 \
                 -I$(BASEDIR)/src \
                 $(DEBUGFLAGS) \
                 $(MACOSX_ASPLUGIN_CFLAGS)
   LDFLAGS     = $(MACOSX_ASPLUGIN_LDFLAGS)
endif

ifeq ($(ASPLUGIN), 1)
SOURCES=enocean.c \
interface_type_003.c \
plugin.c
else
SOURCES=enocean.c \
interface_type_003.c
endif

OBJECTS=$(addprefix $(TECHNO).objects/, $(SOURCES:.c=.o))

all: .deps $(TECHNO).objects $(OBJECTS) $(SONAME)

$(SONAME): $(OBJECTS) 
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $(SONAME);

$(TECHNO).objects/%.o: $(SOURCES)
	@$(CC) $(INCLUDES) -c $(CFLAGS) -MM -MT $(TECHNO).objects/$*.o $*.c > .deps/$*.dep
	$(CC) $(INCLUDES) -c $(CFLAGS) $*.c -o $(TECHNO).objects/$*.o

.deps:
	@mkdir -p .deps

$(TECHNO).objects:
	@mkdir -p $(TECHNO).objects

clean:
	rm -f $(TECHNO).objects/*.o .deps/*.dep $(SONAME)
 
-include .deps/*.dep
