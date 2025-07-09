
subdir = int real str dummy obj mem file ptr

all	:	libstd_types.a libstr.a int.so real.so obj.so

real.so	:	CFLAGS += -lm

%.so	:	%.c
	$(CC) $< $(CFLAGS) -fPIC -shared -I../../../ddq -I../../../ddq_script -o $@

lib%.a	:	%.o
	$(AR) rcs $@ $<

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -I../../../ddq -I../../../ddq_script -o $@


install	:
	mkdir -p $(install_path)
	-cp -r *.so $(install_path)/../ 2>/dev/null || :
	for i in $(subdir) ; do cp -r $$i $(install_path)/../ ; mv $(install_path)/../$$i/$$i.ddq $(install_path)/../ ; done
	rmdir $(install_path)

clean	:
	$(RM) -r *.so *.a *.o *.dSYM

.PHONY	:	all install clean

