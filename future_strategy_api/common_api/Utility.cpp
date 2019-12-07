#include <ctime>
#include <fstream>
#include <chrono>
#include "Utility.h"


std::shared_ptr<char> readFile(const char * dir, const char * fileName)
{
	//assert(dir!=NULL && strlen(dir)>0);
	//assert(fileName!=NULL && strchr(fileName,'/')==NULL);

	std::string fullName(dir);
	if ( *fullName.rbegin()!='/')
		fullName.append("/");

	fullName.append(fileName);
	return readFile(fullName.c_str());
}


std::shared_ptr<char> readFile(const char * fileName)
{
	std::ifstream ifs(fileName, std::ios::binary);
	if (ifs.fail())
	{
		return std::shared_ptr<char>();
	}

	ifs.seekg(0, std::ios::end);
	size_t size = (size_t)ifs.tellg();

	std::shared_ptr<char> buffer(new char[size+1]);
	ifs.seekg(0);
	ifs.read(buffer.get(), size); 
	buffer.get()[size]=0;

	return buffer;
};

int writeFile(const char * dir, const char *fileName, const char *data, int length)
{
	//assert(dir!=NULL && strlen(dir)>0);
	//assert(fileName!=NULL && strchr(fileName,'/')==NULL);

	std::string fullName(dir);
	if ( *fullName.rbegin()!='/')
		fullName.append("/");

	fullName.append(fileName);
	return writeFile(fullName.c_str(), data, length);
}

int writeFile(const char *fileName, const char *data, int length)
{
	std::ofstream ofs(fileName, std::ios::binary | std::ios::trunc | std::ios::out);
	if (ofs.fail())
		return -1;

	ofs.write(data, length);
	return 0;
}

int appendFile(const char *fileName, const char *data, int length)
{
	std::ofstream ofs(fileName, std::ios::binary | std::ios::app | std::ios::out);
	if (ofs.fail())
		return -1;

	ofs.write(data, length);
	return 0;
}

const char * getTodayString()
{
	static char todayString[9];
	time_t rawtime;
	struct tm timeinfo;
	time ( &rawtime );
	localtime_s(&timeinfo, &rawtime );
	strftime(todayString, sizeof(todayString),"%Y%m%d",&timeinfo);
	//sprintf(todayString, "%d%02d%02d",(int)timeinfo.tm_year + 1900, (int)timeinfo.tm_mon + 1, (int)timeinfo.tm_mday);
	return todayString;
}

const int getTodayInt(){
	const char * todayString;
	static int todayInt;
	todayString=getTodayString();
	sscanf(todayString, "%d",&todayInt);
	return todayInt;
}

const char * getNowString()
{
	static char nowString[7];
	time_t rawtime;
	struct tm timeinfo;
	time ( &rawtime );
	localtime_s(&timeinfo, &rawtime );
	strftime(nowString, sizeof(nowString),"%H%M%S",&timeinfo);
	return nowString;
}

bool is_double_equal( const double & a,const double & b )
{
	double x=a-b;
	if ((x >= - EPSINON)&& (x <= EPSINON)){
		return true;
	}else{
		return false;
	}
}

void split_str(const std::string& str, std::vector<std::string>& ret, std::string sep)
{
	std::string::size_type lastPos = str.find_first_not_of(sep, 0);
	std::string::size_type pos = str.find_first_of(sep, lastPos);
	while (std::string::npos != pos || std::string::npos != lastPos)
	{
		ret.push_back(str.substr(lastPos, pos - lastPos));
		lastPos = str.find_first_not_of(sep, pos);
		pos = str.find_first_of(sep, lastPos);
	}
}

void reverse_split_vec(std::string& str, std::vector<std::string>& ret, std::string sep)
{
	for (unsigned int i = 0; i < ret.size(); ++i )
	{
		str.append(ret[i]);
		str.append(sep);
	}
	if (!str.empty() && !sep.empty()) //把最后一个分隔符sep删掉
	{
		str.erase(str.end() - sep.length());
	}
}

std::string getTheTimeFromSec(int now_sec)
{
	//把当天的秒数，转换成 10:50:05 这样的格式

	int hh = now_sec / 3600;
	int mm = now_sec / 60 - hh * 60;
	int ss = now_sec % 60;

	char strtm[12] = { 0 };
	sprintf(strtm, "%02d:%02d:%02d",hh,mm,ss);
	return std::string(strtm);
}

std::string getTheTimeFromUnixSec(long long unix_sec)
{
	//把1970年至今的秒数，转换成 10:50:05 这样的格式
	time_t _0now;
	struct tm _0tm_now;
	time(&_0now);
	localtime_s(&_0tm_now,&_0now);
	_0tm_now.tm_hour = 0;
	_0tm_now.tm_min = 0;
	_0tm_now.tm_sec = 0;
	long long  now_sec  = unix_sec - mktime(&_0tm_now) ;

	int hh = now_sec / 3600;
	int mm = now_sec / 60 - hh * 60;
	int ss = now_sec % 60;

	char strtm[12] = { 0 };
	sprintf(strtm, "%02d:%02d:%02d", hh, mm, ss);
	return std::string(strtm);
}

void replace_all(std::string& str, const std::string& old_value, const std::string& new_value)
{
	while (true) {
		std::string::size_type   pos(0);
		if ((pos = str.find(old_value)) != std::string::npos)
			str.replace(pos, old_value.length(), new_value);
		else
			break;
	}
}

//int getNowTime()
//{
//	std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
//	auto tt1 = std::chrono::system_clock::now().time_since_epoch;
//}
//
//int getNowSec()
//{
//	std::chrono::system_clock::now();
//}