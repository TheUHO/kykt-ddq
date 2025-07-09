
subdir = 

all	:	
	for i in $(subdir) ; do $(MAKE) -e -C $$i ; done

%.so	:	%.c
	$(CC) $< $(CFLAGS) -fPIC -shared -o $@

%.a	:	%.o
	$(AR) rcs $@ $<

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -o $@


install	:
	mkdir -p $(install_path)
	-cp *.ddq *.so $(install_path)/ 2>/dev/null || :
	for i in $(subdir) ; do $(MAKE) -e -C $$i install install_path=$(install_path)/$$i ; done

clean	:
	$(RM) -r *.so *.a *.o *.dSYM

.PHONY	:	all install clean

