#if !defined(SPIDER_FUTUREAPI_H)
#define SPIDER_FUTUREAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <thread>
#include <mutex>
#include <deque>
#include <map>
#include <condition_variable>
#include "SpiderApiStruct.h"
#include "ConfigReader.h"
#include "SpiderCommonApi.h"


#define MaxMarketQueueSize 10000

class BaseMarketSession;
class BaseTradeSession;
class SpiderCommonApi : public SpiderApi
{
	//typedef std::map<std::string, std::shared_ptr<BaseMarketSession> > MarketConnections_t; //key account_id
	//MarketConnections_t market_connections;
	std::shared_ptr<BaseMarketSession> market_session;
	//typedef std::map<std::string, std::shared_ptr<BaseTradeSession> > TradeConnections_t; //key account_id
	//TradeConnections_t trade_connections;
	std::shared_ptr<BaseTradeSession> trade_session;

	ConfigReader my_config;

public:

	SpiderCommonApi();
	~SpiderCommonApi();

	void registerSpi(SpiderSpi *pSpi);

	bool init(const char * account_id, int account_type = 0);
	void release();
	int getAllAccounts(AccountInfo ** &out);

	//行情接口
	void marketStart();
	void marketStop();
	void marketRestart();
	void marketSubscribe(char *codes[], int count);
	void marketUnSubscribe(char *codes[], int count);
	
	//交易接口
	void tradeStart();
	void tradeStop();
	void tradeRestart();
	const char * InsertOrder(OrderInsert * order);
	void CancelOrder(OrderCancel * order);
	void QueryTradingAccount(int request_id);
	void QueryPositions(int request_id);
	void QueryOrders(int request_id);
	void QueryTrades(int request_id);
	
public:
	void onQuote(QuotaData * md);


private:
	void quoteWorker();

private:
	bool market_running;
	bool trade_running;
	std::unique_ptr<std::thread> run_thread;
	std::mutex run_mutex;
	std::condition_variable run_cond;
	std::deque <QuotaData *> quote_queue;
public:
	SpiderSpi * notifySpi;
};


#endif