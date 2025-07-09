
mods = fib fib2

all	: $(mods)

$(mods)	:
	+$(MAKE) -e -C $@ $(EXT)
	-cp $@/*.so ./ 2>/dev/null || :

clean	:
	+$(MAKE) EXT=clean
	-$(RM) *.so 

.PHONY	:	all clean $(mods)

