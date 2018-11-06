ifndef SOURCE
$(error "SOURCE not set, can't make nodejs binary")
endif 

DOWNLOAD=$(SOURCE)/complements/downloads

COMP_NAME=nodejs
COMP_SRC_NAME=node-v8.9.3
COMP_EXEC=out/Release/node
COMP_TAR_FILE=node-v8.9.3.tar.gz
COMP_URL=http://nodejs.org/dist/v8.9.3/node-v8.9.3.tar.gz

COMP_PATH=$(SOURCE)/complements/$(COMP_NAME)
COMP_SRC_PATH=$(COMP_PATH)/src/$(COMP_SRC_NAME)

all: $(COMP_SRC_PATH)/$(COMP_EXEC)

$(COMP_SRC_PATH)/$(COMP_EXEC): $(COMP_SRC_PATH)/Makefile
	cd $(COMP_SRC_PATH) ; make -j2

$(COMP_SRC_PATH)/Makefile: $(COMP_SRC_PATH)/extract.ok
	cd $(COMP_SRC_PATH) ; ./configure ; touch $(COMP_SRC_PATH)/Makefile

$(COMP_SRC_PATH)/extract.ok: $(DOWNLOAD)/$(COMP_TAR_FILE)
	@mkdir -p $(COMP_PATH)/src
	cd $(COMP_PATH)/src ; tar xvzf $(DOWNLOAD)/$(COMP_TAR_FILE) ; touch $(COMP_SRC_PATH)/extract.ok

$(DOWNLOAD)/$(COMP_TAR_FILE):
	@mkdir -p $(DOWNLOAD)
	curl -o $(DOWNLOAD)/$(COMP_TAR_FILE) $(COMP_URL)

download: $(DOWNLOAD)/$(COMP_TAR_FILE)

clean:
	cd $(COMP_SRC_PATH) ; make clean

fullclean: clean
	rm $(DOWNLOAD)/$(COMP_TAR_FILE)
	rm -r COMP_SRC_PATH=$(COMP_PATH)/src
