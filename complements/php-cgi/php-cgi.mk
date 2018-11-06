ifndef SOURCE
$(error "SOURCE not set, can't make php-cgi binary")
endif 

PHPCGI=$(SOURCE)/complements/php-cgi
PHPSOURCE=$(PHPCGI)/src
DOWNLOAD=$(SOURCE)/complements/downloads

all: $(PHPSOURCE)/php-5.5.16/sapi/cgi/php-cgi

$(PHPSOURCE)/php-5.5.16/sapi/cgi/php-cgi: $(PHPSOURCE)/php-5.5.16/Makefile
	cd $(PHPSOURCE)/php-5.5.16 ; make

$(PHPSOURCE)/php-5.5.16/Makefile: $(PHPSOURCE)/php-5.5.16/extract.ok
	cd $(PHPSOURCE)/php-5.5.16 ; ./configure --prefix=/usr/local/php --disable-all --enable-session --enable-pdo --with-sqlite3 --with-pdo-sqlite --with-pdo-mysql --enable-utf8 --enable-json ; touch $(PHPSOURCE)/php-5.5.16/Makefile

$(PHPSOURCE)/php-5.5.16/extract.ok: $(DOWNLOAD)/php5.tar.bz2
	@mkdir -p $(PHPSOURCE)
	cd $(PHPSOURCE) ; tar xvjf $(DOWNLOAD)/php5.tar.bz2 ; touch $(PHPSOURCE)/php-5.5.16/extract.ok

$(DOWNLOAD)/php5.tar.bz2:
	@mkdir -p $(DOWNLOAD)
	curl -L http://fr2.php.net/get/php-5.5.16.tar.bz2/from/this/mirror -o $(DOWNLOAD)/php5.tar.bz2

clean:
	cd $(PHPSOURCE)/php-5.5.16
	make clean

fullclean:
	rm -r $(PHPSOURCE)
	rm $(DOWNLOAD)/php5.tar.bz2

