all: g13d pbm2lpbm

FLAGS=$(CXXFLAGS)  -std=c++0x
SLIBS=-lboost_system -lboost_program_options -lboost_log -lboost_log_setup -lboost_thread
DLIBS=-lusb-1.0 -lpthread
BUILDDIR=build

g13.o: src/g13.h src/helper.hpp src/g13.cc
	g++ $(FLAGS) -c src/g13.cc

g13_main.o: src/g13.h src/helper.hpp src/g13_main.cc
	g++ $(FLAGS) -c src/g13_main.cc


g13_log.o: src/g13.h src/helper.hpp src/g13_log.cc
	g++ $(FLAGS) -c src/g13_log.cc

g13_fonts.o: src/g13.h src/helper.hpp src/g13_fonts.cc
	g++ $(FLAGS) -c src/g13_fonts.cc

g13_lcd.o: src/g13.h src/helper.hpp src/g13_lcd.cc
	g++ $(FLAGS) -c src/g13_lcd.cc

g13_stick.o: src/g13.h src/helper.hpp src/g13_stick.cc
	g++ $(FLAGS) -c src/g13_stick.cc
	
g13_keys.o: src/g13.h src/helper.hpp src/g13_keys.cc
	g++ $(FLAGS) -c src/g13_keys.cc

helper.o: src/helper.hpp src/helper.cpp
	g++ $(FLAGS) -c src/helper.cpp

g13d: g13_main.o g13.o g13_log.o g13_fonts.o g13_lcd.o g13_stick.o g13_keys.o helper.o
	g++ -o g13d -std=c++0x \
		g13_main.o g13.o g13_log.o g13_fonts.o g13_lcd.o g13_stick.o g13_keys.o helper.o \
		-lpthread \
	 	-Wl,-Bstatic \
		-lboost_system \
	 	-lboost_program_options \
	 	-lboost_log\
		-lboost_log_setup\
	 	-lboost_thread\
	 	-Wl,-Bdynamic \
	 	-lusb-1.0  \
	 	-Wl,--as-needed

pbm2lpbm: src/pbm2lpbm.c
	g++ -o pbm2lpbm src/pbm2lpbm.c

clean: 
	rm -f g13 pbm2lpbm
