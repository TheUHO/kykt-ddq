all	:	ops.so types.so

%.so	:	CFLAGS +=-fPIC -shared 

%.so	:	%.c	%.h
	$(CC) $< ${CFLAGS} ${LDFLAGS} -o $@

clean	:
	$(RM) *.so *.o

.PHONY	:	all clean


install	:
	mkdir -p $(install_path)
	-cp *.ddq *.so $(install_path)/ 2>/dev/null || :
	for i in $(subdir) ; do $(MAKE) -e -C $$i install install_path=$(install_path)/$$i ; done

clean	:
	$(RM) *.so *.a *.o
	for i in $(subdir) ; do $(MAKE) clean -e -C $$i ; done