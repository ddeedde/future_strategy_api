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
				LOGE("Ees 配置文件中行情地址格式错误");
				return false;
			}
		}
	}
	if (multi_list.size() <= 0 && tcp_list.size() <= 0)
	{
		LOGE("Ees 配置文件中行情地址为空");
		return false;
	}

	userApi = CreateEESQuoteApi();
	if (userApi == NULL)
	{
		LOGE("创建EES EESQuoteApi 失败");
		return false;
	}

	return true;
}

void SpiderEesMdSpi::start()
{
	//优先使用组播行情
	if (multi_list.size() > 0)
	{
		if (!userApi->InitMulticast(multi_list, this))
		{
			LOGE("EES 组播行情启动失败");
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
			LOGE("EES 连接TCP行情服务器失败");
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
	//组播模式不需要登录
	EqsLoginParam reqUserLogin;
	strncpy(reqUserLogin.m_loginId, myAccount.account_id, sizeof(reqUserLogin.m_loginId) - 1);
	strncpy(reqUserLogin.m_password, myAccount.password, sizeof(reqUserLogin.m_password) - 1);

	LOGI("EESMarketDataSession ReqUserLoginis:" << reqUserLogin.m_loginId);

	userApi->LoginToEqs(reqUserLogin);
}

/// \brief 当服务器连接成功，登录前调用, 如果是组播模式不会发生, 只需判断InitMulticast返回值即可
void SpiderEesMdSpi::OnEqsConnected()
{
	LOGI(myAccount.account_id << ", EES:连接成功");
	smd->on_connected();
	login();
}

/// \brief 当服务器曾经连接成功，被断开时调用，组播模式不会发生该事件
void SpiderEesMdSpi::OnEqsDisconnected()
{
	LOGE(myAccount.account_id << ", EES:连接中断");
	smd->on_disconnected();
}

/// \brief 当登录成功或者失败时调用，组播模式不会发生
/// \param bSuccess 登陆是否成功标志  
/// \param pReason  登陆失败原因  
void SpiderEesMdSpi::OnLoginResponse(bool bSuccess, const char* pReason)
{
	if (bSuccess)
	{
		LOGI(myAccount.account_id << ", EES:登陆成功");
		if (smd)
		{
			smd->on_log_in();
			//重新订阅一下行情
			smd->subscribe(smd->get_subscribed_list());
		}
		
	}
	else {
		LOGW(myAccount.account_id << ", EES:登录失败: " << pReason );
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, pReason, sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

/// \brief 注册symbol响应消息来时调用，组播模式不支持行情注册
/// \param chInstrumentType  EES行情类型
/// \param pSymbol           合约名称
/// \param bSuccess          注册是否成功标志
void SpiderEesMdSpi::OnSymbolRegisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess)
{
	if (bSuccess)
	{
		smd->updateSubscribedList(pSymbol);
	}
	else{
		LOGI(myAccount.account_id << ", EES:订阅失败" << chInstrumentType << ":" << pSymbol);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "订阅失败", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

/// \brief  注销symbol响应消息来时调用，组播模式不支持行情注册
/// \param chInstrumentType  EES行情类型
/// \param pSymbol           合约名称
/// \param bSuccess          注册是否成功标志
void SpiderEesMdSpi::OnSymbolUnregisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess)
{
	if (bSuccess)
	{
		smd->updateSubscribedList(pSymbol,true);
	}
	else {
		LOGI(myAccount.account_id << ", EES:退订失败" << chInstrumentType << ":" << pSymbol);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "退订失败", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

/// \brief 收到行情时调用,具体格式根据instrument_type不同而不同
/// \param chInstrumentType  EES行情类型
/// \param pDepthQuoteData   EES统一行情指针  
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

/// \brief 日志接口，让使用者帮助写日志。
/// \param nlevel    日志级别
/// \param pLogText  日志内容
/// \param nLogLen   日志长度
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
			marketConnection->getUserApi()->RegisterSymbol(EesEqsIntrumentType::EQS_FUTURE, list[i].c_str()); //tcp 行情才需要
		}
		LOGI("EES: 订阅行情："<< EesEqsIntrumentType::EQS_FUTURE<<":"<< list[i]);
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
			marketConnection->getUserApi()->UnregisterSymbol(EesEqsIntrumentType::EQS_FUTURE, list[i].c_str()); //tcp 行情才需要
		}	
		LOGI("EES: 退订行情：" << EesEqsIntrumentType::EQS_FUTURE << ":" << list[i]);
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
		!ifSubscribed(md->Code)) //组播模式下，只发送订阅了的合约的行情 
	{
		return;
	}
	if (spider_common_api != NULL)
	{
		spider_common_api->onQuote(md);
	}
}

