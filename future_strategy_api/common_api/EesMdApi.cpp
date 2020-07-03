#include "EesMdApi.h"
#include "LogWrapper.h"
#include "FutureApi.h"
#include "CommonDefine.h"


SpiderEesMdSpi::SpiderEesMdSpi()
	: userApi(NULL)
	, smd(NULL)
	, recv_count(0)
{

}

SpiderEesMdSpi::~SpiderEesMdSpi()
{

}

bool SpiderEesMdSpi::init(SpiderEesMdSession * sm)
{
	smd = sm;

	myAccount = sm->get_account();

	for (unsigned int i = 0; i < URILIST_MAX_SIZE; ++i)
	{
		if (strlen(myAccount.uri_list[i]) > 0)
		{
			LOGI("Info: Ees TCP Market data receiver: " << myAccount.uri_list[i]);
			std::vector<std::string> ips;
			split_str(myAccount.uri_list[i], ips, ":");

			if (ips.size() == 3) //tcp://127.0.0.1:31001
			{
				replace_all(ips[1], "/", "");
				EqsTcpInfo tmp;
				memset(&tmp,0,sizeof(tmp));
				strncpy(tmp.m_eqsIp, ips[1].c_str(), sizeof(tmp.m_eqsIp) - 1);
				tmp.m_eqsPort = (unsigned short)atoi(ips[2].c_str());
				tcp_list.push_back(tmp);
			}
			else if (ips.size() == 6) //mul://230.0.0.1:31001:0.0.0.0:65000:EnumExchangeIDType
			{
				replace_all(ips[1], "/", "");
				EqsMulticastInfo tmp;
				strncpy(tmp.m_mcIp, ips[1].c_str(), sizeof(tmp.m_mcIp) - 1);
				tmp.m_mcPort = (unsigned short)atoi(ips[2].c_str());
				strncpy(tmp.m_mcLoacalIp, ips[3].c_str(), sizeof(tmp.m_mcLoacalIp) - 1);
				tmp.m_mcLocalPort = (unsigned short)atoi(ips[4].c_str());

				multi_list.push_back(tmp);
			}
			else {
				LOGE("Ees �����ļ��������ַ��ʽ����");
				return false;
			}
		}
	}
	if (multi_list.size() <= 0 && tcp_list.size() <= 0)
	{
		LOGE("Ees �����ļ��������ַΪ��");
		return false;
	}

	userApi = CreateEESQuoteApi();
	if (userApi == NULL)
	{
		LOGE("����EES EESQuoteApi ʧ��");
		return false;
	}

	return true;
}

void SpiderEesMdSpi::start()
{
	//����ʹ���鲥����
	if (multi_list.size() > 0)
	{
		if (!userApi->InitMulticast(multi_list, this))
		{
			LOGE("EES �鲥��������ʧ��");
		}
		else {
			if (smd)
			{
				smd->on_connected();
				smd->on_log_in();
			}
		}
	}
	else if (tcp_list.size() > 0)
	{
		if (!userApi->ConnServer(tcp_list, this))
		{
			LOGE("EES ����TCP���������ʧ��");
		}
	}

}

void SpiderEesMdSpi::stop()
{
	if (userApi != NULL)
	{
		userApi->DisConnServer();
		DestroyEESQuoteApi(userApi);
		userApi = NULL;
	}
}

void SpiderEesMdSpi::login()
{
	//�鲥ģʽ����Ҫ��¼
	EqsLoginParam reqUserLogin;
	strncpy(reqUserLogin.m_loginId, myAccount.account_id, sizeof(reqUserLogin.m_loginId) - 1);
	strncpy(reqUserLogin.m_password, myAccount.password, sizeof(reqUserLogin.m_password) - 1);

	LOGI("EESMarketDataSession ReqUserLoginis:" << reqUserLogin.m_loginId);

	userApi->LoginToEqs(reqUserLogin);
}

/// \brief �����������ӳɹ�����¼ǰ����, ������鲥ģʽ���ᷢ��, ֻ���ж�InitMulticast����ֵ����
void SpiderEesMdSpi::OnEqsConnected()
{
	LOGI(myAccount.account_id << ", EES:���ӳɹ�");
	smd->on_connected();
	login();
}

/// \brief ���������������ӳɹ������Ͽ�ʱ���ã��鲥ģʽ���ᷢ�����¼�
void SpiderEesMdSpi::OnEqsDisconnected()
{
	LOGE(myAccount.account_id << ", EES:�����ж�");
	smd->on_disconnected();
}

/// \brief ����¼�ɹ�����ʧ��ʱ���ã��鲥ģʽ���ᷢ��
/// \param bSuccess ��½�Ƿ�ɹ���־  
/// \param pReason  ��½ʧ��ԭ��  
void SpiderEesMdSpi::OnLoginResponse(bool bSuccess, const char* pReason)
{
	if (bSuccess)
	{
		LOGI(myAccount.account_id << ", EES:��½�ɹ�");
		if (smd)
		{
			smd->on_log_in();
			//���¶���һ������
			smd->subscribe(smd->get_subscribed_list());
		}
		
	}
	else {
		LOGW(myAccount.account_id << ", EES:��¼ʧ��: " << pReason );
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, pReason, sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

/// \brief ע��symbol��Ӧ��Ϣ��ʱ���ã��鲥ģʽ��֧������ע��
/// \param chInstrumentType  EES��������
/// \param pSymbol           ��Լ����
/// \param bSuccess          ע���Ƿ�ɹ���־
void SpiderEesMdSpi::OnSymbolRegisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess)
{
	if (bSuccess)
	{
		smd->updateSubscribedList(pSymbol);
	}
	else{
		LOGI(myAccount.account_id << ", EES:����ʧ��" << chInstrumentType << ":" << pSymbol);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "����ʧ��", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

/// \brief  ע��symbol��Ӧ��Ϣ��ʱ���ã��鲥ģʽ��֧������ע��
/// \param chInstrumentType  EES��������
/// \param pSymbol           ��Լ����
/// \param bSuccess          ע���Ƿ�ɹ���־
void SpiderEesMdSpi::OnSymbolUnregisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess)
{
	if (bSuccess)
	{
		smd->updateSubscribedList(pSymbol,true);
	}
	else {
		LOGI(myAccount.account_id << ", EES:�˶�ʧ��" << chInstrumentType << ":" << pSymbol);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "�˶�ʧ��", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

/// \brief �յ�����ʱ����,�����ʽ����instrument_type��ͬ����ͬ
/// \param chInstrumentType  EES��������
/// \param pDepthQuoteData   EESͳһ����ָ��  
void SpiderEesMdSpi::OnQuoteUpdated(EesEqsIntrumentType chInstrumentType, EESMarketDepthQuoteData* pDepthQuoteData)
{
	if (pDepthQuoteData != NULL)
	{
		if (recv_count++ % 5000 == 0)
		{
			LOGI("SpiderEESMdSpi received: " << recv_count<<", ex:"<< pDepthQuoteData->ExchangeID);
		}
		QuotaData * md = new QuotaData();
		md->ExchangeID = get_exid_from_ctp(pDepthQuoteData->ExchangeID);
		md->UpdateMillisec = pDepthQuoteData->UpdateMillisec;
		memcpy(md->TradingDay, pDepthQuoteData->TradingDay, sizeof(md->TradingDay) - 1);
		memcpy(md->Code, pDepthQuoteData->InstrumentID, sizeof(md->Code) - 1);
		memcpy(md->UpdateTime, pDepthQuoteData->UpdateTime, sizeof(md->UpdateTime) - 1);
		md->AskPrice1 = pDepthQuoteData->AskPrice1;
		md->AskVolume1 = pDepthQuoteData->AskVolume1;
		md->BidPrice1 = pDepthQuoteData->BidPrice1;
		md->BidVolume1 = pDepthQuoteData->BidVolume1;
		md->LastPrice = pDepthQuoteData->LastPrice;
		md->HighestPrice = pDepthQuoteData->HighestPrice;
		md->LowestPrice = pDepthQuoteData->LowestPrice;
		md->LowerLimitPrice = pDepthQuoteData->LowerLimitPrice;
		md->UpperLimitPrice = pDepthQuoteData->UpperLimitPrice;
		md->OpenPrice = pDepthQuoteData->OpenPrice;
		md->PreClosePrice = pDepthQuoteData->PreClosePrice;
		md->ClosePrice = pDepthQuoteData->ClosePrice;
		md->PreSettlementPrice = pDepthQuoteData->PreSettlementPrice;
		md->SettlementPrice = pDepthQuoteData->SettlementPrice;
		md->PreOpenInterest = pDepthQuoteData->PreOpenInterest;
		md->OpenInterest = pDepthQuoteData->OpenInterest;
		md->Turnover = pDepthQuoteData->Turnover;
		md->Volume = pDepthQuoteData->Volume;

		if (smd)
			smd->on_receive_data(md);
		LOGD("ees future api:" << md->Code << "," << md->LastPrice << "," << md->Volume << "," << pDepthQuoteData->UpdateTime << "-" << pDepthQuoteData->UpdateMillisec); //just for test
	}
}

/// \brief ��־�ӿڣ���ʹ���߰���д��־��
/// \param nlevel    ��־����
/// \param pLogText  ��־����
/// \param nLogLen   ��־����
void SpiderEesMdSpi::OnWriteTextLog(EesEqsLogLevel nlevel, const char* pLogText, int nLogLen)
{
	switch (nlevel)
	{
	case QUOTE_LOG_LV_DEBUG:
		LOGD("EES Quote: "<<pLogText);
		break;
	case QUOTE_LOG_LV_INFO:
		LOGI("EES Quote: " << pLogText);
		break;
	case QUOTE_LOG_LV_WARN:
		LOGW("EES Quote: " << pLogText);
		break;
	case QUOTE_LOG_LV_ERROR:		
	case QUOTE_LOG_LV_FATAL:
	case QUOTE_LOG_LV_USER:
	case QUOTE_LOG_LV_END:
		LOGE("EES Quote: " << pLogText);
		break;
	default:
		break;
	}
}

//*********************************************************

SpiderEesMdSession::SpiderEesMdSession(SpiderCommonApi * sci, AccountInfo & ai)
	:BaseMarketSession(sci, ai)
{
	marketConnection.reset(new SpiderEesMdSpi());
}

SpiderEesMdSession::~SpiderEesMdSession()
{

}

bool SpiderEesMdSession::init()
{
	return marketConnection->init(this);
}

void SpiderEesMdSession::start()
{
	marketConnection->start();
}

void SpiderEesMdSession::stop()
{
	marketConnection->stop();
}

void SpiderEesMdSession::subscribe(std::vector<std::string> & list)
{
	for (size_t i = 0; i < list.size(); i++)
	{
		if (marketConnection->if_multicast_mode())
		{
			updateSubscribedList(list[i].c_str());
		}
		else {
			marketConnection->getUserApi()->RegisterSymbol(EesEqsIntrumentType::EQS_FUTURE, list[i].c_str()); //tcp �������Ҫ
		}
		LOGI("EES: �������飺"<< EesEqsIntrumentType::EQS_FUTURE<<":"<< list[i]);
	}
}

void SpiderEesMdSession::unsubscribe(std::vector<std::string> & list)
{
	for (size_t i = 0; i < list.size(); i++)
	{
		if (marketConnection->if_multicast_mode())
		{
			updateSubscribedList(list[i].c_str(),true);
		}
		else {
			marketConnection->getUserApi()->UnregisterSymbol(EesEqsIntrumentType::EQS_FUTURE, list[i].c_str()); //tcp �������Ҫ
		}	
		LOGI("EES: �˶����飺" << EesEqsIntrumentType::EQS_FUTURE << ":" << list[i]);
	}
}

void SpiderEesMdSession::on_connected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnConnect();
		}
	}
}

void SpiderEesMdSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnDisconnect();
		}
	}
}

void SpiderEesMdSession::on_log_in()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderEesMdSession::on_log_out()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogout();
		}
	}
}

void SpiderEesMdSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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

void SpiderEesMdSession::on_receive_data(QuotaData * md)
{
	if (marketConnection->if_multicast_mode() && 
		!ifSubscribed(md->Code)) //�鲥ģʽ�£�ֻ���Ͷ����˵ĺ�Լ������ 
	{
		return;
	}
	if (spider_common_api != NULL)
	{
		spider_common_api->onQuote(md);
	}
}

