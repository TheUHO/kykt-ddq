
liboplib.a	:	oplib.o oplib_load_ast.o oplib_load_builtin.o $(foreach i, $(oplibs), oplib_load_$(i).o)
	$(AR) rcs $@ $^

%.o	:	%.c
	$(CC) -c $< ${CFLAGS} -I../ddq -I../ddq_script $(foreach i, $(oplibs), -Doplib_load_$(i)) -o $@

oplib_load_builtin.o	:	oplib_load_builtin_gen.h oplib_load_builtin_gen.c

oplib_load_builtin_gen.h	:
	$(RM) $@
	touch $@
	for i in $(operators) ; do grep -r -o 'load_builtin_register[[:space:]]*([^():]*)[[:space:]]*;' ../operators/$$i | grep '\.h:' | cut -d: -f1 | sort | uniq | sed -e 's/^/#include "/g' -e 's/$$/"/g' >> $@ ; done

oplib_load_builtin_gen.c	:
	$(RM) $@
	touch $@
	for i in $(operators) ; do grep -r -o 'load_builtin_register[[:space:]]*([^():]*)[[:space:]]*;' ../operators/$$i | grep '\.h:' | cut -d: -f2 >> $@ ; done

clean	:
	$(RM) *.a *.o oplib_load_builtin_gen.h oplib_load_builtin_gen.c

.INTERMEDIATE	:	oplib_load_builtin_gen.h oplib_load_builtin_gen.c

.PHONY	:	all clean

