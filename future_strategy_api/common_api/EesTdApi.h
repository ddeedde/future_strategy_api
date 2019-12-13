#if !defined(SPIDER_EESTDAPI_H)
#define SPIDER_EESTDAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TdApi.h"
#include "SpiderApiStruct.h"
#include "EesTraderApi.h"
#include <map>
#include <deque>


struct ees_order
{
	unsigned int client_token;
	long long market_token;
	int ex;
	int direct;
	int hedge;
	int offset;
	int order_qty;
	double order_px;
	int deal_qty;
	std::string symbol;

	ees_order()
		: client_token(0), market_token(0), ex(0), direct(0), hedge(0), offset(0), order_qty(0), order_px(0), deal_qty(0), symbol("")
	{}
	ees_order(OrderInsert * _ord)
	{
		ex = _ord->ExchangeID;
		direct = _ord->Direction;
		hedge = _ord->HedgeFlag;
		offset = _ord->Offset;
		order_qty = _ord->VolumeTotalOriginal;
		order_px = _ord->LimitPrice;
		deal_qty = 0;
		symbol = _ord->Code;
	}
};

class SpiderEesTdSession;
class SpiderEesTdSpi : public EESTraderEvent
{
	EESTraderApi * userApi;

	SpiderEesTdSession * smd;

	AccountInfo myAccount;

	EES_TradeSvrInfo myServer;

	bool isReady;

	std::shared_ptr<std::thread> independent_thread; //用于定时器，或者异步做些别的事情

	std::deque<async_task> task_queue;
	std::mutex task_mutex;
public:
	EESTraderApi * getUserApi() { return userApi; }
	bool init(SpiderEesTdSession * sm);
	void start();
	void stop();
	void login();
	//void authenticate(); //看穿式认证
	bool ready() { return isReady; }

	void add_task(async_task _t);
	void process_task();

public:
	SpiderEesTdSpi();
	~SpiderEesTdSpi();

	///	\brief	服务器连接事件
	///	\param  errNo                   连接成功能与否的消息
	///	\param  pErrStr                 错误信息
	///	\return void  

	virtual void OnConnection(ERR_NO errNo, const char* pErrStr);

	/// 连接断开消息的回调

	/// \brief	服务器主动断开，会收到这个消息
	/// \param  ERR_NO errNo         连接成功能与否的消息
	/// \param  const char* pErrStr  错误信息
	/// \return void  

	virtual void OnDisConnection(ERR_NO errNo, const char* pErrStr);

	/// 登录消息的回调

	/// \param  pLogon                  登录成功或是失败的结构
	/// \return void 

	virtual void OnUserLogon(EES_LogonResponse* pLogon);

	/// 修改密码响应回调

	/// \param  nResult                  服务器响应的成功与否返回码
	/// \return void 

	virtual void OnRspChangePassword(EES_ChangePasswordResult nResult) {}

	/// 查询用户下面帐户的返回事件

	/// \param  pAccountInfo	        帐户的信息
	/// \param  bFinish	                如果没有传输完成，这个值是 false ，如果完成了，那个这个值为 true 
	/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pAccountInfo值无效。
	/// \return void 

	virtual void OnQueryUserAccount(EES_AccountInfo * pAccoutnInfo, bool bFinish);

	/// 查询帐户下面期货仓位信息的返回事件	
	/// \param  pAccount	                帐户ID 	
	/// \param  pAccoutnPosition	        帐户的仓位信息					   
	/// \param  nReqId		                发送请求消息时候的ID号。
	/// \param  bFinish	                    如果没有传输完成，这个值是false，如果完成了，那个这个值为 true 
	/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pAccountInfo值无效。
	/// \return void 	
	virtual void OnQueryAccountPosition(const char* pAccount, EES_AccountPosition* pAccoutnPosition, int nReqId, bool bFinish);

	/// 查询帐户下面期权仓位信息的返回事件, 注意这个回调, 和上一个OnQueryAccountPosition, 会在一次QueryAccountPosition请求后, 分别返回, 先返回期货, 再返回期权, 即使没有期权仓位, 也会返回一条bFinish=true的记录
	/// \param  pAccount	                帐户ID 	
	/// \param  pAccoutnPosition	        帐户的仓位信息					   
	/// \param  nReqId		                发送请求消息时候的ID号。
	/// \param  bFinish	                    如果没有传输完成，这个值是false，如果完成了，那个这个值为 true 
	/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pAccountInfo值无效。
	/// \return void 	
	virtual void OnQueryAccountOptionPosition(const char* pAccount, EES_AccountOptionPosition* pAccoutnOptionPosition, int nReqId, bool bFinish) {}


	/// 查询帐户下面资金信息的返回事件

	/// \param  pAccount	                帐户ID 	
	/// \param  pAccoutnPosition	        帐户的仓位信息					   
	/// \param  nReqId		                发送请求消息时候的ID号
	/// \return void 

	virtual void OnQueryAccountBP(const char* pAccount, EES_AccountBP* pAccoutnPosition, int nReqId);

	/// 查询合约列表的返回事件

	/// \param  pSymbol	                    合约信息   
	/// \param  bFinish	                    如果没有传输完成，这个值是 false，如果完成了，那个这个值为 true   
	/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pSymbol 值无效。
	/// \return void 

	virtual void OnQuerySymbol(EES_SymbolField* pSymbol, bool bFinish) {}

	/// 查询帐户交易保证金的返回事件

	/// \param  pAccount                    帐户ID 
	/// \param  pSymbolMargin               帐户的保证金信息 
	/// \param  bFinish	                    如果没有传输完成，这个值是 false，如果完成，那个这个值为 true 
	/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pSymbolMargin 值无效。
	/// \return void 

	virtual void OnQueryAccountTradeMargin(const char* pAccount, EES_AccountMargin* pSymbolMargin, bool bFinish) {}

	/// 查询帐户交易费用的返回事件

	/// \param  pAccount                    帐户ID 
	/// \param  pSymbolFee	                帐户的费率信息	 
	/// \param  bFinish	                    如果没有传输完成，这个值是 false，如果完成了，那个这个值为 true    
	/// \remark 如果碰到 bFinish == true ，那么是传输结束，并且 pSymbolFee 值无效。
	/// \return void 

	virtual void OnQueryAccountTradeFee(const char* pAccount, EES_AccountFee* pSymbolFee, bool bFinish) {}

	/// 下单被柜台系统接受的事件

	/// \brief 表示这个订单已经被柜台系统正式的接受
	/// \param  pAccept	                    订单被接受以后的消息体
	/// \return void 

	virtual void OnOrderAccept(EES_OrderAcceptField* pAccept);


	/// 下单被市场接受的事件

	/// \brief 表示这个订单已经被交易所正式的接受
	/// \param  pAccept	                    订单被接受以后的消息体，里面包含了市场订单ID
	/// \return void 
	virtual void OnOrderMarketAccept(EES_OrderMarketAcceptField* pAccept);


	///	下单被柜台系统拒绝的事件

	/// \brief	订单被柜台系统拒绝，可以查看语法检查或是风控检查。 
	/// \param  pReject	                    订单被接受以后的消息体
	/// \return void 

	virtual void OnOrderReject(EES_OrderRejectField* pReject);


	///	下单被市场拒绝的事件

	/// \brief	订单被市场拒绝，可以查看语法检查或是风控检查。 
	/// \param  pReject	                    订单被接受以后的消息体，里面包含了市场订单ID
	/// \return void 

	virtual void OnOrderMarketReject(EES_OrderMarketRejectField* pReject);


	///	订单成交的消息事件

	/// \brief	成交里面包括了订单市场ID，建议用这个ID查询对应的订单
	/// \param  pExec	                   订单被接受以后的消息体，里面包含了市场订单ID
	/// \return void 

	virtual void OnOrderExecution(EES_OrderExecutionField* pExec);

	///	订单成功撤销事件

	/// \brief	成交里面包括了订单市场ID，建议用这个ID查询对应的订单
	/// \param  pCxled		               订单被接受以后的消息体，里面包含了市场订单ID
	/// \return void 

	virtual void OnOrderCxled(EES_OrderCxled* pCxled);

	///	撤单被拒绝的消息事件

	/// \brief	一般会在发送撤单以后，收到这个消息，表示撤单被拒绝
	/// \param  pReject	                   撤单被拒绝消息体
	/// \return void 

	virtual void OnCxlOrderReject(EES_CxlOrderRej* pReject);

	///	查询订单的返回事件

	/// \brief	查询订单信息时候的回调，这里面也可能包含不是当前用户下的订单
	/// \param  pAccount                 帐户ID 
	/// \param  pQueryOrder	             查询订单的结构
	/// \param  bFinish	                 如果没有传输完成，这个值是 false，如果完成了，那个这个值为 true    
	/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pQueryOrder值无效。
	/// \return void 

	virtual void OnQueryTradeOrder(const char* pAccount, EES_QueryAccountOrder* pQueryOrder, bool bFinish);

	///	查询订单的返回事件

	/// \brief	查询订单信息时候的回调，这里面也可能包含不是当前用户下的订单成交
	/// \param  pAccount                        帐户ID 
	/// \param  pQueryOrderExec	                查询订单成交的结构
	/// \param  bFinish	                        如果没有传输完成，这个值是false，如果完成了，那个这个值为 true    
	/// \remark 如果碰到 bFinish == true，那么是传输结束，并且pQueryOrderExec值无效。
	/// \return void 

	virtual void OnQueryTradeOrderExec(const char* pAccount, EES_QueryOrderExecution* pQueryOrderExec, bool bFinish);

	///	接收外部订单的消息

	/// \brief	一般会在系统订单出错，进行人工调整的时候用到。
	/// \param  pPostOrder	                    查询订单成交的结构
	/// \return void 

	virtual void OnPostOrder(EES_PostOrder* pPostOrder) {}

	///	接收外部订单成交的消息

	/// \brief	一般会在系统订单出错，进行人工调整的时候用到。
	/// \param  pPostOrderExecution	             查询订单成交的结构
	/// \return void 

	virtual void OnPostOrderExecution(EES_PostOrderExecution* pPostOrderExecution) {}

	///	查询交易所可用连接的响应

	/// \brief	每个当前系统支持的汇报一次，当bFinish= true时，表示所有交易所的响应都已到达，但本条消息本身不包含有用的信息。
	/// \param  pPostOrderExecution	             查询订单成交的结构
	/// \return void 
	virtual void OnQueryMarketSession(EES_ExchangeMarketSession* pMarketSession, bool bFinish) {}

	///	交易所连接状态变化报告，

	/// \brief	当交易所连接发生连接/断开时报告此状态
	/// \param  MarketSessionId: 交易所连接代码
	/// \param  ConnectionGood: true表示交易所连接正常，false表示交易所连接断开了。
	/// \return void 
	virtual void OnMarketSessionStatReport(EES_MarketSessionId MarketSessionId, bool ConnectionGood) {}

	///	合约状态变化报告

	/// \brief	当合约状态发生变化时报告
	/// \param  pSymbolStatus: 参见EES_SymbolStatus合约状态结构体定义
	/// \return void 
	virtual void OnSymbolStatusReport(EES_SymbolStatus* pSymbolStatus) {}


	///	合约状态查询响应

	/// \brief  响应合约状态查询请求
	/// \param  pSymbolStatus: 参见EES_SymbolStatus合约状态结构体定义
	/// \param	bFinish: 当为true时，表示查询所有结果返回。此时pSymbolStatus为空指针NULL
	/// \return void 
	virtual void OnQuerySymbolStatus(EES_SymbolStatus* pSymbolStatus, bool bFinish) {}

	/// 深度行情查询响应
	/// \param	pMarketMBLData: 参见EES_MarketMBLData深度行情结构体定义
	/// \param	bFinish: 当为true时，表示查询所有结果返回。此时pMarketMBLData内容中,仅m_RequestId有效
	/// \return void 
	virtual void OnQueryMarketMBLData(EES_MarketMBLData* pMarketMBLData, bool bFinish) {}

};

class SpiderEesTdSession : public BaseTradeSession
{
public:
	SpiderEesTdSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderEesTdSession();

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
	void init_exoid(int max_exoid);
	//输入C#策略中的3位orderref，然后拼成真实的orderref传给交易服务器
	int get_real_order_ref(int order_ref);

	void insert_market_token(int orderref, long long token);
	void update_deal_qty(int orderref, int qty);
	std::shared_ptr<ees_order> get_ees_order(int orderref);

	const char * get_the_time(unsigned long long int nanosec);

private:
	std::shared_ptr<SpiderEesTdSpi> tradeConnection;

	int exoid;
	std::mutex exoid_mutex;

	std::map<int, std::shared_ptr<ees_order> > orderref_order_map;
	std::mutex order_mutex;

	//缓存单条查询结果，然后一次性组包发送
	std::mutex cache_mutex;
	std::map<int, std::vector<TradingAccount* > > cache_account_info; //key request_id
	std::map<int, std::map<std::string, InvestorPosition * > > cache_positions_info; //2nd key contract#direct
	std::map<int, std::vector<OrderInfo * > > cache_orders_info;
	std::map<int, std::vector<TradeInfo * > > cache_trades_info;
};






#endif