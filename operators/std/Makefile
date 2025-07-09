
subdir = std_types std_ops time ddq

all	:
	for i in $(subdir) ; do $(MAKE) -e -C $$i ; done

%.so	:	%.c
	$(CC) $< $(CFLAGS) -fPIC -shared -I../../ddq -I../../ddq_script -o $@

lib%.a	:	%.o
	$(AR) rcs $@ $<

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -I../../ddq -I../../ddq_script -o $@


install	:
	mkdir -p $(install_path)
	-cp *.ddq *.so $(install_path)/ 2>/dev/null || :
	for i in $(subdir) ; do $(MAKE) -e -C $$i install install_path=$(install_path)/$$i ; done

clean	:
	$(RM) -r *.so *.a *.o *.dSYM
	for i in $(subdir) ; do $(MAKE) clean -e -C $$i ; done

.PHONY	:	all install clean

