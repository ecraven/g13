all: g13d pbm2lpbm

g13.o: g13.h g13.cc
	g++ -std=c++0x -c g13.cc

g13_fonts.o: g13.h g13_fonts.cc
	g++ -std=c++0x -c g13_fonts.cc

g13d: g13.o g13_fonts.o
	g++ -o g13d -std=c++0x g13.o g13_fonts.o -lusb-1.0

pbm2lpbm: pbm2lpbm.c
	g++ -o pbm2lpbm pbm2lpbm.c

package:
	rm -Rf g13-userspace
	mkdir g13-userspace
	cp g13.cc g13.h logo.h Makefile pbm2lpbm.c g13-userspace
	tar cjf g13-userspace.tbz2 g13-userspace
	rm -Rf g13-userspace
clean: 
	rm -f g13 pbm2lpbm