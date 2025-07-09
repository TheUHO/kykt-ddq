
all	:	$(processors)

%	:	processor_%
	+$(MAKE) -e -C $< $(EXT)

clean	:
	+$(MAKE) EXT=clean

.PHONY	:	all clean

