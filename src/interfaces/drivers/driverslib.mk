ifndef BASEDIR
$(error - BASEDIR is unset)
endif

ifndef TECHNO
$(error - TECHNO is unset)
endif

NAME=driverslib

LIBNAME=$(NAME).a

SHELL = /bin/bash

DEBUGFLAGS  = -D__DEBUG_ON__
ifeq ($(TECHNO), linux)
   CFLAGS      = -std=c99 \
                 -std=gnu99 \
                 -D_DEFAULT_SOURCE \
                 -O2 \
                 -DTECHNO_$(TECHNO) \
                 -I/usr/include/mysql \
                 -I/usr/include/python2.7 \
                 -I$(BASEDIR)/src \
                 $(DEBUGFLAGS)
endif
ifeq ($(TECHNO), macosx)
   CFLAGS      = -std=c99 \
                 -O2 \
                 -DTECHNO_$(TECHNO) \
                 -I/usr/local/mysql/include \
                 -I/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 \
                 -I$(BASEDIR)/src \
                 $(DEBUGFLAGS)
endif

LIBDIR=$(BASEDIR)/lib/mealib/$(TECHNO)

SOURCES=arduino_pins.c \
comio2.c \
enocean.c \
netatmo.c \
philipshue.c \
philipshue_color.c \
serial.c \
xbee.c

OBJECTS=$(addprefix $(TECHNO).objects/, $(SOURCES:.c=.o))

$(TECHNO).objects/%.o: %.c
	$(CC) $(INCLUDES) -c $(CFLAGS) $*.c -o $(TECHNO).objects/$(notdir $*.o)

all: $(TECHNO).objects $(LIBDIR)/$(LIBNAME)

$(TECHNO).objects:
	@mkdir -p $(TECHNO).objects

$(LIBDIR)/$(LIBNAME): $(OBJECTS)
	@mkdir -p $(LIBDIR)
	rm -f $(LIBDIR)/$(LIBNAME)
	ar q $(LIBDIR)/$(LIBNAME) $(OBJECTS) $(OBJECTS2B)
	ranlib $(LIBDIR)/$(LIBNAME)

clean:
	rm -f $(TECHNO).objects/*.o $(LIBDIR)/$(LIBNAME) .deps/*.dep
 
-include .deps/*.dep
