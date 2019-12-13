#if !defined(SPIDER_MDAPI_H)
#define SPIDER_MDAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SpiderApiStruct.h"
#include <mutex>
#include <set>
#include <vector>

class SpiderCommonApi;
class BaseMarketSession
{
public:
	BaseMarketSession(SpiderCommonApi * sci, AccountInfo & ai)
		: spider_common_api(sci)
		, account_info(ai)
		, is_test(false)
	{}

	virtual ~BaseMarketSession() {}

	virtual bool init() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;

	virtual void subscribe(std::vector<std::string> & list) = 0;
	virtual void unsubscribe(std::vector<std::string> & list) = 0;

	virtual void on_connected() = 0;
	virtual void on_disconnected() = 0;
	virtual void on_log_in() = 0;
	virtual void on_log_out() = 0;
	virtual void on_error(std::shared_ptr<SpiderErrorMsg> & err_msg) = 0;
	virtual void on_receive_data(QuotaData * md) = 0;

public:
	AccountInfo & get_account() { return account_info; }
	std::vector<std::string> get_subscribed_list();
	void updateSubscribedList(const char * ins, bool del = false);
	bool ifSubscribed(const char * ins);
	void setTest(bool test) { is_test = test; }
	bool ifTest() { return is_test; }
protected:
	AccountInfo account_info;
	SpiderCommonApi * spider_common_api;
	std::set<std::string> subscribed_symbol_list;
	std::mutex subscribed_symbol_mutex;
	bool is_test; //test模式下不过滤快照行情
};




#endif