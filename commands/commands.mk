SHELL = /bin/bash

ifndef TECHNO
$(error - TECHNO is unset)
endif

ifndef BASEDIR
$(error - BASEDIR is unset)
endif

#COMMANDS=$(shell ls -d $(BASEDIR)/commands/*/)
COMMANDS=mea-compilr mea-enocean
.DEFAULT_GOAL = all

printenv:
	@$(foreach V, $(sort $(.VARIABLES)), $(if $(filter-out environment% default automatic, $(origin $V)), $(warning $V=$($V) ($(value $V)))))

all:
	for i in $(COMMANDS) ; \
	do \
           echo $$i ;\
	   $(MAKE) -C $$i -f Makefile.$(TECHNO) BASEDIR="$(BASEDIR)" CC=$(CC); \
	done

clean:
	@for i in $(COMMANDS) ; \
	do \
	   $(MAKE) -C $$i -f Makefile.$(TECHNO) BASEDIR="$(BASEDIR)" clean ; \
	done
