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
	unsigned int Type;		//300��Ʊ
	unsigned int Seq;		//��������Դ�е�����seq��
	char ExID[8];			//����������
	char SecID[8];			//֤ȯ����
	char SecName[16];		//֤ȯ����		
	int	SourceID;				//������Դ		
	unsigned int Time;		//Int��ʽ��ʱ�� 10:01:02 000 = 100102000
	unsigned int PreClose;		//ǰ����
	unsigned int Open;			// ���տ��̼�
	unsigned int High;			// ������߼�
	unsigned int Low;			// ������ͼ�
	unsigned int Match;			// ���¼�
	unsigned int HighLimited;	// ��ͣ��۸�
	unsigned int LowLimited;	// ��ͣ��۸�
	unsigned int NumTrades;		// �ɽ�����
	long long Volume;			// �ɽ���
	long long Turnover;			// �ɽ����
	long long AdjVolume;		// ������ĳɽ���������һ���Ĳ�
	long long AdjTurnover;		// ������ĳɽ�������һ���Ĳ�
	unsigned int LocalTime;		// ����ʱ�䣬��ȷ�����룬Int��ʽ 10:01:02 000 = 100102000
	int	Reserved1;
	MarketDataIndex() { memset(this, 0, sizeof(MarketDataIndex)); }
};
typedef boost::shared_ptr<MarketDataIndex> MarketDataIndexPtr;

class SpiderMultiIndexSession;
class SpiderMultiIndexSpi
{
	//���ն������鲥
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