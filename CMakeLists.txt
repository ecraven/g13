cmake_minimum_required(VERSION 3.5)

project(g13d LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(g13d)
add_executable(pbm2lpbm)

#------------pbm2lpbm Configure-----------------
# pbm2lpbm sources
target_sources( pbm2lpbm PUBLIC
	src/pbm2lpbm.c
)
set_source_files_properties(src/pbm2lpbm.c PROPERTIES LANGUAGE CXX)

#------------g13d Configure-----------------

#Configure dynamic libraries for g13d
target_link_libraries(g13d PUBLIC usb-1.0 )
target_link_libraries(g13d PUBLIC pthread)

#Configure static libraries for g13d
target_link_libraries( g13d PUBLIC boost_system.a)
target_link_libraries( g13d PUBLIC boost_program_options.a)
target_link_libraries( g13d PUBLIC boost_log.a)
target_link_libraries( g13d PUBLIC boost_log_setup.a)
target_link_libraries( g13d PUBLIC boost_thread.a)

#g13d source files
target_sources( g13d PUBLIC
	src/g13.cc
	src/g13_keys.cc
	src/g13_log.cc
	src/g13_stick.cc
	src/g13_fonts.cc
	src/g13_lcd.cc
	src/g13_main.cc
	src/helper.cpp
)

install(TARGETS g13d DESTINATION /usr/local/bin/)
install(TARGETS pbm2lpbm DESTINATION /usr/local/bin/)
install(FILES sys/default.bind DESTINATION /etc/g13d)
install(FILES sys/91-g13d.rules DESTINATION  /etc/udev/rules.d/)
install(FILES sys/g13d.service DESTINATION /etc/systemd/system/)
