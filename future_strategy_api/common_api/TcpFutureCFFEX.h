#if !defined(SPIDER_TCPFUTURECFFEX_H)
#define SPIDER_TCPFUTURECFFEX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MdApi.h"
#include "SpiderApiStruct.h"
#include "USTPFtdcMduserApi.h"

/*
2020-04-02
�����н�Lv2�鲥�����api����ȡ���飬�����Ǿ���ķ�ʽ;
�н��udp��apiĿǰֻ��linux�汾������tcp����windows�汾
*/



class SpiderTcpFutureCFFEXSession;
class SpiderTcpFutureCFFEXSpi : public CUstpFtdcMduserSpi
{
	// ָ��CUstpFtdcMduserApiʵ����ָ��
	CUstpFtdcMduserApi *m_pUserApi;

	SpiderTcpFutureCFFEXSession * smd;

	AccountInfo myAccount;

	int recv_count;
public:
	SpiderTcpFutureCFFEXSpi();
	~SpiderTcpFutureCFFEXSpi();

	bool init(SpiderTcpFutureCFFEXSession * sm);
	void start();
	void stop();
	void login();
	CUstpFtdcMduserApi * getUserApi() { return m_pUserApi; }

public:
	// ���ͻ��������鷢��������������ͨ�����ӣ��ͻ�����Ҫ���е�¼
	virtual void OnFrontConnected();

	///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
	///@param nReason ����ԭ��
	///        0x1001 �����ʧ��
	///        0x1002 ����дʧ��
	///        0x2001 ����������ʱ
	///        0x2002 ��������ʧ��
	///        0x2003 �յ�������
	virtual void OnFrontDisconnected(int nReason);

	///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
	///@param nTimeLapse �����ϴν��ձ��ĵ�ʱ��
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///����Ӧ��
	virtual void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	// ���ͻ��˷�����¼����֮�󣬸÷����ᱻ���ã�֪ͨ�ͻ��˵�¼�Ƿ�ɹ�
	virtual void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///�û��˳�Ӧ��
	virtual void OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///��������Ӧ��
	virtual void OnRspSubscribeTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	// �������֪ͨ�����������������֪ͨ�ͻ���
	virtual void OnRtnDepthMarketData(CUstpFtdcDepthMarketDataField *pMarketData);

	///���ĺ�Լ�������Ϣ
	virtual void OnRspSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///�˶���Լ�������Ϣ
	virtual void OnRspUnSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);


};




class SpiderTcpFutureCFFEXSession : public BaseMarketSession
{
public:
	SpiderTcpFutureCFFEXSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderTcpFutureCFFEXSession();

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
	std::shared_ptr<SpiderTcpFutureCFFEXSpi> marketConnection;
};












#endif