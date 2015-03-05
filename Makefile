GROUP='usb'
PREFIX='/usr/local/g13-userspace'

all: g13d pbm2lpbm 91-g13.rules clock.sh

g13d: g13.h g13.cc
	g++ -o g13d -std=c++0x g13.cc -lusb-1.0

pbm2lpbm: pbm2lpbm.c
	g++ -o pbm2lpbm pbm2lpbm.c

91-g13.rules: 91-g13.rules.tmpl
	sed 's/!!GROUP!!/${GROUP}/' 91-g13.rules.tmpl > 91-g13.rules

clock.sh: clock.sh.tmpl
	sed 's|\!\!PREFIX\!\!|${PREFIX}|' clock.sh.tmpl > clock.sh

install: all
	install -m 755 -D g13d ${PREFIX}/g13d
	install -m 755 -D clock.sh ${PREFIX}/g13-clock.sh
	install -m 755 -D pbm2lpbm ${PREFIX}/pbm2lpbm

package:
	rm -Rf g13-userspace
	mkdir g13-userspace
	cp g13.cc g13.h logo.h Makefile pbm2lpbm.c g13-userspace
	tar cjf g13-userspace.tbz2 g13-userspace
	rm -Rf g13-userspace
clean: 
	rm -f g13 pbm2lpbm
