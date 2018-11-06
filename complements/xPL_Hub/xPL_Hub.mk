ifndef SOURCE
$(error "SOURCE not set, can't make xplhub binary")
endif 

COMP_NAME=xPL_Hub
COMP_PATH=$(SOURCE)/complements/$(COMP_NAME)
COMP_SRC_LIB_PATH=$(COMP_PATH)/src/xPLLib
COMP_SRC_HUB_PATH=$(COMP_PATH)/src/xPLLib/examples
COMP_EXEC=xPL_Hub

all: $(COMP_SRC_LIB_PATH)/xPL.a $(COMP_SRC_HUB_PATH)/$(COMP_EXEC)

$(COMP_SRC_LIB_PATH)/xPL.a:
	cd $(COMP_SRC_LIB_PATH) ; make

$(COMP_SRC_HUB_PATH)/$(COMP_EXEC): $(COMP_SRC_HUB_PATH)/Makefile $(COMP_SRC_LIB_PATH)/xPL.a
	cd $(COMP_SRC_HUB_PATH) ; make xPL_Hub

clean:
	cd $(COMP_SRC_LIB_PATH) ; make clean
	cd $(COMP_SRC_HUB_PATH) ; make clean
