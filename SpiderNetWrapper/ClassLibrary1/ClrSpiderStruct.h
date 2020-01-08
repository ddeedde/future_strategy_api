#pragma once

#include "SpiderCommonApi.h"

using namespace System;


namespace Spider {

	public enum class EnumExchangeIDType 
	{
		IB = 49,
		XBXG = 50,
		INE = 51,
		OKEX = 52,
		HUOBI = 53,
		BIAN = 54,
		BITMEX = 55,
		ZB = 56,
		APOROO = 57,
		SSE = 65,
		BOCE = 66,
		CME = 67,
		DCE = 68,
		CME_CBT = 69,
		SZSE = 70,
		SGE = 71,
		eCBOT = 72,
		NYBOT = 73,
		CFFEX = 74,
		KRX = 75,
		XEurex = 76,
		SGXQ = 77,
		TOCOM = 78,
		HKEX = 79,
		LME = 80,
		Liffe = 81,
		BMD = 82,
		SHFE = 83,
		SFE = 84,
		ICE = 85,
		DME = 86,
		JPX = 87,
		EURONEXT = 88,
		IPE = 89,
		CZCE = 90,
		EXX = 97,
		BITASSET = 98,
		BITFINEX = 99,
		BITSTAMP = 100,
		COINBASE = 101,
		KRAKEN = 102
	};

	public enum class EnumAccountType 
	{
		AccountUnset = 0,
		AccountTradeFuture = 1,
		AccountMarketFuture = 2,
		AccountMarketIndex = 3
	};


	public ref struct RspError
	{
	public:
		int ErrorID;
		String^ ErrorMsg;
		RspError()
		{}
		RspError(SpiderErrorMsg *pRspInfo)
		{
			this->ErrorID = pRspInfo->ErrCode;
			this->ErrorMsg = gcnew String(pRspInfo->ErrMsg);
		}
	};

	public ref struct LoginInfo
	{
		String^ AccountID;
		EnumAccountType AccountType;
		LoginInfo()
		{

		}
		LoginInfo(AccountInfo* pAcc)
		{
			this->AccountID = gcnew String(pAcc->account_id);
			this->AccountType = (EnumAccountType)pAcc->account_type;
		}
	};

	public ref struct Quota
	{
		EnumExchangeIDType ExchangeID;
		int UpdateMillisec;
		String^ TradingDay;
		String^ Code;
		String^ UpdateTime;
		double AskPrice1;
		double AskVolume1;
		double BidPrice1;
		double BidVolume1;
		double LastPrice;
		double HighestPrice;
		double LowestPrice;
		double LowerLimitPrice;
		double UpperLimitPrice;
		double OpenPrice;
		double PreClosePrice;
		double ClosePrice;
		double PreSettlementPrice;
		double SettlementPrice;
		double PreOpenInterest;
		double OpenInterest;
		double Turnover;
		long long Volume;
		Quota()
		{

		}
		Quota(QuotaData *pRspInfo)
		{
			this->ExchangeID = (Spider::EnumExchangeIDType)pRspInfo->ExchangeID;
			this->TradingDay = gcnew String(pRspInfo->TradingDay);
			this->Code = gcnew String(pRspInfo->Code);
			this->UpdateTime = gcnew String(pRspInfo->UpdateTime);
			this->AskPrice1 = pRspInfo->AskPrice1;
			this->AskVolume1 = pRspInfo->AskVolume1;
			this->BidPrice1 = pRspInfo->BidPrice1;
			this->BidVolume1 = pRspInfo->BidVolume1;
			this->LastPrice = pRspInfo->LastPrice;
			this->HighestPrice = pRspInfo->HighestPrice;
			this->LowestPrice = pRspInfo->LowestPrice;
			this->LowerLimitPrice = pRspInfo->LowerLimitPrice;
			this->UpperLimitPrice = pRspInfo->UpperLimitPrice;
			this->OpenPrice = pRspInfo->OpenPrice;
			this->PreClosePrice = pRspInfo->PreClosePrice;
			this->ClosePrice = pRspInfo->ClosePrice;
			this->PreSettlementPrice = pRspInfo->PreSettlementPrice;
			this->SettlementPrice = pRspInfo->SettlementPrice;
			this->PreOpenInterest = pRspInfo->PreOpenInterest;
			this->OpenInterest = pRspInfo->OpenInterest;
			this->Turnover = pRspInfo->Turnover;
			this->Volume = pRspInfo->Volume;
		}
	};

	public enum class EnumDirectionType
	{
		Buy = 1,
		Sell = 2
	};

	public enum class EnumHedgeFlagType
	{
		Arbitrage = 1,
		Hedge = 2,
		Speculation = 3
	};

	public enum class EnumOffsetFlagType
	{
		Open = 1,
		Close = 2,
		CloseToday = 3,
		CloseYesterday = 4
	};

	public ref struct OrderInsertInfo
	{
		String^ Code;
		EnumExchangeIDType ExchangeID;
		EnumDirectionType Direction;
		EnumHedgeFlagType HedgeFlag;
		EnumOffsetFlagType Offset;
		String^ OrderRef;
		double LimitPrice;
		int VolumeTotalOriginal;
		OrderInsertInfo(){}
		OrderInsertInfo(OrderInsert * order)
		{
			this->Code = gcnew String(order->Code);
			this->OrderRef = gcnew String(order->OrderRef);
			this->Direction = (EnumDirectionType)order->Direction;
			this->ExchangeID = (EnumExchangeIDType)order->ExchangeID;
			this->HedgeFlag = (EnumHedgeFlagType)order->HedgeFlag;
			this->Offset = (EnumOffsetFlagType)order->Offset;
			this->LimitPrice = order->LimitPrice;
			this->VolumeTotalOriginal = order->VolumeTotalOriginal;
		}
	};

	public ref struct OrderCancelInfo
	{
		String^ OrderRef;
		String^ OrderSysID;
		String^ Code;
		EnumExchangeIDType ExchangeID;
	};

	public enum class EnumOrderStatusType
	{
		Failed = -1,
		StatusUnknown = 0,
		AllTraded = 1,
		NoTradeQueueing = 2,
		Canceled = 3
	};

	public ref struct OrderInforField
	{
		String^ Code;
		String^ OrderRef;
		String^ OrderSysID;
		EnumExchangeIDType ExchangeID;
		EnumDirectionType Direction;
		EnumHedgeFlagType HedgeFlag;
		EnumOffsetFlagType Offset;
		double LimitPrice;
		int VolumeTotalOriginal;
		int VolumeTotal;
		int VolumeTraded;
		EnumOrderStatusType OrderStatus;
		String^ StatusMsg;
		String^ OrderTime;
		OrderInforField(){}
		OrderInforField(OrderInfo * order)
		{
			this->Code = gcnew String(order->Code);
			this->OrderRef = gcnew String(order->OrderRef);
			this->OrderSysID = gcnew String(order->OrderSysID);
			this->Direction = (EnumDirectionType)order->Direction;
			this->ExchangeID = (EnumExchangeIDType)order->ExchangeID;
			this->HedgeFlag = (EnumHedgeFlagType)order->HedgeFlag;
			this->Offset = (EnumOffsetFlagType)order->Offset;
			this->LimitPrice = order->LimitPrice;
			this->VolumeTotalOriginal = order->VolumeTotalOriginal;
			this->VolumeTotal = order->VolumeTotal;
			this->VolumeTraded = order->VolumeTraded;
			this->OrderStatus = (EnumOrderStatusType)order->OrderStatus;
			this->StatusMsg = gcnew String(order->StatusMsg);
			this->OrderTime = gcnew String(order->OrderTime);
		}
	};

	public ref struct TradeInforField
	{
		String^ Code;
		String^ OrderRef;
		String^ OrderSysID;
		String^ TradeID;
		String^ TradeTime;
		EnumDirectionType Direction;
		EnumExchangeIDType ExchangeID;
		EnumHedgeFlagType HedgeFlag;
		EnumOffsetFlagType Offset;
		double Price;
		int Volume;
		TradeInforField() {}
		TradeInforField(TradeInfo * trade)
		{
			this->Code = gcnew String(trade->Code);
			this->OrderRef = gcnew String(trade->OrderRef);
			this->OrderSysID = gcnew String(trade->OrderSysID);
			this->TradeID = gcnew String(trade->TradeID);
			this->TradeTime = gcnew String(trade->TradeTime);
			this->Direction = (EnumDirectionType)trade->Direction;
			this->ExchangeID = (EnumExchangeIDType)trade->ExchangeID;
			this->HedgeFlag = (EnumHedgeFlagType)trade->HedgeFlag;
			this->Offset = (EnumOffsetFlagType)trade->Offset;
			this->Price = trade->Price;
			this->Volume = trade->Volume;
		}
	};

	public ref struct TradingAccountField
	{
		String^ AccountID;
		double Available;
		double Balance;
		double CloseProfit;
		double Commission;
		String^ Currency;
		double CurrMargin;
		double Deposit;
		double FrozenCash;
		double FrozenCommission;
		double FrozenMargin;
		double PositionProfit;
		double PreBalance;
		String^ TradingDay;
		double Withdraw;
		double WithdrawQuota;
		TradingAccountField() {}
		TradingAccountField(TradingAccount* acc)
		{
			this->AccountID = gcnew String(acc->AccountID);
			this->Currency = gcnew String(acc->Currency);
			this->TradingDay = gcnew String(acc->TradingDay);
			this->Available = acc->Available;
			this->Balance = acc->Balance;
			this->Available = acc->Available;
			this->CloseProfit = acc->CloseProfit;
			this->Commission = acc->Commission;
			this->CurrMargin = acc->CurrMargin;
			this->Deposit = acc->Deposit;
			this->FrozenCash = acc->FrozenCash;
			this->FrozenCommission = acc->FrozenCommission;
			this->FrozenMargin = acc->FrozenMargin;
			this->PositionProfit = acc->PositionProfit;
			this->PreBalance = acc->PreBalance;
			this->Withdraw = acc->Withdraw;
			this->WithdrawQuota = acc->WithdrawQuota;

		}
	};
	public enum class EnumPosiDirectionType
	{
		Net = 1,
		Long = 2,
		Short = 3
	};
	public ref struct InvestorPositionField
	{
		String^ AccountID;
		String^ Code;
		EnumHedgeFlagType HedgeFlag;
		double OpenCost;
		EnumPosiDirectionType PosiDirection;
		int Position;
		double PositionCost;
		double ProfitLoss;
		int TodayPosition;
		int YesterdayPosition;
		InvestorPositionField() {}
		InvestorPositionField(InvestorPosition* pos)
		{
			this->AccountID = gcnew String(pos->AccountID);
			this->Code = gcnew String(pos->Code);
			this->HedgeFlag = (EnumHedgeFlagType)pos->HedgeFlag;
			this->PosiDirection = (EnumPosiDirectionType)pos->PosiDirection;
			this->OpenCost = pos->OpenCost;
			this->Position = pos->Position;
			this->PositionCost = pos->PositionCost;
			this->ProfitLoss = pos->ProfitLoss;
			this->TodayPosition = pos->TodayPosition;
			this->YesterdayPosition = pos->YesterdayPosition;
		}
	};
}