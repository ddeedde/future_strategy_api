#if !defined(SPIDER_XELETDAPI_H)
#define SPIDER_XELETDAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TdApi.h"
#include "SpiderApiStruct.h"
#include "CXeleTraderApi.hpp"
#include <map>

typedef HMODULE X_DLL_HANDLE;

class SpiderXeleTdSession;
class SpiderXeleTdSpi : public CXeleTraderSpi
{
	X_DLL_HANDLE x_handle;
	CXeleTraderApi * userApi;

	SpiderXeleTdSession * smd;

	AccountInfo myAccount;

	bool isReady;

	char trade_url[80];
	char query_url[80];

public:
	CXeleTraderApi * getUserApi() { return userApi; }
	bool init(SpiderXeleTdSession * sm);
	void start();
	void stop();
	void login();
	void authenticate(); //����ʽ��֤
	bool ready() { return isReady; }
	void setReady(bool rd) { isReady = rd; };

public:
	SpiderXeleTdSpi();
	~SpiderXeleTdSpi();

	// ���ӳɹ��ص�
	virtual void OnFrontConnected() final;

	// ���ӶϿ��ص�
	virtual void OnFrontDisconnected(int nReason) final;

	// ��½�ص�
	virtual void OnRspUserLogin(CXeleFtdcRspUserLoginField *pRspUserLogin, CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

	virtual void OnRspUserLogout(CXeleFtdcRspUserLogoutField *pRspUserLogout, CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

	// �����ص�
	virtual void OnRspOrderInsert(CXeleFtdcInputOrderField *pInputOrder, CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

	// �����ص�
	virtual void OnRspOrderAction(CXeleFtdcOrderActionField *pOrderAction, CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

	///
	virtual void OnRtnTrade(CXeleFtdcTradeField *pTrade) final;

	// �����ر��ص�
	virtual void OnRtnOrder(CXeleFtdcOrderField *pOrder) final;

	///
	virtual void OnErrRtnOrderInsert(CXeleFtdcInputOrderField *pInputOrder, CXeleFtdcRspInfoField *pRspInfo) final;

	///
	virtual void OnErrRtnOrderAction(CXeleFtdcOrderActionField *pOrderAction, CXeleFtdcRspInfoField *pRspInfo) final;

	// ������Ӧ�ص�
	virtual void OnRspError(CXeleFtdcRspInfoField *pRspInfo, int nRequestID,bool bIsLast) final;
	
	// �ֲֲ�ѯ�ص�
	virtual void OnRspQryClientPosition(CXeleFtdcRspClientPositionField *pRspClientPosition, CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

	// �ͻ��ʽ��ѯ�ص�
	virtual void OnRspQryClientAccount(CXeleFtdcRspClientAccountField *pClientAccount, CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

	///
	virtual void OnRspQryOrder(CXeleFtdcOrderField* pOrderField, CXeleFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) final;

	///
	virtual void OnRspQryTrade(CXeleFtdcTradeField* pTradeField, CXeleFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) final;

};

class SpiderXeleTdSession : public BaseTradeSession
{
public:
	SpiderXeleTdSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderXeleTdSession();

	virtual bool init();
	virtual void start();
	virtual void stop();

	virtual const char * insert_order(OrderInsert *);
	virtual void cancel_order(OrderCancel *);

	virtual void on_connected();
	virtual void on_disconnected();
	virtual void on_log_in();
	virtual void on_log_out();
	virtual void on_error(std::shared_ptr<SpiderErrorMsg> & err_msg);

	virtual void on_order_change(OrderInfo *);
	virtual void on_trade_change(TradeInfo *);
	virtual void on_order_cancel(OrderInfo *);
	virtual void on_order_insert(OrderInfo *);

	virtual void query_trading_account(int);
	virtual void query_positions(int);
	virtual void query_orders(int);
	virtual void query_trades(int);
	virtual void on_rsp_query_account(TradingAccount * account, int request_id, bool last = false);
	virtual void on_rsp_query_position(InvestorPosition * position, int request_id, bool last = false);
	virtual void on_rsp_query_trade(TradeInfo * trade, int request_id, bool last = false);
	virtual void on_rsp_query_order(OrderInfo * order, int request_id, bool last = false);
public:
	void init_exoid(const char * max_exoid);
	//����C#�����е�3λorderref��Ȼ��ƴ����ʵ��orderref�������׷�����
	int get_real_order_ref(int order_ref);


private:
	std::shared_ptr<SpiderXeleTdSpi> tradeConnection;

	int exoid;
	std::mutex exoid_mutex;


	//���浥����ѯ�����Ȼ��һ�����������
	std::mutex cache_mutex;
	std::map<int, std::vector<TradingAccount* > > cache_account_info; //key request_id
	std::map<int, std::map<std::string, InvestorPosition * > > cache_positions_info; //2nd key contract#direct
	std::map<int, std::vector<OrderInfo * > > cache_orders_info;
	std::map<int, std::vector<TradeInfo * > > cache_trades_info;
};


#endif