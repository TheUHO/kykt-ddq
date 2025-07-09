
libddq.a	:	ddq.o ddq_loop.o ddq_pack.o ddq_mem.o
	$(AR) rcs $@ $^

processorfiles=$(foreach i, $(processors), ../processors/processor_$(i)/processor_$(i).c ../processors/processor_$(i)/processor_$(i).h)
OS := $(shell uname -s)

ddq.o	:	ddq.c ddq_plugin.h ddq_types.h ddq_types_list.h ddq_include_gen.h ddq_section_gen.h $(processorfiles)
	$(CC) -c $< $(CFLAGS) -I'$(pathpref)/ddq'

ddq_loop.o	:	ddq_loop.c ddq_plugin.h ddq_types.h ddq_types_list.h ddq_include_gen.h ddq_section_gen.h $(processorfiles)
	$(CC) -c $< $(CFLAGS) -I'$(pathpref)/ddq'

ddq_pack.o :	ddq_pack.c ddq_include_gen.h
	$(CC) -c $< $(CFLAGS) -I'$(pathpref)/ddq'

ddq_mem.o : ddq_mem.c ddq_mem.h
	$(CC) -c $< $(CFLAGS) -I'$(pathpref)/ddq'



ddq_include_gen.h	:	ddq_include_template.h ddq_types_list.h
	$(CPP) ddq_include_template.h $(CFLAGS) -o ddq_include_gen.h

ddq_section_gen.h	:	ddq_section_template.h ddq_section_piece.h ddq_types_list.h
ifeq ($(OS),Darwin)
	$(CPP) ddq_section_template.h $(CFLAGS) -o - | ./sed.sh > ddq_section_gen.h
else
	$(CPP) ddq_section_template.h $(CFLAGS) -o - | sed '/this_type/a #include "ddq_section_piece.h"' > ddq_section_gen.h
endif

clean	:
	$(RM) *.a *.o

.INTERMEDIATE	:	ddq_include_gen.h ddq_section_gen.h

.PHONY	:	clean

