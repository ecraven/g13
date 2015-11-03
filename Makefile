all: g13d pbm2lpbm

g13d: g13.h g13.cc g13map.o
	g++ -o g13d -std=c++0x g13.cc g13map.o -lusb-1.0

g13map.o: g13map.cc g13map.h
	g++ -c -o g13map.o -std=c++0x g13map.cc

pbm2lpbm: pbm2lpbm.c
	g++ -o pbm2lpbm pbm2lpbm.c

package:
	rm -Rf g13-userspace
	mkdir g13-userspace
	cp g13.cc g13.h logo.h Makefile pbm2lpbm.c g13-userspace
	tar cjf g13-userspace.tbz2 g13-userspace
	rm -Rf g13-userspace
clean: 
	rm -f g13d g13map.o pbm2lpbm
