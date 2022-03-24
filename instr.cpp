#include <stdio.h>
#include <string>
#include <fstream>

static int indent=0;
std::ofstream os;

void func_enter(const std::string& fname, uint64_t cs) {
	if(!os.is_open()) {
		os.open("a.txt");
	}
	for(int i=0; i<indent; i++)
		os<<' ';
	os<<fname<<" >>>>> "<<std::endl;
	indent+=2;
}

void func_exit(const std::string& fname, uint64_t cs) {
	indent-=2;
	if(!os.is_open()) {
		os.open("a.txt");
	}
	for(int i=0; i<indent; i++)
		os<<' ';
	os<<fname<<" <<<<< "<<std::endl;
}
