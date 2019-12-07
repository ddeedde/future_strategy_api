#if !defined(SPIDER_MULTIINDEXAPI_H)
#define SPIDER_MULTIINDEXAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "MdApi.h"
#include "SpiderApiStruct.h"
#include "boost/asio.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
#include "boost/shared_ptr.hpp"


struct MarketDataIndex
{
	//120 Bytes
	unsigned int Type;		//300股票
	unsigned int Seq;		//保留行情源中的行情seq号
	char ExID[8];			//交易所代码
	char SecID[8];			//证券代码
	char SecName[16];		//证券名称		
	int	SourceID;				//行情来源		
	unsigned int Time;		//Int格式的时间 10:01:02 000 = 100102000
	unsigned int PreClose;		//前收盘
	unsigned int Open;			// 当日开盘价
	unsigned int High;			// 当日最高价
	unsigned int Low;			// 当日最低价
	unsigned int Match;			// 最新价
	unsigned int HighLimited;	// 涨停板价格
	unsigned int LowLimited;	// 跌停板价格
	unsigned int NumTrades;		// 成交笔数
	long long Volume;			// 成交量
	long long Turnover;			// 成交金额
	long long AdjVolume;		// 调整后的成交量，和上一条的差
	long long AdjTurnover;		// 调整后的成交金额，和上一条的差
	unsigned int LocalTime;		// 本地时间，精确到毫秒，Int格式 10:01:02 000 = 100102000
	int	Reserved1;
	MarketDataIndex() { memset(this, 0, sizeof(MarketDataIndex)); }
};
typedef boost::shared_ptr<MarketDataIndex> MarketDataIndexPtr;

class SpiderMultiIndexSession;
class SpiderMultiIndexSpi
{
	//接收二进制组播
	SpiderMultiIndexSession * smd;
public:
	SpiderMultiIndexSpi();
	~SpiderMultiIndexSpi();

	bool init(SpiderMultiIndexSession * sm);
	void start();
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
	int recv_count;
};


class SpiderMultiIndexSession : public BaseMarketSession
{
public:
	SpiderMultiIndexSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderMultiIndexSession();

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
	std::shared_ptr<SpiderMultiIndexSpi> marketConnection;

};


#endif