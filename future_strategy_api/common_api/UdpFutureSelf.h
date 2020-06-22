#if !defined(SPIDER_UDPFUTURESELF_H)
#define SPIDER_UDPFUTURESELF_H

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
#include "boost/bind.hpp"



//期货行情，共200字节
struct MarketDataFuture
{
	unsigned int Type;			// 类型代码，MARKET_DATA_TYPE_FUTURE 或 MARKET_DATA_TYPE_FUTURE_SNAP
	unsigned int Seq;			// 序号
	char ExID[8];				// 交易所代码
	char SecID[24];				// 证券代码
	unsigned int Time;			// 交易所时间，精确到毫秒，Int格式 10:01:02 000 = 100102000
	unsigned int Volume;		// 累计成交量
	long long Turnover;			// 累计成交金额, 单位：元
	long long PreSettlement;	// 昨结算价，单位：0.0001元
	long long PreClose;			// 昨收盘，单位：0.0001元
	long long PreOpenInterest;	// 昨持仓量
	long long Match;			// 最新价，单位：0.0001元
	long long Open;				// 今开盘，单位：0.0001元
	long long High;				// 最高价，单位：0.0001元
	long long Low;				// 最低价，单位：0.0001元
	long long HighLimited;		// 涨停板价，单位：0.0001元
	long long LowLimited;		// 跌停板价，单位：0.0001元
	long long Settlement;		// 今结算价，单位：0.0001元
	long long Close;			// 今收盘，单位：0.0001元
	long long OpenInterest;		// 今持仓量
	long long BidPrice;			// 买价，单位：0.0001元
	long long BidVol;			// 买量
	long long AskPrice;			// 卖价，单位：0.0001元
	long long AskVol;			// 卖量
	long long LocalTime;		// 本地时间，精确到微秒，Int格式，"10:01:02 000 000" = 100102000000
	unsigned int NatureDate;
	unsigned int Reserved1;

	MarketDataFuture() { memset(this, 0, sizeof(MarketDataFuture)); }
	std::string toString()
	{
		std::stringstream ss;
		auto md = this;
		ss << md->Type << "," << md->Seq << "," << md->ExID << "," << md->SecID << ","
			<< md->PreSettlement << "," << md->PreClose << "," << md->PreOpenInterest << "," << md->Match << ","
			<< md->Open << "," << md->High << "," << md->Low << "," << md->Volume << ","
			<< md->Turnover << "," << md->HighLimited << "," << md->LowLimited << "," << md->Settlement << ","
			<< md->Close << "," << md->OpenInterest << "," << md->Time << "," << md->LocalTime << ","
			<< md->BidPrice << "," << md->BidVol << "," << md->AskPrice << "," << md->AskVol << ",\r\n";
		return ss.str();
	}
};


class SpiderUdpFutureSession;
class SpiderUdpFutureSpi
{
	//接收二进制组播
	SpiderUdpFutureSession * smd;
public:
	SpiderUdpFutureSpi();
	~SpiderUdpFutureSpi();

	bool init(SpiderUdpFutureSession * sm);
	void start();

	long long get_not_microsec();

private:
	void async_receive();
	void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);
	void on_start();
	void send(const std::string & msg);
	void on_send(const boost::system::error_code &ec, std::size_t bytes);
	void ping();
private:
	boost::asio::io_service io;
	boost::asio::io_service::work* io_keeper;
	boost::asio::ip::udp::socket udp_socket;
	boost::scoped_ptr<boost::thread> io_thread;
	boost::asio::deadline_timer ping_timer;
	boost::asio::ip::udp::endpoint udp_sender_endpoint;
	boost::asio::ip::udp::endpoint udp_recv_endpoint;

	char m_data[10240];
	std::string udp_ip;
	int udp_port;
	int recv_count;
};


class SpiderUdpFutureSession : public BaseMarketSession
{
public:
	SpiderUdpFutureSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderUdpFutureSession();

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
	std::shared_ptr<SpiderUdpFutureSpi> marketConnection;
};


#endif