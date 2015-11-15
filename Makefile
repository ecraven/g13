
FLAGS=$(CXXFLAGS) -DBOOST_LOG_DYN_LINK -std=c++0x
LIBS=-lusb-1.0 -lboost_log -lboost_log_setup-mt -lboost_thread -lboost_system-mt -lpthread

all: g13d pbm2lpbm

g13d: g13.o g13map.o g13logging.o
	$(CXX) $(FLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

g13.o: g13.cc g13.h g13logging.h g13map.h logo.h
g13map.o: g13map.cc g13map.h
g13logging.o: g13logging.cc g13logging.h

%.o: %.cc
	$(CXX) $(FLAGS) -c $(FLAGS) -o $@ $<

pbm2lpbm: pbm2lpbm.c
	$(CXX) $(FLAGS) -o pbm2lpbm pbm2lpbm.c $(LDFLAGS)

package:
	rm -Rf g13-userspace
	mkdir g13-userspace
	cp *.cc *.h Makefile g13-userspace
	tar cjf g13-userspace.tbz2 g13-userspace
	rm -Rf g13-userspace
clean:
	rm -f g13d *.o pbm2lpbm
