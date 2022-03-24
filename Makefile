a.out : libwrap.so
	g++ a.cpp -finstrument-functions -L. -lwrap -std=c++11 -linstr

libwrap.so : wrap.cpp libinstr.so
	g++ wrap.cpp -std=c++11 -I./ELFIO -shared -o libwrap.so -L. -linstr -fPIC

libinstr.so : instr.cpp
	g++ -std=c++11 instr.cpp -shared -o libinstr.so -fPIC
