#if !defined(SPIDER_CTPTDAPI_H)
#define SPIDER_CTPTDAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TdApi.h"
#include "SpiderApiStruct.h"
#include "ThostFtdcTraderApi.h"
#include <map>
#include <vector>

class SpiderCtpTdSession;
class SpiderCtpTdSpi : public CThostFtdcTraderSpi
{
	CThostFtdcTraderApi * userApi;

	SpiderCtpTdSession * smd;

	AccountInfo myAccount;

public:
	CThostFtdcTraderApi * getUserApi() { return userApi; }
	bool init(SpiderCtpTdSession * sm);
	void start();
	void stop();
	void login();
	void authenticate(); //看穿式认证

public:
	SpiderCtpTdSpi();
	~SpiderCtpTdSpi();

	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse) {};

	///客户端认证响应
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///报单操作请求响应
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询报单响应
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓响应
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者响应
	virtual void OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///请求查询产品响应
	virtual void OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///请求查询合约响应
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///请求查询投资者结算结果响应
	virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///请求查询投资者持仓明细响应
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///请求查询投资者持仓明细响应
	virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	///成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

	///报单录入错误回报
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

	///报单操作错误回报
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

	///交易通知
	virtual void OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo) {};


};

class SpiderCtpTdSession : public BaseTradeSession
{
public:
	SpiderCtpTdSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderCtpTdSession();

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

	virtual void query_trading_account(int );
	virtual void query_positions(int);
	virtual void query_orders(int);
	virtual void query_trades(int);
	virtual void on_rsp_query_account(TradingAccount * account, int request_id, bool last = false);
	virtual void on_rsp_query_position(InvestorPosition * position, int request_id, bool last = false);
	virtual void on_rsp_query_trade(TradeInfo * trade, int request_id, bool last = false);
	virtual void on_rsp_query_order(OrderInfo * order, int request_id, bool last = false);
public:
	//void init_exoid(int max_exoid)
	//{
	//	if(max_exoid > 0 && max_exoid < INT_MAX)
	//		exoid = max_exoid; 
	//}
	////输入C#策略中的3位orderref，然后拼成真实的orderref传给交易服务器
	//int get_real_order_ref(int order_ref);

private:
	std::shared_ptr<SpiderCtpTdSpi> tradeConnection;
	//int exoid;

	//缓存单条查询结果，然后一次性组包发送
	std::mutex cache_mutex;
	std::map<int, std::vector<TradingAccount* > > cache_account_info; //key request_id
	std::map<int, std::map<std::string, InvestorPosition * > > cache_positions_info; //2nd key contract#direct
	std::map<int, std::vector<OrderInfo * > > cache_orders_info;
	std::map<int, std::vector<TradeInfo * > > cache_trades_info;
};

#endif