#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <fstream>
#include <elfio/elfio_dump.hpp>

using namespace ELFIO;

static std::map<std::string,uint64_t> loadFiles;
static std::map<uint64_t, std::string> site2Functions;

int hexCharValue(const char c) {
	if(c >='a' && c<='f') return c-'a'+10;
	if(c >= 'A' && c<='F') return c-'A'+10;
	if(c>='0' && c<='9') return c-'0';
	return 0;
}

struct mapInfo {
	uint64_t startAddr;
	uint64_t size;
	std::string binPath;

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
		
		for(int j=0; j<4; j++) {
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

void readFunctionInfo( std::string binPath, uint64_t offset ) {
	elfio reader;
	if( !reader.load(binPath.c_str()) ) {
		printf("File %s is not found or not a ELF");
		return;
	}
	Elf_Half n = reader.sections.size();
	for(int i=0; i<n; i++) {
		section* sec = reader.sections[i];
		if( sec->get_type() == SHT_SYMTAB ) {
			symbol_section_accessor symbols( reader, sec );
			Elf_Xword sym_n = symbols.get_symbols_num();
			for(int j=0; j<sym_n; j++) {
				std::string name;
				Elf64_Addr value=0;
				Elf_Xword size=0;
				unsigned char bind = 0;
				unsigned char type = 0;
				Elf_Half shndx = 0;
				unsigned char other =0 ;
				symbols.get_symbol(j, name, value, size, bind, type, shndx, other);
				if( type == STT_FUNC ) {
					site2Functions[value + offset] = name;
				}
			}
			return;
		}
	}
}

extern void func_enter(const std::string& fname, uint64_t callsite);
extern void func_exit(const std::string& fname, uint64_t callsite);

extern "C" void __cyg_profile_func_enter(void* this_fn, void* call_site) {
	if( site2Functions.empty() ) {
		parse_maps();
		for(auto &x : loadFiles) {
			readFunctionInfo( x.first, x.second);
		}
	}
	uint64_t addr = (uint64_t) this_fn;
	if( site2Functions.find(addr) != site2Functions.end() ) {
		func_enter( site2Functions[addr], (uint64_t) call_site);
	} else {
		fprintf(stderr, "Function [unknown] is called from %x\n", call_site);
	}
}

extern "C" void __cyg_profile_func_exit(void* this_fn, void* call_site) {
	if( site2Functions.empty() ) {
		parse_maps();
		for(auto &x : loadFiles) {
			readFunctionInfo( x.first, x.second);
		}
	}
	uint64_t addr = (uint64_t) this_fn;
	if( site2Functions.find(addr) != site2Functions.end() ) {
		func_exit( site2Functions[addr], (uint64_t) call_site);
	} else {
		fprintf(stderr, "Function [unknown] is called from %x\n", call_site);
	}
}
