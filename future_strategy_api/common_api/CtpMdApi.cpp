#include "CtpMdApi.h"
#include "LogWrapper.h"
#include "FutureApi.h"
#include "CommonDefine.h"

SpiderCtpMdSpi::SpiderCtpMdSpi()
	: userApi(NULL)
	, smd(NULL)
	, recv_count(0)
{

}

SpiderCtpMdSpi::~SpiderCtpMdSpi()
{

}

bool SpiderCtpMdSpi::init(SpiderCtpMdSession * sm)
{
	smd = sm;

	myAccount = sm->get_account();
	std::string workdir("./config/CTPQ");
	workdir += myAccount.account_id;
	userApi = CThostFtdcMdApi::CreateFtdcMdApi(workdir.c_str());
	if (userApi == NULL)
	{
		LOGE("创建CTP CThostFtdcMdApi 失败");
		return false;
	}
	userApi->RegisterSpi(this);

	return true;
}

void SpiderCtpMdSpi::start()
{
	for (unsigned int i = 0; i < 2; ++i)
	{
		if (strlen(myAccount.uri_list[i]) > 0)
		{
			userApi->RegisterFront(myAccount.uri_list[i]);
			LOGI("Info: CTP Market data receiver registered front server " << myAccount.uri_list[i]);
		}
	}
	userApi->Init();
}

void SpiderCtpMdSpi::stop()
{
	userApi->Release();
	userApi->Join();
}

void SpiderCtpMdSpi::login()
{
	CThostFtdcReqUserLoginField reqUserLogin;
	memset(&reqUserLogin, 0, sizeof(reqUserLogin));
	strncpy(reqUserLogin.BrokerID, myAccount.broker_id, sizeof(reqUserLogin.BrokerID) - 1);
	strncpy(reqUserLogin.UserID, myAccount.account_id, sizeof(reqUserLogin.UserID) - 1);
	strncpy(reqUserLogin.Password, myAccount.password, sizeof(reqUserLogin.Password) - 1);
	strncpy(reqUserLogin.UserProductInfo, myAccount.product_id, sizeof(reqUserLogin.UserProductInfo) - 1);
	LOGI("CTPMarketDataSession ReqUserLogin.UserProductInfo is: " << reqUserLogin.UserProductInfo <<", "<< reqUserLogin.BrokerID << ", " << reqUserLogin.UserID);

	int r = userApi->ReqUserLogin(&reqUserLogin, get_seq());
	if (r != 0)
	{
		LOGW("Warning: CTPMarketDataSession encountered an error when try to login, error no: "<< r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "ReqUserLogin调用失败", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

void SpiderCtpMdSpi::OnFrontConnected()
{
	LOGI(myAccount.account_id <<", CTP:连接成功");
	smd->on_connected();
	login();
}

void SpiderCtpMdSpi::OnFrontDisconnected(int nReason)
{
	LOGE(myAccount.account_id << ", CTP:连接中断: "<<nReason);
	smd->on_disconnected();
}

///登录请求响应
void SpiderCtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{	
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspUserLogin: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pRspUserLogin != NULL)
	{
		LOGI(myAccount.account_id << ", CTP:登陆成功，frontid从"<< myAccount.front_id<<"变更为"<<pRspUserLogin->FrontID<<"，sessionid从"<<myAccount.session_id<<"变更为"<<pRspUserLogin->SessionID);
		sprintf(myAccount.front_id,"%d", pRspUserLogin->FrontID);
		sprintf(myAccount.session_id, "%d", pRspUserLogin->SessionID);
		LOGD("交易所时间打印："<<getNowString()<<", "<<pRspUserLogin->SHFETime << ", " << pRspUserLogin->DCETime << ", " << pRspUserLogin->CZCETime << ", " << pRspUserLogin->FFEXTime << ", " << pRspUserLogin->INETime);
		if (smd)
		{
			smd->on_log_in();
			//重新订阅一下行情
			smd->subscribe(smd->get_subscribed_list());
		}
	}
}

///登出请求响应
void SpiderCtpMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{		
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspUserLogout: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pUserLogout != NULL)
	{
		LOGI(myAccount.account_id << ", CTP:登出成功: "<<pUserLogout->BrokerID<<", "<<pUserLogout->UserID);
		if (smd)
		{
			smd->on_log_out();
		}
	}
}

///错误应答
void SpiderCtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		LOGE(myAccount.account_id << ", CTP:收到错误消息：" << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = pRspInfo->ErrorID;
		strncpy(_err->ErrMsg, pRspInfo->ErrorMsg,sizeof(_err->ErrMsg)-1);
		smd->on_error(_err);
	}
}

///订阅行情应答
void SpiderCtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{	
		if (pRspInfo->ErrorID != 0)
		{
			LOGI(myAccount.account_id << ", CTP:OnRspSubMarketData: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pSpecificInstrument != NULL)
	{
		smd->updateSubscribedList(pSpecificInstrument->InstrumentID);
	}
}

///取消订阅行情应答
void SpiderCtpMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{	
		if (pRspInfo->ErrorID != 0)
		{
			LOGI(myAccount.account_id << ", CTP:OnRspUnSubMarketData: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pSpecificInstrument != NULL)
	{
		smd->updateSubscribedList(pSpecificInstrument->InstrumentID,true);
	}
}

///深度行情通知
void SpiderCtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	if (pDepthMarketData != NULL)
	{
		if (++recv_count % 100 == 0)
		{
			LOGD("SpiderCtpMdSpi received: " << recv_count);
		}
		QuotaData * md = new QuotaData();
		md->ExchangeID = get_exid_from_ctp(pDepthMarketData->ExchangeID);
		md->UpdateMillisec = pDepthMarketData->UpdateMillisec;
		memcpy(md->TradingDay, pDepthMarketData->TradingDay, sizeof(md->TradingDay) - 1);
		memcpy(md->Code, pDepthMarketData->InstrumentID, sizeof(md->Code)-1);
		memcpy(md->UpdateTime, pDepthMarketData->UpdateTime, sizeof(md->UpdateTime) - 1);
		md->AskPrice1 = pDepthMarketData->AskPrice1;
		md->AskVolume1 = pDepthMarketData->AskVolume1;
		md->BidPrice1 = pDepthMarketData->BidPrice1;
		md->BidVolume1 = pDepthMarketData->BidVolume1;
		md->LastPrice = pDepthMarketData->LastPrice;
		md->HighestPrice = pDepthMarketData->HighestPrice;
		md->LowestPrice = pDepthMarketData->LowestPrice;
		md->LowerLimitPrice = pDepthMarketData->LowerLimitPrice;
		md->UpperLimitPrice = pDepthMarketData->UpperLimitPrice;
		md->OpenPrice = pDepthMarketData->OpenPrice;
		md->PreClosePrice = pDepthMarketData->PreClosePrice;
		md->ClosePrice = pDepthMarketData->ClosePrice;
		md->PreSettlementPrice = pDepthMarketData->PreSettlementPrice;
		md->SettlementPrice = pDepthMarketData->SettlementPrice;
		md->PreOpenInterest = pDepthMarketData->PreOpenInterest;
		md->OpenInterest = pDepthMarketData->OpenInterest;
		md->Turnover = pDepthMarketData->Turnover;
		md->Volume = pDepthMarketData->Volume;

		if(smd)
			smd->on_receive_data(md);
	}
}


//*********************************************************

SpiderCtpMdSession::SpiderCtpMdSession(SpiderCommonApi * sci, AccountInfo & ai)
	:BaseMarketSession(sci,ai)
{
	marketConnection.reset(new SpiderCtpMdSpi());
}

SpiderCtpMdSession::~SpiderCtpMdSession()
{

}

bool SpiderCtpMdSession::init()
{
	return marketConnection->init(this);
}

void SpiderCtpMdSession::start()
{
	marketConnection->start();
}

void SpiderCtpMdSession::stop()
{
	marketConnection->stop();
}

void SpiderCtpMdSession::subscribe(std::vector<std::string> & list)
{
	if (!list.empty())
	{
		std::vector<char *> contracts;
		for (unsigned int i = 0; i < list.size(); ++i)
		{
			contracts.push_back(const_cast<char *>(list[i].c_str()));
		}
		int result = marketConnection->getUserApi()->SubscribeMarketData(&contracts[0], contracts.size());
		LOGI("subscribed to " << contracts.size() << ", result: " << result <<", "<< contracts[0]);
		if (result != 0)
		{
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = result;
			strncpy(_err->ErrMsg, "SubscribeMarketData调用失败", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
		}
	}
}

void SpiderCtpMdSession::unsubscribe(std::vector<std::string> & list)
{
	if (!list.empty())
	{
		std::vector<char *> contracts;
		for (unsigned int i = 0; i < list.size(); ++i)
		{
			contracts.push_back(const_cast<char *>(list[i].c_str()));
		}
		int result = marketConnection->getUserApi()->UnSubscribeMarketData(&contracts[0], contracts.size());
		LOGI("unsubscribed to " << contracts.size() << ", result: " << result);
		if (result != 0)
		{
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = result;
			strncpy(_err->ErrMsg, "UnSubscribeMarketData调用失败", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
		}
	}
}

void SpiderCtpMdSession::on_connected()
{
	if (spider_common_api != NULL)
	{
		//if (spider_common_api->callMarketConnect != NULL)
		//{
		//	spider_common_api->callMarketConnect();
		//}
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnConnect();
		}
	}
}

void SpiderCtpMdSession::on_disconnected()
{
	if (spider_common_api != NULL )
	{
		//if (spider_common_api->callMarketDisconnect!= NULL)
		//{
		//	spider_common_api->callMarketDisconnect();
		//}
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnDisconnect();
		}
	}
}

void SpiderCtpMdSession::on_log_in()
{
	if (spider_common_api != NULL )
	{
		//if (spider_common_api->callMarketLogin != NULL)
		//{
		//	spider_common_api->callMarketLogin(get_account().account_id);
		//}
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderCtpMdSession::on_log_out()
{
	if (spider_common_api != NULL)
	{
		//if (spider_common_api->callMarketLogout != NULL)
		//{
		//	spider_common_api->callMarketLogout();
		//}
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogout();
		}
	}
}

void SpiderCtpMdSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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

void SpiderCtpMdSession::on_receive_data(QuotaData * md)
{
	if (spider_common_api != NULL)
	{
		spider_common_api->onQuote(md);
	}
}

