#pragma once
#ifndef TRADE_ENGINE_UTILITY_H
#define TRADE_ENGINE_UTILITY_H
#include <string>
#include <vector>
#include <memory>

#ifndef _WIN32
	#include <unistd.h>	
	#define ACCESS access
	#define ATI64 atoll
	#define sscanf_s sscanf
	#define sprintf_s sprintf
	#define strcpy_s strcpy
#else
	#define ACCESS _access
	#define ATI64 _atoi64
#endif

const double EPSINON = 0.0001;

bool is_double_equal(const double & a,const double & b);

inline std::string makePath(const char * dir, const char * fileName)
{
	//assert(dir!=NULL && strlen(dir)>0);
	//assert(fileName!=NULL && strchr(fileName,'/')==NULL && strchr(fileName,'\\')==NULL);

	std::string fullName(dir);
	if ( *fullName.rbegin()!='/' && *fullName.rbegin()!='\\')
		fullName.append("/");
	fullName.append(fileName);

	return fullName;
}

std::shared_ptr<char> readFile(const char * dir, const char * fileName);
std::shared_ptr<char> readFile(const char * fileName);

inline std::shared_ptr<char> readFile(const std::string& dir, const char * fileName)
{
	return readFile(dir.c_str(), fileName);
};

int writeFile(const char * dir, const char *fileName, const char *data, int length);
int writeFile(const char *fileName, const char *data, int length);
int appendFile(const char *fileName, const char *data, int length);

inline int writeFile(const std::string& dir, const char *fileName, const char *data, int length)
{
	return writeFile(dir.c_str(), fileName, data, length);
}

const char * getTodayString();
const int getTodayInt();
const char * getNowString();

//int getNowTime();
//int getNowSec();

void split_str(const std::string& str, std::vector<std::string>& ret, std::string sep = ",");
void reverse_split_vec(std::string& str, std::vector<std::string>& ret, std::string sep = ",");
std::string getTheTimeFromSec(int now_sec);
std::string getTheTimeFromUnixSec(long long unix_sec);
void replace_all(std::string& str, const std::string& old_value, const std::string& new_value);

static inline int isBigEndian(void) //测试主机大小端
{
	union {

		int32_t i;

		char c[4];

	}u;

	u.i = 1;

	return (u.c[0] == 0);

}
inline unsigned short  spider_swap16(const unsigned short v)
{
	return ((v & 0xff) << 8) | (v >> 8);
}

inline unsigned int  spider_swap32(const unsigned int  v)
{
	return (v >> 24)
		| ((v & 0x00ff0000) >> 8)
		| ((v & 0x0000ff00) << 8)
		| (v << 24);
}

inline unsigned long long  spider_swap64(const unsigned long long  v)
{
	return (v >> 56)
		| ((v & 0x00ff000000000000) >> 40)
		| ((v & 0x0000ff0000000000) >> 24)
		| ((v & 0x000000ff00000000) >> 8)
		| ((v & 0x00000000ff000000) << 8)
		| ((v & 0x0000000000ff0000) << 24)
		| ((v & 0x000000000000ff00) << 40)
		| (v << 56);
}

inline double spider_swapdouble(const double v)
{
	double data64 = 0;

	uint64_t tmp_64 = spider_swap64(*((uint64_t *)& v));

	data64 = *((double*)&tmp_64);

	return data64;
}
#endif

