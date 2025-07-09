
install	:	ddqrun
	mkdir -p $(install_path)/bin
	cp ddqrun $(install_path)/bin/

%	:	%.c $(install_path)/lib/ddq/*.a
	$(CC) $< ${CFLAGS} -I../ddq -I../ddq_script -I../oplib -Wl,--export-dynamic $(LDLIBS) $(LDFLAGS) -o $@ 

clean	:
	$(RM) ddqrun
	$(RM) -r ddqrun.dSYM

.PHONY	:	all clean

