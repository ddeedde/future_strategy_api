// ClassLibrary1.h

#pragma once

#include "ClrSpiderStruct.h"


using namespace System;


namespace Spider {
	
	//==========托管类实现==============
	class SelfCommonApi;
	//行情接口
	public ref class BaseQuotaApi
	{
		SelfCommonApi * selfCommon = NULL;
		String^ AccountID; //登录成功的回调中进行赋值
		EnumAccountType AccountType; //登录成功的回调中进行赋值
	public:
		BaseQuotaApi();

		~BaseQuotaApi();

		bool Init(LoginInfo^ Account);

		void Start();

		void Stop();

		void Restart();

		void Subscribe(array<String^> ^ list);

		void Unsubscribe(array<String^> ^ list);

		event EventHandler<RspError^> ^OnRspError;
		void RaiseRspErrorEvent(SpiderErrorMsg *pRspInfo)
		{
			OnRspError(this, gcnew RspError(pRspInfo));
		}
	
		event Action<BaseQuotaApi^> ^OnConnect;
		void RaiseOnConnectMarketEvent()
		{
			OnConnect(this);
		}
		event Action<BaseQuotaApi^> ^OnDisConnect;
		void RaiseOnDisconnectMarketEvent()
		{
			OnDisConnect(this);
		}
		event Action<BaseQuotaApi^> ^OnLogin;
		void RaiseOnLoginMarketEvent(String^ account_id, EnumAccountType account_type)
		{
			AccountID = account_id;
			AccountType = account_type;
			OnLogin(this);
		}
		event Action<BaseQuotaApi^> ^OnLoginout;
		void RaiseOnLogoutMarketEvent()
		{
			OnLoginout(this);
		}
		event Action<BaseQuotaApi^, Quota^> ^OnDataArrive;
		void RaiseOnDataArriveEvent(QuotaData* md)
		{
			OnDataArrive(this, gcnew Quota(md));
		}
	public:
		String^ getAccountID() { return AccountID; }
		EnumAccountType getAccountType() { return AccountType; }
		array<LoginInfo^>^ getAllAccounts();
	};

	//交易接口
	public ref class BaseTradeApi
	{
		SelfCommonApi * selfCommon;
		String^ AccountID; //登录成功的回调中进行赋值
		EnumAccountType AccountType; //登录成功的回调中进行赋值
	public:
		BaseTradeApi();

		~BaseTradeApi();

		bool Init(LoginInfo^ Account);

		void Start();

		void Stop();

		void Restart();

		String^ InsertOrder(OrderInsertInfo ^ Order);

		void CancelOrder(OrderCancelInfo ^ Order);

		void QueryTradingAccount(int request_id);
		void QueryPositions(int request_id);
		void QueryOrders(int request_id);
		void QueryTrades(int request_id);

		event EventHandler<RspError^> ^OnRspError;
		void RaiseRspErrorEvent(SpiderErrorMsg *pRspInfo)
		{
			OnRspError(this, gcnew RspError(pRspInfo));
		}

		event Action<BaseTradeApi^> ^OnConnect;
		void RaiseOnConnectTradeEvent()
		{
			OnConnect(this);
		}
		event Action<BaseTradeApi^> ^OnDisConnect;
		void RaiseOnDisconnectTradeEvent()
		{
			OnDisConnect(this);
		}
		event Action<BaseTradeApi^> ^OnLogin;
		void RaiseOnLoginTradeEvent(String^ account_id, EnumAccountType account_type)
		{
			AccountID = account_id;
			AccountType = account_type;
			OnLogin(this);
		}
		event Action<BaseTradeApi^> ^OnLoginout;
		void RaiseOnLogoutTradeEvent()
		{
			OnLoginout(this);
		}

		event Action<BaseTradeApi^,OrderInforField^> ^OnRtnOrder;
		void RaiseOnRtnOrderEvent(OrderInfo * order)
		{
			OnRtnOrder(this,gcnew OrderInforField(order));
		}
		event Action<BaseTradeApi^, TradeInforField^> ^OnRtnTrade;
		void RaiseOnRtnTradeEvent(TradeInfo * trade)
		{
			OnRtnTrade(this, gcnew TradeInforField(trade));
		}
		event Action<BaseTradeApi^, OrderInforField^> ^OnRspCancel;
		void RaiseOnRspCancelEvent(OrderInfo * order)
		{
			OnRspCancel(this, gcnew OrderInforField(order));
		}
		event Action<BaseTradeApi^, OrderInforField^> ^OnRspOrderInsert;
		void RaiseOnRspOrderInsertEvent(OrderInfo * order)
		{
			OnRspOrderInsert(this, gcnew OrderInforField(order));
		}

		event Action<BaseTradeApi^, array<OrderInforField^>^, int> ^OnRspQryOrder;
		void RaiseOnRspQryOrderEvent(array<OrderInforField^>^ orders, int RequestID)
		{
			OnRspQryOrder(this, orders, RequestID);
		}

		event Action<BaseTradeApi^, array<TradeInforField^>^, int> ^OnRspQryTrade;
		void RaiseOnRspQryTradeEvent(array<TradeInforField^>^ trades, int RequestID)
		{
			OnRspQryTrade(this, trades, RequestID);
		}

		event Action<BaseTradeApi^, array<TradingAccountField^>^, int> ^OnRspQryTradingAccount;
		void RaiseOnRspQryTradingAccountEvent(array<TradingAccountField^>^ accounts, int RequestID)
		{
			OnRspQryTradingAccount(this, accounts, RequestID);
		}

		event Action<BaseTradeApi^, array<InvestorPositionField^>^, int> ^OnRspQryPosition;
		void RaiseOnRspQryPositionEvent(array<InvestorPositionField^>^ positions, int RequestID)
		{
			OnRspQryPosition(this, positions, RequestID);
		}

	public:
		String^ getAccountID() { return AccountID; }
		EnumAccountType getAccountType() {return AccountType;}
		array<LoginInfo^>^ getAllAccounts();
	};

	//===非托管===================
	class QuoteSingleThreadWrapper;
	public class SelfCommonApi : public SpiderSpi
	{
		SpiderApi * nestSpider;
		gcroot<BaseQuotaApi^>* mdClr;
		gcroot<BaseTradeApi^>* tdClr;
		static QuoteSingleThreadWrapper quoteWrapper;
	public:
		SelfCommonApi(BaseQuotaApi^ clr);
		SelfCommonApi(BaseTradeApi^ clr);
		~SelfCommonApi();

		bool Init(LoginInfo^ Account);
		void Release();
		array<LoginInfo^>^ getAllAccounts(bool ifTrade = false);

		//行情相关
		void StartMarket() { this->nestSpider->marketStart(); }
		void StopMarket() { this->nestSpider->marketStop(); }
		void RestartMarket() { this->nestSpider->marketRestart(); }
		void SubscribeMarket(array<String^> ^ list);
		void UnsubscribeMarket(array<String^> ^ list);
		
		//交易相关
		void StartTrade() { this->nestSpider->tradeStart(); }
		void StopTrade() { this->nestSpider->tradeStop(); }
		void RestartTrade() { this->nestSpider->tradeRestart(); }
		String^ InsertOrder(OrderInsertInfo^ order);
		void CancelOrder(OrderCancelInfo^ order);
		void QueryTradingAccount(int request_id);
		void QueryPositions(int request_id);
		void QueryOrders(int request_id);
		void QueryTrades(int request_id);

		//回调
		virtual void onError(SpiderErrorMsg *);

		virtual void marketOnConnect();
		virtual void marketOnDisconnect();
		virtual void marketOnLogin(const char *, int);
		virtual void marketOnLogout();
		virtual void marketOnDataArrive(QuotaData *);

		virtual void tradeOnConnect();
		virtual void tradeOnDisconnect();
		virtual void tradeOnLogin(const char *, int);
		virtual void tradeOnLogout();
		virtual void tradeRtnOrder(OrderInfo *);
		virtual void tradeRtnTrade(TradeInfo *);
		virtual void tradeOnOrderInsert(OrderInfo *);
		virtual void tradeOnOrderCancel(OrderInfo *);
		virtual void tradeRspQueryAccount(TradingAccount **, int, int);
		virtual void tradeRspQueryPosition(InvestorPosition **, int, int);
		virtual void tradeRspQueryTrade(TradeInfo **, int, int);
		virtual void tradeRspQueryOrder(OrderInfo **, int, int);

	};


}
