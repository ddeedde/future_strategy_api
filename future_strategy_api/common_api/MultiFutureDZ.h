#if !defined(SPIDER_MULTFUTUREAPIDZ_H)
#define SPIDER_MULTFUTUREAPIDZ_H

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

//东证期货机房中收取Level的中金所期货，通过组播的方式
namespace Future_DZ
{
#pragma pack(1)
	struct pankou_info
	{
		int16_t head;
		int16_t length;
		double bid_price;
		int bid_vol;
		double ask_price;
		int ask_vol;
	};
	struct level2_future_zj
	{
		char unkonw[8];
		int seq;
		int16_t struct_num;
		int16_t length;
		char reserved[4];
		int16_t head1;
		int16_t length1;
		char insid[31];
		char modify_time[9];
		int modify_milisec;
		int16_t head2;
		int16_t length2;
		double open_price;
		double high_price;
		double low_price;
		double close_price;
		double high_limit_price;
		double low_limit_price;
		double settle_price;
		double curr_delta;
		int16_t head3;
		int16_t length3;
		double latest_price;
		int volume;
		double turnover;
		double open_interest;
		pankou_info pankou[5];
	};
#pragma pack()
};

class SpiderMultiFutureDZSession;
class SpiderMultiFutureDZSpi
{
	//接收二进制组播
	SpiderMultiFutureDZSession * smd;
public:
	SpiderMultiFutureDZSpi();
	~SpiderMultiFutureDZSpi();

	bool init(SpiderMultiFutureDZSession * sm);
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


class SpiderMultiFutureDZSession : public BaseMarketSession
{
public:
	SpiderMultiFutureDZSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderMultiFutureDZSession();

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
	std::shared_ptr<SpiderMultiFutureDZSpi> marketConnection;
};


#endif