#if !defined(SPIDER_COMMONAPI_H)
#define SPIDER_COMMONAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#if defined(_WIN32)
#ifdef LIB_SPIDER_API_EXPORT
#define SPIDER_API_EXPORT __declspec(dllexport)
#else
#define SPIDER_API_EXPORT __declspec(dllimport)
#endif
#else
#define SPIDER_API_EXPORT 
#endif

#include "SpiderApiStruct.h"

class SpiderSpi
{
public:

	virtual void onError(SpiderErrorMsg *) {}

	//行情回调
	virtual void marketOnConnect() {}
	virtual void marketOnDisconnect() {}
	virtual void marketOnLogin(const char *, int) {}
	virtual void marketOnLogout() {}
	virtual void marketOnDataArrive(QuotaData *) {}
	
	//交易回调
	virtual void tradeOnConnect() {}
	virtual void tradeOnDisconnect() {}
	virtual void tradeOnLogin(const char *, int) {}
	virtual void tradeOnLogout() {}
	virtual void tradeRtnOrder(OrderInfo *) {}
	virtual void tradeRtnTrade(TradeInfo *) {}
	virtual void tradeOnOrderInsert(OrderInfo *) {}
	virtual void tradeOnOrderCancel(OrderInfo *) {}
	virtual void tradeRspQueryAccount(TradingAccount **, int, int) {}
	virtual void tradeRspQueryPosition(InvestorPosition **, int, int) {}
	virtual void tradeRspQueryTrade(TradeInfo **, int, int) {}
	virtual void tradeRspQueryOrder(OrderInfo **, int, int) {}
};

class SPIDER_API_EXPORT SpiderApi
{
public:
	SpiderApi() {}

	static SpiderApi* createSpiderApi();

	virtual void registerSpi(SpiderSpi *pSpi) = 0;

	virtual bool init(const char * account_id, int account_type = 0) = 0;
	virtual void release() = 0;
	virtual int getAllAccounts(AccountInfo ** &out) = 0;

	//行情接口
	virtual void marketStart() = 0;
	virtual void marketStop() = 0;
	virtual void marketRestart() = 0;
	virtual void marketSubscribe(char *codes[], int count) = 0;
	virtual void marketUnSubscribe(char *codes[], int count) = 0;

	//交易接口
	virtual void tradeStart() = 0;
	virtual void tradeStop() = 0;
	virtual void tradeRestart() = 0;
	virtual const char * InsertOrder(OrderInsert * order) = 0;
	virtual void CancelOrder(OrderCancel * order) = 0;
	virtual void QueryTradingAccount(int request_id) = 0;
	virtual void QueryPositions(int request_id) = 0;
	virtual void QueryOrders(int request_id) = 0;
	virtual void QueryTrades(int request_id) = 0;

protected:
	~SpiderApi() {}
};



#endif