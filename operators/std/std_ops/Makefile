
subdir = ops

all	:	libstd_ops.a

lib%.a	:	%.o
	$(AR) rcs $@ $<

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -I../../../ddq -I../../../ddq_script -o $@


install	:
	mkdir -p $(install_path)
	for i in $(subdir) ; do cp -r $$i/*.ddq $(install_path)/../ ; done
	rmdir $(install_path)

clean	:
	$(RM) -r *.so *.a *.o *.dSYM

.PHONY	:	all install clean

