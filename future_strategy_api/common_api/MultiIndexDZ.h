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

//��֤�ڻ���������ȡָ�����飬ͨ���鲥�ķ�ʽ
namespace Index_DZ
{
#pragma pack(1)
	// ��ʳɽ�
	struct TickData
	{
		char    SecurityExchange[8];  // ���������Ϻ�:"SHL2"������:"SZL2"
		char    SecurityID[8];        // ֤ȯ����
		char    LastUpdateTime[32];   // ISO8601��ʽ�������ʱ�� ���� 2019-11-26T13:25:26+08:00(�Ϻ�)��2019-11-25T14:01:20.13+08:00(����)
		double  Price;                // �ɽ��۸�
		int64_t Volume;               // �ɽ���
		char    Channel[12];          // �ɽ�ͨ��
		char    TradeNo[12];          // �ɽ����
		double  Amount;               // �ɽ����
		char    BuyNo[12];            // ί����
		char    SellNo[12];           // ί�����
		char    BSFlag;               // �ɽ����� T�ɽ� B����ɽ� S�����ɽ� C����
	};

	// ���ί��(��������)
	struct Order
	{
		char    SecurityExchange[8];   // ���������Ϻ�:"SHL2"������:"SZL2"
		char    SecurityID[8];         // ֤ȯ����
		char    UpdateTime[32];        // ʱ���
		double  Price;                 // ί�м�
		int64_t Volume;                // ί����
		char    Channel[12];           // ί��ͨ��
		char    OrderNo[12];           // ί�б��
		char    BSFlag;                // ί�з��� T��ȷ�� B�� S�� G���� F���
		char    OrderType;             // ί������ 1:�м�ί�� 2:�޼�ί�� U:��������
	};

	// ��λ���ݽṹ��
	struct EntrySnap
	{
		double  MDEntryPx;             // ��/���۸�
		int64_t MDEntrySize;           // ��/������
		int32_t MDEntryPositionNo;     // ��λ���(��0��ʼ)
	};

	// ��̬���ݽṹ��
	struct StaticInfoEntry
	{
		char    Symbol[16];           // ֤ȯ����(UTF-8)
		double  OpenPx;               // ���̼�
		double  ClosePx;              // �����̼�
		double  HighLimitPx;          // ��ͣ��
		double  LowLimitPx;           // ��ͣ��
		double  PrevSetPx;            // ������(����Ȩ)
		double  PrevClosePx;          // �����̼�
		char    LastUpdateTime[36];   // ISO8601��ʽ��̬���ݸ���ʱ��
	};


	// ����״̬FixDTradingStatus�ֶ�˵����
	// D Ĭ�ϡ�����״̬ *Ӧ��Ϊ����������
	// S ����ǰ
	// C ���̼��Ͼ��ۡ���ǰ����
	// T �������ס��������
	// F �̺�̶��۸���
	// B ����
	// P ͣ��
	// H ��ʱͣ�ơ���ʱͣ�С��������жϣ���Ȩ��
	// M �ɻָ��۶ϡ����ɻָ��۶�
	// I ���м��Ͼ���
	// U ���̼��Ͼ��ۡ��̺���
	// A ���̺�
	// E ����

	// ���۽���״̬TradingPhaseCode�ֶ�˵��
	// �Ϻ�
	// START ����
	// OCALL ���м��Ͼ���
	// TRADE �����Զ����
	// SUSP ͣ��
	// CCALL ���̼��Ͼ���
	// CLOSE ���У��Զ�������м۸�
	// ENDTR ���׽���
	// VOLA �������׺ͼ��Ͼ��۽��׵Ĳ������ж�
	// -------------------------------------
	// ����
	// ��0λ
	// S ����(��ʼǰ)
	// O ���̼��Ͼ���
	// T ��������
	// B ����
	// C ���̼��Ͼ���
	// E �ѱ���
	// H ��ʱͣ��
	// A �̺���
	// V �������ж�
	// ��1λ
	// 0 ����״̬
	// 1 ȫ��ͣ��
	// ����TradingPhaseCode�ֶ�ʣ��λ�ÿո�(ASCII��0x20)��������󲻺��ַ���������'\0'

	struct Snapshot
	{
		char            SecurityExchange[8];   // ���������Ϻ�:"SHL2"������:"SZL2"
		char            SecurityID[8];         // ֤ȯ����
		char            FixDTradingStatus[8];  // ����״̬ �ο��Ϸ�FixDTradingStatus�ֶ�˵��
		char            LastUpdateTime[32];    // ISO8601��ʽ�������ʱ�� ���� "2019-11-26T13:25:26+08:00"(�Ϻ�)��"2019-11-25T14:01:20.13+08:00"(����)
		double          LastPx;                // ���¼�
		double          HighPx;                // ��߼�
		double          LowPx;                 // ��ͼ�
		char            TradingPhaseCode[8];   // ���۽���״̬ �ο��Ϸ�TradingPhaseCode�ֶ�˵��
		int64_t         NumTrades;             // ���ױ���
		int64_t         TotalVolumeTraded;     // �ɽ���
		double          TotalValueTraded;      // �ܳɽ���
		double          IOPV;                  // �ο���ֵ(������)
		int64_t         TotalLongPosition;     // �ֲܳ���(����Ȩ)	
		uint32_t        BuySeqDepth;           // ��λ����
		EntrySnap       MDEntryBuyer[10];      // ����������
		uint32_t        SellSeqDepth;          // ����λ����
		EntrySnap       MDEntrySeller[10];     // ����������
		int32_t         BuyNumOrders;		   // ��ʵ��ί�б���
		int32_t 	    BuyNoOrders[50];       // 50����ί��
		int32_t         SellNumOrders;		   // ��ʵ��ί�б���
		int32_t 	    SellNoOrders[50];      // 50����ί��
		int64_t         TotalBidQty;           // ί��������
		double          WeightedAvgBidPx;      // ���Ȩ����
		int64_t         TotalOfferQty;         // ί��������
		double          WeightedAvgOfferPx;    // ����Ȩ����
		StaticInfoEntry StaticInfo;            // ��̬����
	};

	union DataField
	{
		Snapshot snapshot;      // �������
		Order order;            // ���ί��(��������)
		TickData tick;          // ��ʳɽ�
	};

	// ����ṹ��
	// MDStreamID�ֶ�˵����
	// MDStreamIDΪ'E'��Ʊ 'O'��Ȩ 'W'Ȩ֤ 'B'ծȯ 'I'ָ�� 'F' ���� ��ӦdataΪ�������Snapshot�ṹ��
	// MDStreamIDΪ"TI"��ӦdataΪ��ʳɽ�TickData�ṹ��
	// MDStreamIDΪ"SO"��ӦdataΪ���ί��Order�ṹ��

	struct MarketDataField
	{
		char MDStreamID[8];         // ���ͣ�'E' 'O' "TI" "SO"�ȣ��ο��Ϸ�MDStreamID�ֶ�˵��
		union DataField data;       // ����
	};
#pragma pack()

}


class SpiderMultiIndexDZSession;
class SpiderMultiIndexDZSpi
{
	//���ն������鲥
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