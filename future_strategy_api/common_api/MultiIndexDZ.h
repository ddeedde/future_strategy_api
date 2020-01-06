#if !defined(SPIDER_MULTIINDEXAPIDZ_H)
#define SPIDER_MULTIINDEXAPIDZ_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "MdApi.h"
#include "SpiderApiStruct.h"
#include "boost/asio.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/date_time.hpp"

//东证期货机房中收取指数行情，通过组播的方式
namespace Index_DZ
{
#pragma pack(1)
	// 逐笔成交
	struct TickData
	{
		char    SecurityExchange[8];  // 交易所，上海:"SHL2"，深圳:"SZL2"
		char    SecurityID[8];        // 证券代码
		char    LastUpdateTime[32];   // ISO8601格式行情更新时间 例如 2019-11-26T13:25:26+08:00(上海)，2019-11-25T14:01:20.13+08:00(深圳)
		double  Price;                // 成交价格
		int64_t Volume;               // 成交量
		char    Channel[12];          // 成交通道
		char    TradeNo[12];          // 成交编号
		double  Amount;               // 成交金额
		char    BuyNo[12];            // 委买编号
		char    SellNo[12];           // 委卖编号
		char    BSFlag;               // 成交方向 T成交 B买入成交 S卖出成交 C撤单
	};

	// 逐笔委托(仅深圳有)
	struct Order
	{
		char    SecurityExchange[8];   // 交易所，上海:"SHL2"，深圳:"SZL2"
		char    SecurityID[8];         // 证券代码
		char    UpdateTime[32];        // 时间戳
		double  Price;                 // 委托价
		int64_t Volume;                // 委托量
		char    Channel[12];           // 委托通道
		char    OrderNo[12];           // 委托编号
		char    BSFlag;                // 委托方向 T不确定 B买 S卖 G借入 F借出
		char    OrderType;             // 委托类型 1:市价委托 2:限价委托 U:本方最优
	};

	// 档位数据结构体
	struct EntrySnap
	{
		double  MDEntryPx;             // 买/卖价格
		int64_t MDEntrySize;           // 买/卖数量
		int32_t MDEntryPositionNo;     // 档位序号(从0开始)
	};

	// 静态数据结构体
	struct StaticInfoEntry
	{
		char    Symbol[16];           // 证券名称(UTF-8)
		double  OpenPx;               // 开盘价
		double  ClosePx;              // 今收盘价
		double  HighLimitPx;          // 涨停价
		double  LowLimitPx;           // 跌停价
		double  PrevSetPx;            // 昨结算价(仅期权)
		double  PrevClosePx;          // 昨收盘价
		char    LastUpdateTime[36];   // ISO8601格式静态数据更新时间
	};


	// 交易状态FixDTradingStatus字段说明：
	// D 默认、延续状态 *应视为可正常交易
	// S 开盘前
	// C 开盘集合竞价、盘前交易
	// T 正常交易、连续撮合
	// F 盘后固定价格交易
	// B 休市
	// P 停牌
	// H 临时停牌、临时停市、波动性中断（期权）
	// M 可恢复熔断、不可恢复熔断
	// I 盘中集合竞价
	// U 收盘集合竞价、盘后交易
	// A 收盘后
	// E 闭市

	// 竞价交易状态TradingPhaseCode字段说明
	// 上海
	// START 启动
	// OCALL 开市集合竞价
	// TRADE 连续自动撮合
	// SUSP 停牌
	// CCALL 收盘集合竞价
	// CLOSE 闭市，自动计算闭市价格
	// ENDTR 交易结束
	// VOLA 连续交易和集合竞价交易的波动性中断
	// -------------------------------------
	// 深圳
	// 第0位
	// S 启动(开始前)
	// O 开盘集合竞价
	// T 连续竞价
	// B 休市
	// C 收盘集合竞价
	// E 已闭市
	// H 临时停牌
	// A 盘后交易
	// V 波动性中断
	// 第1位
	// 0 正常状态
	// 1 全天停牌
	// 深圳TradingPhaseCode字段剩余位用空格(ASCII码0x20)填满，最后不含字符串结束符'\0'

	struct Snapshot
	{
		char            SecurityExchange[8];   // 交易所，上海:"SHL2"，深圳:"SZL2"
		char            SecurityID[8];         // 证券代码
		char            FixDTradingStatus[8];  // 交易状态 参考上方FixDTradingStatus字段说明
		char            LastUpdateTime[32];    // ISO8601格式行情更新时间 例如 "2019-11-26T13:25:26+08:00"(上海)，"2019-11-25T14:01:20.13+08:00"(深圳)
		double          LastPx;                // 最新价
		double          HighPx;                // 最高价
		double          LowPx;                 // 最低价
		char            TradingPhaseCode[8];   // 竞价交易状态 参考上方TradingPhaseCode字段说明
		int64_t         NumTrades;             // 交易笔数
		int64_t         TotalVolumeTraded;     // 成交量
		double          TotalValueTraded;      // 总成交额
		double          IOPV;                  // 参考净值(仅基金)
		int64_t         TotalLongPosition;     // 总持仓量(仅期权)	
		uint32_t        BuySeqDepth;           // 买档位数量
		EntrySnap       MDEntryBuyer[10];      // 各档买行情
		uint32_t        SellSeqDepth;          // 卖档位数量
		EntrySnap       MDEntrySeller[10];     // 各档卖行情
		int32_t         BuyNumOrders;		   // 买实际委托笔数
		int32_t 	    BuyNoOrders[50];       // 50笔买委托
		int32_t         SellNumOrders;		   // 卖实际委托笔数
		int32_t 	    SellNoOrders[50];      // 50笔卖委托
		int64_t         TotalBidQty;           // 委托买总量
		double          WeightedAvgBidPx;      // 买加权均价
		int64_t         TotalOfferQty;         // 委托卖总量
		double          WeightedAvgOfferPx;    // 卖加权均价
		StaticInfoEntry StaticInfo;            // 静态数据
	};

	union DataField
	{
		Snapshot snapshot;      // 行情快照
		Order order;            // 逐笔委托(仅深圳有)
		TickData tick;          // 逐笔成交
	};

	// 行情结构体
	// MDStreamID字段说明：
	// MDStreamID为'E'股票 'O'期权 'W'权证 'B'债券 'I'指数 'F' 基金 对应data为行情快照Snapshot结构体
	// MDStreamID为"TI"对应data为逐笔成交TickData结构体
	// MDStreamID为"SO"对应data为逐笔委托Order结构体

	struct MarketDataField
	{
		char MDStreamID[8];         // 类型，'E' 'O' "TI" "SO"等，参考上方MDStreamID字段说明
		union DataField data;       // 数据
	};
#pragma pack()

}


class SpiderMultiIndexDZSession;
class SpiderMultiIndexDZSpi
{
	//接收二进制组播
	SpiderMultiIndexDZSession * smd;
public:
	SpiderMultiIndexDZSpi();
	~SpiderMultiIndexDZSpi();

	bool init(SpiderMultiIndexDZSession * sm);
	void start();

	long long get_not_microsec();

private:
	void async_receive();
	void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);
	void on_start(const boost::system::error_code& error);
private:
	boost::asio::io_service io;
	boost::asio::io_service::work* io_keeper;
	boost::asio::ip::udp::socket mul_socket;
	boost::asio::ip::udp::endpoint mul_sender_endpoint;
	boost::scoped_ptr<boost::thread> io_thread;
	boost::asio::deadline_timer start_timer;

	char m_data[10240];
	std::string mul_ip;
	int mul_port;
	std::string local_ip;
	int recv_count;
};


class SpiderMultiIndexDZSession : public BaseMarketSession
{
public:
	SpiderMultiIndexDZSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderMultiIndexDZSession();

	virtual bool init();
	virtual void start();
	virtual void stop();

	void subscribe(std::vector<std::string> & list);
	void unsubscribe(std::vector<std::string> & list);

	virtual void on_connected();
	virtual void on_disconnected();
	virtual void on_log_in();
	virtual void on_log_out();
	virtual void on_error(std::shared_ptr<SpiderErrorMsg> & err_msg);
	virtual void on_receive_data(QuotaData * md);

private:
	std::shared_ptr<SpiderMultiIndexDZSpi> marketConnection;
};


#endif