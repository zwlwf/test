#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <fstream>
#include <elfio/elfio_dump.hpp>

using namespace ELFIO;

static std::map<std::string,uint64_t> loadFiles;

static const unsigned int MAXLINE = 2048;

struct mapInfo {
	uint64_t startAddr;
	uint64_t size;
	std::string binPath;
	auto hexCharValue = [](char c) {
		if(c >='a' && c<='f') return c-'a'+10;
		if(c >= 'A' && c<='F') return c-'A'+10;
		if(c>='0' && c<='9') return c-'0';
		return 0;
	};

	mapInfo(const std::string&line) {
		// parse_hex
		int i =0;
		startAddr = 0;
		int n = line.size();
		while(i<n && line[i]!='-') {
			char c = line[i];
			startAddr = (startAddr<<4) + hexCharValue(c);
			i++;
		}
		
		for(int i=0; i<4; i++) {
			while(i<n && line[i]!=' ') i++;
			while(i<n && line[i]==' ') i++;
		}
		// parse size
		size = 0;
		while(i<n && line[i]!=' ') {
			size = size*10 + hexCharValue(line[i]);
			i++;
		}

		while(i<n && line[i]==' ') i++;
		while(i<n && line[i]!=' ' && line[i]!='\n') binPath += line[i++];
	}
};

void parse_maps() {
	const char* fname = "/proc/self/maps";
	FILE* fp = fopen(fname, "r");
	std::ifstream is(fname);
	std::string line;
	bool isFirst = true; // treat the first line as exe
	while( std::getline(is,line) ) {
		mapInfo tmp(line);
		if(tmp.size == 0 ||
		   tmp.binPath.empty() ||
		   tmp.binPath[0] == '[' || // name like [stack]
		   loadFiles.find(tmp.binPath) != loadFiles.end() ) continue;
		if( isFirst ) loadFiles[tmp.binPath] = 0;
		else loadFiles[tmp.binPath] = tmp.startAddr;
		isFirst = false;
	}
	is.close();
}

void readFunctionInfo( std::string binPath ) {
	elfio reader;
	if( !reader.load(binPath.c_str()) ) {
		printf("File %s is not found or not a ELF");
		return;
	}
}

int main() {
	parse_maps();
	for(auto &x : loadFiles) {
		printf("%s : %x\n", x.first.c_str(), x.second);
	}
	return 0;
}
