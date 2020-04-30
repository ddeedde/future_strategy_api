//在这里面定义数据结构体和字段定义

#if !defined(SPIDER_FUTURESTRUCT_H)
#define SPIDER_FUTURESTRUCT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//4 Bytes
enum EnumExchangeIDType
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

enum EnumAccountType
{
	AccountUnset = 0,
	AccountTradeFuture = 1,
	AccountMarketFuture = 2,
	AccountMarketIndex = 3
};

struct SpiderErrorMsg
{
	int ErrCode;
	char ErrMsg[128];
};

//200 Bytes
struct QuotaData
{
	int ExchangeID;
	int UpdateMillisec;
	char TradingDay[16];
	char Code[16];
	char UpdateTime[16];
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
};

struct AccountInfo //该结构体不用在c#的cli中
{
	int account_type;
	char broker_id[12];
	char front_id[16];
	char session_id[16];
	char account_id[16];
	char password[16];
	char product_id[16]; //看穿式认证
	char app_id[32]; //看穿式认证
	char auth_code[32]; //看穿式认证
	//char local_mac[32]; //本机的mac地址，盛立要求
	char uri_list[5][128]; //暂时只支持一个账户，最多5个地址
	char broker_account_id[16]; //真正的柜台的资金账户，上面的account_id，可能只是一个登录子用户
};


struct OrderInsert
{
	char Code[16];
	char OrderRef[16];
	int ExchangeID;
	int Direction;
	int HedgeFlag;
	int Offset;	
	double LimitPrice;
	int VolumeTotalOriginal;
};

struct OrderCancel
{
	char OrderRef[16];
	char OrderSysID[32];
	char Code[16];
	int ExchangeID;
};

enum EnumDirectionType
{
	Buy = 1,
	Sell = 2
};

enum EnumHedgeFlagType
{
	Arbitrage = 1,
	Hedge = 2,
	Speculation = 3
};

enum EnumOffsetFlagType
{
	Open = 1,
	Close = 2,
	CloseToday = 3,
	CloseYesterday = 4
};

enum EnumOrderStatusType
{
	Failed = -1,
	StatusUnknown = 0,
	AllTraded = 1,
	NoTradeQueueing = 2,
	Canceled = 3
};

struct OrderInfo
{
	char Code[16];
	char OrderRef[16];
	char OrderSysID[32];
	int ExchangeID;
	int Direction;	
	int HedgeFlag;
	int Offset;
	double LimitPrice;
	int VolumeTotalOriginal;
	int VolumeTotal;
	int VolumeTraded;	
	int OrderStatus;
	char StatusMsg[64];
	char OrderTime[16];
};

struct TradeInfo
{
	char Code[16];
	char OrderRef[16];
	char OrderSysID[32];
	char TradeID[16];
	char TradeTime[16];
	int Direction;
	int ExchangeID;
	int HedgeFlag;
	int Offset;
	double Price;
	int Volume;
};

struct TradingAccount
{
	char AccountID[16];
	double Available;
	double Balance;
	double CloseProfit;
	double Commission;
	char Currency[8];
	double CurrMargin;
	double Deposit;
	double FrozenCash;
	double FrozenCommission;
	double FrozenMargin;
	double PositionProfit;
	double PreBalance;
	char TradingDay[16];
	double Withdraw;
	double WithdrawQuota;
};

enum EnumPosiDirectionType
{
	Net = 1,
	Long = 2,
	Short = 3
};
struct InvestorPosition
{
	char AccountID[16];
	char Code[16];
	int HedgeFlag;
	double OpenCost;
	int PosiDirection;
	int Position;
	double PositionCost;
	double ProfitLoss;
	//int StockAvailable;
	int TodayPosition;
	int YesterdayPosition;
};

#endif