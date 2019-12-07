#include <iostream>
#include <vector>
#include <string>
#include "../common_api/SpiderCommonApi.h"
//#include "../common_api/SpiderFutureApi.h"

#pragma comment(lib,"../Release/future_common_api.lib")

SpiderApi* a;


class myspi : public SpiderSpi
{
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
	virtual void tradeOnLogin(const char *, int) 
	{ 
		std::cout << "connect" << std::endl; 

		OrderInsert * _ord = new OrderInsert();
		strncpy(_ord->Code, "IF1912", sizeof(_ord->Code));
		_ord->ExchangeID = 74;
		_ord->Direction = 1;
		_ord->HedgeFlag = 3;
		_ord->Offset = 1;

		strncpy(_ord->OrderRef, "009", sizeof(_ord->OrderRef));
		_ord->LimitPrice = 3800;
		_ord->VolumeTotalOriginal = 1;
		const char * tmp = a->InsertOrder(_ord);
		printf("AAA: %p", tmp);

	}
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

int main()
{
	myspi b;

	a = SpiderApi::createSpiderApi();
	a->registerSpi(&b);
	if (a->init("108741",1))
	{
		std::cout << "init ok" << std::endl;
		a->tradeStart();
		

	}
	else {
		std::cout << "init fail" << std::endl;
	}
	while (getchar() != 'q')
	{ }

	return 0;
}