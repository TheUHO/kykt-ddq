
libddq_script.a	:	ast.lex.o ast.tab.o ast.o pool.o passes.o
	$(AR) rcs $@ $^

%.o	:	%.c
	$(CC) -c $< ${CFLAGS} -I../ddq -I../oplib $(foreach i, $(oplibs), -Doplib_load_$(i)) -o $@



ast.lex.c	:	ast.l ast.tab.c
	flex -o $@ --header-file=$(patsubst %.c,%.h,$@) --debug $<

ast.tab.c	:	ast.y
	bison -o $@ --defines=$(patsubst %.c,%.h,$@) --debug $<


ast.o	:	ast.tab.o ast.lex.o


# make's builtin rules would get things a mess!
%.c: %.y


%.c: %.l



clean	:
	$(RM) *.a *.o ast.tab.* ast.lex.*

.PHONY	:	all clean

