ifndef BASEDIR
$(error - BASEDIR is unset)
endif

ifndef TECHNO
$(error - TECHNO is unset)
endif

SHELL = /bin/bash

ifeq ($(ASPLUGIN), 1)
LINUX_SONAME           = interface_type_010.so
MACOSX_SONAME          = interface_type_010.dylib
LINUX_ASPLUGIN_CFLAGS  = -DASPLUGIN -fPIC
LINUX_ASPLUGIN_LDFLAGS = -shared -Wl,--export-dynamic
MACOSX_ASPLUGIN_CFLAGS = -DASPLUGIN
MACOSX_ASPLUGIN_LDFLAGS= -dynamiclib -undefined suppress -flat_namespace
else
LINUX_SONAME           =
MACOSX_SONAME          =
LINUX_ASPLUGIN_CFLAGS  =
LINUX_ASPLUGIN_LDFLAGS =
MACOSX_ASPLUGIN_CFLAGS =
MACOSX_ASPLUGIN_LDFLAGS=
endif

DEBUGFLAGS  = -D__DEBUG_ON__

ifndef PYTHONVERSION
PYTHONVERSION=2.7
endif
PYTHON=python$(PYTHONVERSION)

SHELL = /bin/bash

DEBUGFLAGS  = -D__DEBUG_ON__
ifeq ($(TECHNO), linux)
   CFLAGS      = -std=gnu99 \
                 -D_DEFAULT_SOURCE \
                 -O2 \
                 -DTECHNO_$(TECHNO) \
                 -I/usr/include/$(PYTHON) \
                 -I"$(BASEDIR)/src" \
                 $(DEBUGFLAGS)
endif
ifeq ($(TECHNO), macosx)
   CFLAGS      = -std=c99 \
                 -O2 \
                 -DTECHNO_$(TECHNO) \
                 -I/System/Library/Frameworks/Python.framework/Versions/$(PYTHON_VERSION)/include/$(PYTHON) \
                 -I"$(BASEDIR)/src" \
                 $(DEBUGFLAGS)
endif

ifeq ($(ASPLUGIN), 1)
SOURCES=interface_type_010.c \
plugin.c
else
SOURCES=interface_type_010.c
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
