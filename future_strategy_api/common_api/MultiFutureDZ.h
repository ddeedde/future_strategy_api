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
	//原始定义
#pragma pack(1)
	struct CffexHead
	{
		char unknown[8];
		int32_t index;
		int16_t struct_num;
		int16_t length;
		char reserved[4];
	};
	// 2439和2432没有加密，明文解析即可
	struct Cffex2439
	{
		int16_t head;
		int16_t length;
		char contract[31];
		char update_time[9];
		int32_t update_msec;
	};
	struct Cffex2432
	{
		int16_t head;
		int16_t length;
		double open_price;
		double highest_price;
		double lowest_price;
		double close_price;
		double upper_limit;
		double lower_limit;
		double settlement_price;
		double curr_delta;
	};
	//2433-2438需要解密解析；原始数据的五档盘口需要更改次序。
	struct Cffex2433
	{
		int16_t head;
		int16_t length;
		double last_price;
		int32_t volume;
		double turnover;
		double open_interest;
	};
	struct Cffex2434
	{
		int16_t head;
		int16_t length;
		double bid_price;
		int32_t bid_volume;
		double ask_price;
		int32_t ask_volume;
	};
	struct CffexData
	{
		CffexHead head;
		Cffex2439 d2439;
		Cffex2432 d2432;
		Cffex2433 d2433;
		Cffex2434 d2434[5];
	};
#pragma pack()

	//我们自己的等价定义
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
//below 2 created on 2020-01-08
inline double decrypt_strategy_double_1(double data)
{
	unsigned char * ptr = (unsigned char *)(&data);
	if (0x7f == ptr[0] && 0xef == ptr[1]) {
		ptr[3] = ptr[3] ^ ptr[2];
		ptr[7] = ptr[7] ^ ptr[6];
	}
	else if (0x40 == ptr[0])
	{
		ptr[3] = ptr[3] ^ ptr[2];
		ptr[7] = ptr[7] ^ ptr[6];
	}
	else if (0x41 == ptr[0] && ptr[1] >= 0x40)
	{
		ptr[3] = ptr[3] ^ ptr[2];
		ptr[7] = ptr[7] ^ ptr[6];
	}
	else {
		ptr[2] = ptr[3] ^ ptr[2];
		ptr[6] = ptr[7] ^ ptr[6];
	}
	return (double)(*(double*)ptr);
}
inline int decrypt_strategy_int_1(int data)
{
	char* ptr = (char *)(&data);
	ptr[3] = ptr[3] ^ ptr[2];
	return (int)(*(int*)ptr);
}

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