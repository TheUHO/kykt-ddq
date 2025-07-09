
all	:	$(operators)

$(operators)	:
	+$(MAKE) -e -C $@ $(EXT)

install	:
	for i in $(operators) ; do $(MAKE) -e -C $$i install install_path=$(install_path)/usr/oplib/$$i ; done

clean	:
	+$(MAKE) EXT=clean

.PHONY	:	all clean $(operators)

