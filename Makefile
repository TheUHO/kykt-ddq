

export install_path=$(shell pwd)/compiled_$(target)

last_target=$(subst Makefile.target.,,$(wildcard Makefile.target.*))
export target=$(last_target)

export pathpref=$(shell pwd)

include targets/Makefile.inc.$(target)

export CFLAGS+=-Doplib_path='"$(install_path)/usr/oplib"'
CFLAGS+=-g


compile.ddq	:
	+$(MAKE) -e -C ddq

compile.processors	:
	+$(MAKE) -e -C processors

compile.ddq_script	:
	+$(MAKE) -e -C ddq_script

compile.operators	:
	+$(MAKE) -e -C operators

compile.oplib	:	compile.operators
	+$(MAKE) -e -C oplib

install.operators	:	compile.operators
	+$(MAKE) -e -C operators install

install.lib	:	compile.ddq compile.ddq_script compile.oplib compile.operators compile.processors
	mkdir -p $(install_path)/lib/ddq
	cp ddq/libddq.a ddq_script/libddq_script.a oplib/liboplib.a ` for i in $(operators) ; do find operators/$$i -name "*.a" ; done ` ` for i in $(processors) ; do find processors/processor_$$i -name "*.a" ; done ` $(install_path)/lib/ddq/

install.mains	:	install.lib
	+$(MAKE) -e -C mains install

log.cfg		:
	mkdir -p $(install_path)/etc
	@echo $(foreach i, $(processors), -Denable_processor_$(i)) > $(install_path)/etc/ddq.cfg

Makefile.target.$(target)	:
	-[ -n "$(last_target)" ] &&  [ "$(last_target)" != "$(target)" ] && make target=$(last_target) clean
	-$(RM) Makefile.target.*
	touch $@

compile	:	Makefile.target.$(target) log.cfg compile.ddq compile.processors compile.ddq_script compile.operators compile.oplib

install	:	compile install.operators install.lib install.mains

clear	:
	+$(MAKE) -e -C ddq clean
	+$(MAKE) -e -C processors clean
	+$(MAKE) -e -C ddq_script clean
	+$(MAKE) -e -C operators clean
	+$(MAKE) -e -C oplib clean
	+$(MAKE) -e -C mains clean

clean	: clear
	-$(RM) -r compiled_*

.PHONY	:	all clean compile install compile.ddq compile.processors compile.ddq_script compile.operators compile.oplib install.operators install.lib install.mains log.cfg

