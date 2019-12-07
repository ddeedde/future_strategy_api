#if !defined(SPIDER_EESMDAPI_H)
#define SPIDER_EESMDAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*
ʢ������ӿ�
*/

#include "MdApi.h"
#include "SpiderApiStruct.h"
#include "EESQuoteApi.h"


class SpiderEesMdSession;
class SpiderEesMdSpi : public EESQuoteEvent
{
	EESQuoteApi * userApi;

	SpiderEesMdSession * smd;

	AccountInfo myAccount;

	int recv_count;

	std::vector<EqsTcpInfo> tcp_list; //������ͬһʱ��ֻ����һ��������
	std::vector<EqsMulticastInfo> multi_list; //������ͬһʱ��ֻ����һ��������
public:
	EESQuoteApi * getUserApi() { return userApi; }
	bool init(SpiderEesMdSession * sm);
	void start();
	void stop();
	void login();
	bool if_multicast_mode() { return multi_list.size() > 0; }
public:
	SpiderEesMdSpi();
	~SpiderEesMdSpi();

	/// \brief �����������ӳɹ�����¼ǰ����, ������鲥ģʽ���ᷢ��, ֻ���ж�InitMulticast����ֵ����
	virtual void OnEqsConnected();

	/// \brief ���������������ӳɹ������Ͽ�ʱ���ã��鲥ģʽ���ᷢ�����¼�
	virtual void OnEqsDisconnected();

	/// \brief ����¼�ɹ�����ʧ��ʱ���ã��鲥ģʽ���ᷢ��
	/// \param bSuccess ��½�Ƿ�ɹ���־  
	/// \param pReason  ��½ʧ��ԭ��  
	virtual void OnLoginResponse(bool bSuccess, const char* pReason);

	/// \brief �յ�����ʱ����,�����ʽ����instrument_type��ͬ����ͬ
	/// \param chInstrumentType  EES��������
	/// \param pDepthQuoteData   EESͳһ����ָ��  
	virtual void OnQuoteUpdated(EesEqsIntrumentType chInstrumentType, EESMarketDepthQuoteData* pDepthQuoteData);

	/// \brief ��־�ӿڣ���ʹ���߰���д��־��
	/// \param nlevel    ��־����
	/// \param pLogText  ��־����
	/// \param nLogLen   ��־����
	virtual void OnWriteTextLog(EesEqsLogLevel nlevel, const char* pLogText, int nLogLen);

	/// \brief ע��symbol��Ӧ��Ϣ��ʱ���ã��鲥ģʽ��֧������ע��
	/// \param chInstrumentType  EES��������
	/// \param pSymbol           ��Լ����
	/// \param bSuccess          ע���Ƿ�ɹ���־
	virtual void OnSymbolRegisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess);

	/// \brief  ע��symbol��Ӧ��Ϣ��ʱ���ã��鲥ģʽ��֧������ע��
	/// \param chInstrumentType  EES��������
	/// \param pSymbol           ��Լ����
	/// \param bSuccess          ע���Ƿ�ɹ���־
	virtual void OnSymbolUnregisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess);

	/// \brief ��ѯsymbol�б���Ӧ��Ϣ��ʱ���ã��鲥ģʽ��֧�ֺ�Լ�б��ѯ
	/// \param chInstrumentType  EES��������
	/// \param pSymbol           ��Լ����
	/// \param bLast             ���һ����ѯ��Լ�б���Ϣ�ı�ʶ
	/// \remark ��ѯ��Լ�б���Ӧ, last = trueʱ��������������Ч���ݡ�
	virtual void OnSymbolListResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bLast) {}
};

class SpiderEesMdSession : public BaseMarketSession
{
public:
	SpiderEesMdSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderEesMdSession();

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
	std::shared_ptr<SpiderEesMdSpi> marketConnection;

};


















#endif