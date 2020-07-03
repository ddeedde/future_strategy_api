#include "TcpFutureCFFEX.h"
#include <algorithm>
#include "boost/bind.hpp"
#include "LogWrapper.h"
#include "CommonDefine.h"
#include "Utility.h"
#include "FutureApi.h"


SpiderTcpFutureCFFEXSpi::SpiderTcpFutureCFFEXSpi()
	: m_pUserApi(NULL)	
	, smd(NULL)
	, recv_count(0)
{

}

SpiderTcpFutureCFFEXSpi::~SpiderTcpFutureCFFEXSpi()
{
	stop();
}

bool SpiderTcpFutureCFFEXSpi::init(SpiderTcpFutureCFFEXSession * sm)
{
	smd = sm;
	myAccount = sm->get_account();

	if (strlen(sm->get_account().uri_list[0]) <= 0)
	{
		LOGE("�����ļ��������ַ����Ϊ��");
		return false;
	}
	std::vector<std::string> ips;
	split_str(sm->get_account().uri_list[0], ips, ":");
	if (ips.size() != 3)
	{
		LOGE("�����ļ��������ַ��ʽ����tcp://127.0.0.1:17001");
		return false;
	}

	m_pUserApi = CUstpFtdcMduserApi::CreateFtdcMduserApi();
	if (m_pUserApi == NULL)
	{
		LOGE("�н�tcp�����ʼ��ʧ��");
		return false;
	}
	int _MajorVersion = 0, _MinorVersion = 0;
	LOGI("�н�tcp�����ʼ���ɹ����汾�ţ�"<< CUstpFtdcMduserApi::GetVersion(_MajorVersion, _MinorVersion));
	m_pUserApi->RegisterSpi(this);

	return true;
}

void SpiderTcpFutureCFFEXSpi::start()
{
	m_pUserApi->SubscribeMarketDataTopic(110,USTP_TERT_QUICK); //110����ţ����ƶ��������������£�110���������������ת������
	m_pUserApi->RegisterFront(smd->get_account().uri_list[0]);
	m_pUserApi->Init();
}

void SpiderTcpFutureCFFEXSpi::stop()
{
	if (m_pUserApi)
	{
		m_pUserApi->Release();
		m_pUserApi->Join();
		m_pUserApi = NULL;
	}
}

void SpiderTcpFutureCFFEXSpi::login()
{
	CUstpFtdcReqUserLoginField reqUserLogin;
	memset(&reqUserLogin, 0, sizeof(reqUserLogin));
	strncpy(reqUserLogin.BrokerID, myAccount.broker_id, sizeof(reqUserLogin.BrokerID) - 1);
	strncpy(reqUserLogin.UserID, myAccount.account_id, sizeof(reqUserLogin.UserID) - 1);
	strncpy(reqUserLogin.Password, myAccount.password, sizeof(reqUserLogin.Password) - 1);
	strncpy(reqUserLogin.UserProductInfo, myAccount.product_id, sizeof(reqUserLogin.UserProductInfo) - 1);
	LOGI("CFFEXSpi ReqUserLogin.UserProductInfo is: " << reqUserLogin.UserProductInfo << ", " << reqUserLogin.BrokerID << ", " << reqUserLogin.UserID);


	int r = m_pUserApi->ReqUserLogin(&reqUserLogin, get_seq());
	if (r != 0)
	{
		LOGW("Warning: CFFEXSpi encountered an error when try to login, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "ReqUserLogin����ʧ��", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

// ���ͻ��������鷢��������������ͨ�����ӣ��ͻ�����Ҫ���е�¼
void SpiderTcpFutureCFFEXSpi::OnFrontConnected()
{
	LOGI(myAccount.account_id << ", CFFEXSpi:���ӳɹ�");
	smd->on_connected();
	login();
}

///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
///@param nReason ����ԭ��
///        0x1001 �����ʧ��
///        0x1002 ����дʧ��
///        0x2001 ����������ʱ
///        0x2002 ��������ʧ��
///        0x2003 �յ�������
void SpiderTcpFutureCFFEXSpi::OnFrontDisconnected(int nReason)
{
	LOGE(myAccount.account_id << ", CFFEXSpi:�����ж�, code:" <<nReason);
	smd->on_disconnected();
}

///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
///@param nTimeLapse �����ϴν��ձ��ĵ�ʱ��
void SpiderTcpFutureCFFEXSpi::OnHeartBeatWarning(int nTimeLapse)
{
	LOGW(myAccount.account_id << ", CFFEXSpi:�յ�������ʱ����, no msg for:" << nTimeLapse);
}

///����Ӧ��
void SpiderTcpFutureCFFEXSpi::OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		LOGE(myAccount.account_id << ", CFFEXSpi:�յ�������Ϣ��" << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = pRspInfo->ErrorID;
		strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

// ���ͻ��˷�����¼����֮�󣬸÷����ᱻ���ã�֪ͨ�ͻ��˵�¼�Ƿ�ɹ�
void SpiderTcpFutureCFFEXSpi::OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CFFEXSpi:OnRspUserLogin: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pRspUserLogin != NULL)
	{
		LOGI(myAccount.account_id << ", CFFEXSpi:��½�ɹ���������" << pRspUserLogin->TradingDay << "��" << pRspUserLogin->BrokerID << "��" << pRspUserLogin->UserID);
		LOGI("��¼ʱ�䣺" << pRspUserLogin->LoginTime << ", " << pRspUserLogin->TradingSystemName << ", " << pRspUserLogin->DataCenterID << ", " << pRspUserLogin->PrivateFlowSize << ", " << pRspUserLogin->UserFlowSize);
		if (smd)
		{
			smd->on_log_in();
			//���¶���һ������
			smd->subscribe(smd->get_subscribed_list());
		}
	}
}

///�û��˳�Ӧ��
void SpiderTcpFutureCFFEXSpi::OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CFFEXSpi:OnRspUserLogout: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pRspUserLogout != NULL)
	{
		LOGI(myAccount.account_id << ", CFFEXSpi:�ǳ��ɹ�: " << pRspUserLogout->BrokerID << ", " << pRspUserLogout->UserID);
		if (smd)
		{
			smd->on_log_out();
		}
	}
}

///��������Ӧ��
void SpiderTcpFutureCFFEXSpi::OnRspSubscribeTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo)
	{
		if (pRspInfo->ErrorID == 0)
		{
			LOGI(myAccount.account_id << ", CFFEXSpi:���ⶩ�ĳɹ�");
			if (pDissemination)
			{
				LOGI("CFFEXSpi:���ⶩ�ĳɹ�,"<< pDissemination->SequenceSeries<<":"<< pDissemination->SequenceNo);
			}
		}
		else {
			LOGE(myAccount.account_id << ", CFFEXSpi:���ⶩ��ʧ�ܣ�"<< pRspInfo->ErrorID << ", "<< pRspInfo->ErrorMsg);
		}
	}
	else {
		if (pDissemination)
		{
			LOGW("CFFEXSpi:���ⶩ��δ֪," << pDissemination->SequenceSeries << ":" << pDissemination->SequenceNo);
		}
	}
}

// �������֪ͨ�����������������֪ͨ�ͻ���
void SpiderTcpFutureCFFEXSpi::OnRtnDepthMarketData(CUstpFtdcDepthMarketDataField *pMarketData)
{
	if (pMarketData != NULL)
	{
		if (++recv_count % 5000 == 0)
		{
			LOGI("FutureCFFEXSpi received: " << recv_count);
		}
		QuotaData * md = new QuotaData();
		md->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		md->UpdateMillisec = pMarketData->UpdateMillisec;
		memcpy(md->TradingDay, pMarketData->TradingDay, sizeof(md->TradingDay) - 1);
		memcpy(md->Code, pMarketData->InstrumentID, sizeof(md->Code) - 1);
		memcpy(md->UpdateTime, pMarketData->UpdateTime, sizeof(md->UpdateTime) - 1);
		md->AskPrice1 = (pMarketData->AskPrice1>=DBL_MAX) ? 0 : pMarketData->AskPrice1;
		md->AskVolume1 = (pMarketData->AskVolume1 >= DBL_MAX) ? 0 : pMarketData->AskVolume1;
		md->BidPrice1 = (pMarketData->BidPrice1 >= DBL_MAX) ? 0 : pMarketData->BidPrice1;
		md->BidVolume1 = (pMarketData->BidVolume1 >= DBL_MAX) ? 0 : pMarketData->BidVolume1;
		md->LastPrice = (pMarketData->LastPrice >= DBL_MAX) ? 0 : pMarketData->LastPrice;
		md->HighestPrice = (pMarketData->HighestPrice >= DBL_MAX) ? 0 : pMarketData->HighestPrice;
		md->LowestPrice = (pMarketData->LowestPrice >= DBL_MAX) ? 0 : pMarketData->LowestPrice;
		md->LowerLimitPrice = (pMarketData->LowerLimitPrice >= DBL_MAX) ? 0 : pMarketData->LowerLimitPrice;
		md->UpperLimitPrice = (pMarketData->UpperLimitPrice >= DBL_MAX) ? 0 : pMarketData->UpperLimitPrice;
		md->OpenPrice = (pMarketData->OpenPrice >= DBL_MAX) ? 0 : pMarketData->OpenPrice;
		md->PreClosePrice = (pMarketData->PreClosePrice >= DBL_MAX) ? 0 : pMarketData->PreClosePrice;
		md->ClosePrice = (pMarketData->ClosePrice >= DBL_MAX) ? 0 : pMarketData->ClosePrice;
		md->PreSettlementPrice = (pMarketData->PreSettlementPrice >= DBL_MAX) ? 0 : pMarketData->PreSettlementPrice;
		md->SettlementPrice = (pMarketData->SettlementPrice >= DBL_MAX) ? 0 : pMarketData->SettlementPrice;
		md->PreOpenInterest = (pMarketData->PreOpenInterest >= DBL_MAX) ? 0 : pMarketData->PreOpenInterest;
		md->OpenInterest = (pMarketData->OpenInterest >= DBL_MAX) ? 0 : pMarketData->OpenInterest;
		md->Turnover = (pMarketData->Turnover >= DBL_MAX) ? 0 : pMarketData->Turnover;
		md->Volume = pMarketData->Volume;

		LOGD("tcp cffex future:" << md->Code << "," << md->LastPrice << "," << md->PreClosePrice << "," << md->PreSettlementPrice << "," << md->AskPrice1 << "," << md->AskVolume1 << "," << md->BidVolume1 << "," << md->BidPrice1 << "," << md->UpperLimitPrice << "," << md->Volume << "," << md->Turnover << "," << md->UpdateTime << "," << md->UpdateMillisec); //just for test

		if (smd)
			smd->on_receive_data(md);
	}
}

///���ĺ�Լ�������Ϣ
void SpiderTcpFutureCFFEXSpi::OnRspSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGI(myAccount.account_id << ", CFFEXSpi:OnRspSubMarketData: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pSpecificInstrument != NULL)
	{
		LOGI(myAccount.account_id << ", CFFEXSpi:���鶩�ĳɹ���" << pSpecificInstrument->InstrumentID);
		smd->updateSubscribedList(pSpecificInstrument->InstrumentID);
	}
}

///�˶���Լ�������Ϣ
void SpiderTcpFutureCFFEXSpi::OnRspUnSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGI(myAccount.account_id << ", CFFEXSpi:OnRspUnSubMarketData: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pSpecificInstrument != NULL)
	{
		LOGI(myAccount.account_id << ", CFFEXSpi:�����˶��ɹ���" << pSpecificInstrument->InstrumentID);
		smd->updateSubscribedList(pSpecificInstrument->InstrumentID, true);
	}
}

//*********************************************************

SpiderTcpFutureCFFEXSession::SpiderTcpFutureCFFEXSession(SpiderCommonApi * sci, AccountInfo & ai)
	:BaseMarketSession(sci, ai)
{
	marketConnection.reset(new SpiderTcpFutureCFFEXSpi());
}

SpiderTcpFutureCFFEXSession::~SpiderTcpFutureCFFEXSession()
{

}

bool SpiderTcpFutureCFFEXSession::init()
{
	return marketConnection->init(this);
}

void SpiderTcpFutureCFFEXSession::start()
{
	marketConnection->start();
}

void SpiderTcpFutureCFFEXSession::stop()
{
	marketConnection->stop();
}

void SpiderTcpFutureCFFEXSession::subscribe(std::vector<std::string> & list)
{
	if (!list.empty())
	{
		std::vector<char *> contracts;
		for (unsigned int i = 0; i < list.size(); ++i)
		{
			contracts.push_back(const_cast<char *>(list[i].c_str()));
		}
		int result = marketConnection->getUserApi()->SubMarketData(&contracts[0], contracts.size());
		LOGI("subscribed to " << contracts.size() << ", result: " << result << ", " << contracts[0]);
		if (result != 0)
		{
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = result;
			strncpy(_err->ErrMsg, "SubscribeMarketData����ʧ��", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
		}
	}
}

void SpiderTcpFutureCFFEXSession::unsubscribe(std::vector<std::string> & list)
{
	if (!list.empty())
	{
		std::vector<char *> contracts;
		for (unsigned int i = 0; i < list.size(); ++i)
		{
			contracts.push_back(const_cast<char *>(list[i].c_str()));
		}
		int result = marketConnection->getUserApi()->UnSubMarketData(&contracts[0], contracts.size());
		LOGI("unsubscribed to " << contracts.size() << ", result: " << result);
		if (result != 0)
		{
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = result;
			strncpy(_err->ErrMsg, "UnSubscribeMarketData����ʧ��", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
		}
	}
}

void SpiderTcpFutureCFFEXSession::on_connected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnConnect();
		}
	}
}

void SpiderTcpFutureCFFEXSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnDisconnect();
		}
	}
}

void SpiderTcpFutureCFFEXSession::on_log_in()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderTcpFutureCFFEXSession::on_log_out()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogout();
		}
	}
}

void SpiderTcpFutureCFFEXSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
{
	if (err_msg.get())
	{
		if (spider_common_api != NULL)
		{
			if (spider_common_api->notifySpi != NULL)
			{
				spider_common_api->notifySpi->onError(err_msg.get());
			}
		}
	}
}

void SpiderTcpFutureCFFEXSession::on_receive_data(QuotaData * md)
{
	if (spider_common_api != NULL)
	{
		spider_common_api->onQuote(md);
	}
}

