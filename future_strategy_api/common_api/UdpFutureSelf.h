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



//�ڻ����飬��200�ֽ�
struct MarketDataFuture
{
	unsigned int Type;			// ���ʹ��룬MARKET_DATA_TYPE_FUTURE �� MARKET_DATA_TYPE_FUTURE_SNAP
	unsigned int Seq;			// ���
	char ExID[8];				// ����������
	char SecID[24];				// ֤ȯ����
	unsigned int Time;			// ������ʱ�䣬��ȷ�����룬Int��ʽ 10:01:02 000 = 100102000
	unsigned int Volume;		// �ۼƳɽ���
	long long Turnover;			// �ۼƳɽ����, ��λ��Ԫ
	long long PreSettlement;	// �����ۣ���λ��0.0001Ԫ
	long long PreClose;			// �����̣���λ��0.0001Ԫ
	long long PreOpenInterest;	// ��ֲ���
	long long Match;			// ���¼ۣ���λ��0.0001Ԫ
	long long Open;				// ���̣���λ��0.0001Ԫ
	long long High;				// ��߼ۣ���λ��0.0001Ԫ
	long long Low;				// ��ͼۣ���λ��0.0001Ԫ
	long long HighLimited;		// ��ͣ��ۣ���λ��0.0001Ԫ
	long long LowLimited;		// ��ͣ��ۣ���λ��0.0001Ԫ
	long long Settlement;		// �����ۣ���λ��0.0001Ԫ
	long long Close;			// �����̣���λ��0.0001Ԫ
	long long OpenInterest;		// ��ֲ���
	long long BidPrice;			// ��ۣ���λ��0.0001Ԫ
	long long BidVol;			// ����
	long long AskPrice;			// ���ۣ���λ��0.0001Ԫ
	long long AskVol;			// ����
	long long LocalTime;		// ����ʱ�䣬��ȷ��΢�룬Int��ʽ��"10:01:02 000 000" = 100102000000
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
	//���ն������鲥
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