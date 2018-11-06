ifndef SOURCE
$(error "SOURCE not set, can't make xplhub binary")
endif 

COMP_NAME=xplhub
COMP_PATH=$(SOURCE)/complements/$(COMP_NAME)
COMP_SRC_NAME=xplhub
COMP_SRC_PATH=$(COMP_PATH)/src/$(COMP_SRC_NAME)
COMP_EXEC=xplhub

all: $(COMP_SRC_PATH)/$(COMP_EXEC)

$(COMP_SRC_PATH)/$(COMP_EXEC): $(COMP_SRC_PATH)/Makefile
	cd $(COMP_SRC_PATH) ; make

clean:
	cd $(COMP_SRC_PATH) ; make clean
