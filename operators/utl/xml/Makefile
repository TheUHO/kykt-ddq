
subdir = 

all	:	xml.so
	for i in $(subdir) ; do $(MAKE) -e -C $$i ; done

%.so	:	%.c
	$(CC) $< $(CFLAGS) -fPIC -shared -I../../../ddq -o $@

%.a	:	%.o
	$(AR) rcs $@ $<

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -o $@


install	:
	mkdir -p $(install_path)
	-cp *.ddq $(install_path)/ 2>/dev/null || :
	-cp *.so $(install_path)/../ 2>/dev/null || :
	for i in $(subdir) ; do $(MAKE) -e -C $$i install install_path=$(install_path)/$$i ; done

clean	:
	$(RM) *.so *.a *.o

.PHONY	:	all install clean

