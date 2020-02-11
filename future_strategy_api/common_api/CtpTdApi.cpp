#include "CtpTdApi.h"
#include "LogWrapper.h"
#include "FutureApi.h"
#include "CommonDefine.h"

SpiderCtpTdSpi::SpiderCtpTdSpi()
	: userApi(NULL)
	, smd(NULL)
{

}

SpiderCtpTdSpi::~SpiderCtpTdSpi()
{

}


bool SpiderCtpTdSpi::init(SpiderCtpTdSession * sm)
{
	smd = sm;
	myAccount = sm->get_account();
	std::string workdir("./config/CTP");
	workdir += myAccount.account_id;
	userApi = CThostFtdcTraderApi::CreateFtdcTraderApi(workdir.c_str());
	if (userApi == NULL)
	{
		LOGE("创建CTP CThostFtdcTraderApi 失败");
		return false;
	}
	userApi->RegisterSpi(this);

	return true;
}

void SpiderCtpTdSpi::start()
{
	for (unsigned int i = 0; i < 2; ++i)
	{
		if (strlen(myAccount.uri_list[i]) > 0)
		{
			userApi->RegisterFront(myAccount.uri_list[i]);
			LOGI("Info: CTP trade engine registered front server " << myAccount.uri_list[i]);
		}
	}
	userApi->SubscribePrivateTopic(THOST_TE_RESUME_TYPE::THOST_TERT_RESUME);
	userApi->SubscribePublicTopic(THOST_TE_RESUME_TYPE::THOST_TERT_RESUME);
	userApi->Init();
}

void SpiderCtpTdSpi::stop()
{
	userApi->Release();
	userApi->Join();
}

void SpiderCtpTdSpi::authenticate()
{
	CThostFtdcReqAuthenticateField field;
	memset(&field, 0, sizeof(field));
	strncpy(field.BrokerID,myAccount.broker_id,sizeof(field.BrokerID)-1);
	strncpy(field.UserID, myAccount.account_id, sizeof(field.UserID) - 1);
	strncpy(field.UserProductInfo, myAccount.product_id, sizeof(field.UserProductInfo) - 1);
	strncpy(field.AppID, myAccount.app_id, sizeof(field.AppID) - 1);
	strncpy(field.AuthCode, myAccount.auth_code, sizeof(field.AuthCode) - 1);
	LOGI("CTPTradeDataSession ReqAuthenticate.UserProductInfo is: " << field.UserProductInfo << ", " << field.BrokerID << ", " << field.UserID);
	int r = userApi->ReqAuthenticate(&field, get_seq());
	if (r != 0)
	{
		LOGW("Warning: CTPTradeDataSession encountered an error when try to authenticate, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "ReqAuthenticate调用失败", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

void SpiderCtpTdSpi::login()
{
	CThostFtdcReqUserLoginField reqUserLogin;
	memset(&reqUserLogin, 0, sizeof(reqUserLogin));
	strncpy(reqUserLogin.BrokerID, myAccount.broker_id, sizeof(reqUserLogin.BrokerID) - 1);
	strncpy(reqUserLogin.UserID, myAccount.account_id, sizeof(reqUserLogin.UserID) - 1);
	strncpy(reqUserLogin.Password, myAccount.password, sizeof(reqUserLogin.Password) - 1);
	strncpy(reqUserLogin.UserProductInfo, myAccount.product_id, sizeof(reqUserLogin.UserProductInfo) - 1);
	LOGI("CTPTradeDataSession ReqUserLogin.UserProductInfo is: " << reqUserLogin.UserProductInfo << ", " << reqUserLogin.BrokerID << ", " << reqUserLogin.UserID);

	int r = userApi->ReqUserLogin(&reqUserLogin, get_seq());
	if (r != 0)
	{
		LOGW("Warning: CTPTradeDataSession encountered an error when try to login, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "ReqUserLogin调用失败", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

void SpiderCtpTdSpi::OnFrontConnected()
{
	LOGI(myAccount.account_id << ", CTP:连接成功");
	smd->on_connected();
	authenticate();
}

void SpiderCtpTdSpi::OnFrontDisconnected(int nReason)
{
	LOGE(myAccount.account_id << ", CTP:连接中断: " << nReason);
	smd->on_disconnected();
}

///客户端认证响应
void SpiderCtpTdSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspAuthenticate: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	login();
}

///登录请求响应
void SpiderCtpTdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
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
		LOGI(myAccount.account_id << ", CTP:登陆成功，frontid从" << myAccount.front_id << "变更为" << pRspUserLogin->FrontID << "，sessionid从" << myAccount.session_id << "变更为" << pRspUserLogin->SessionID << "，MaxOrderRef:" << pRspUserLogin->MaxOrderRef);
		sprintf(myAccount.front_id, "%d", pRspUserLogin->FrontID);
		sprintf(myAccount.session_id, "%d", pRspUserLogin->SessionID);
		//int _exoid = atoi(pRspUserLogin->MaxOrderRef);
		LOGD("交易所时间打印：" << pRspUserLogin->SHFETime << ", " << pRspUserLogin->DCETime << ", " << pRspUserLogin->CZCETime << ", " << pRspUserLogin->FFEXTime << ", " << pRspUserLogin->INETime);
		
		//确认结算单
		CThostFtdcSettlementInfoConfirmField field;
		memset(&field,0,sizeof(field));
		strncpy(field.BrokerID, myAccount.broker_id,sizeof(field.BrokerID)-1);
		strncpy(field.InvestorID, myAccount.account_id, sizeof(field.InvestorID) - 1);
		strncpy(field.ConfirmDate, userApi->GetTradingDay(), sizeof(field.ConfirmDate) - 1);
		strncpy(field.ConfirmTime, "00:00:00", sizeof(field.ConfirmTime) - 1);
		int r = userApi->ReqSettlementInfoConfirm(&field,get_seq());
		if (r != 0)
		{
			LOGW(myAccount.account_id << ", CTP:结算清单确认错误："<<r);
		}

		if (smd)
		{
			//smd->init_exoid(_exoid / 1000); //重置一下orderref
			smd->on_log_in();
		}
	}
}

///登出请求响应
void SpiderCtpTdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
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
		LOGI(myAccount.account_id << ", CTP:登出成功: " << pUserLogout->BrokerID << ", " << pUserLogout->UserID);
		if (smd)
		{
			smd->on_log_out();
		}
	}
}

///错误应答
void SpiderCtpTdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		LOGE(myAccount.account_id << ", CTP:收到错误消息：" << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = pRspInfo->ErrorID;
		strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

///报单录入请求响应
void SpiderCtpTdSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{	
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspOrderInsert: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			//std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			//_err->ErrCode = pRspInfo->ErrorID;
			//strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			//smd->on_error(_err);
			//if (pInputOrder == NULL)
			//{
			//	return;
			//}
			//OrderInfo * order = new OrderInfo();
			//strncpy(order->Code, pInputOrder->InstrumentID, sizeof(order->Code) - 1);
			//strncpy(order->OrderRef, pInputOrder->OrderRef, sizeof(order->OrderRef) - 1);
			//order->ExchangeID = get_exid_from_ctp(pInputOrder->ExchangeID);
			//order->Direction = get_direction_from_ctp(pInputOrder->Direction);
			//order->Offset = get_offset_from_ctp(pInputOrder->CombOffsetFlag[0]);
			//order->HedgeFlag = Speculation;
			//order->LimitPrice = pInputOrder->LimitPrice;
			//order->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
			//order->OrderStatus = -1; //约定好的失败
			//strncpy(order->StatusMsg, pRspInfo->ErrorMsg, sizeof(order->StatusMsg) - 1);
			//if (smd)
			//	smd->on_order_insert(order);
			//delete order;
			return;
		}
	}
	if (pInputOrder != NULL)
	{
		LOGI(myAccount.account_id << ", CTP:报单成功，OrderRef：" << pInputOrder->OrderRef<<"，Code："<<pInputOrder->InstrumentID);
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, pInputOrder->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pInputOrder->OrderRef, sizeof(order->OrderRef) - 1);
		order->ExchangeID = get_exid_from_ctp(pInputOrder->ExchangeID);
		order->Direction = get_direction_from_ctp(pInputOrder->Direction);
		order->Offset = get_offset_from_ctp(pInputOrder->CombOffsetFlag[0]);
		order->HedgeFlag = Speculation;
		order->LimitPrice = pInputOrder->LimitPrice;
		order->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
		if (smd)
			smd->on_order_insert(order);
		delete order;
	}
}

///报单操作请求响应
void SpiderCtpTdSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{	
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspOrderAction: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			//std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			//_err->ErrCode = pRspInfo->ErrorID;
			//strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			//smd->on_error(_err);
			//if (pInputOrderAction == NULL)
			//{
			//	return;
			//}
			//OrderInfo * order = new OrderInfo();
			//strncpy(order->Code, pInputOrderAction->InstrumentID, sizeof(order->Code) - 1);
			//strncpy(order->OrderRef, pInputOrderAction->OrderRef, sizeof(order->OrderRef) - 1);
			//strncpy(order->OrderSysID, pInputOrderAction->OrderSysID, sizeof(order->OrderSysID) - 1);
			//order->ExchangeID = get_exid_from_ctp(pInputOrderAction->ExchangeID);
			//order->LimitPrice = pInputOrderAction->LimitPrice;
			//order->OrderStatus = -1; //约定好的失败
			//strncpy(order->StatusMsg, pRspInfo->ErrorMsg, sizeof(order->StatusMsg) - 1);
			//if (smd)
			//	smd->on_order_cancel(order);
			//delete order;
			return;
		}
	}
	if (pInputOrderAction != NULL)
	{
		LOGI(myAccount.account_id << ", CTP:撤单成功，OrderRef：" << pInputOrderAction->OrderRef <<"，OrderSysID："<< pInputOrderAction->OrderSysID<<"， ActionRef："<< pInputOrderAction->OrderActionRef<< "，Code：" << pInputOrderAction->InstrumentID);
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, pInputOrderAction->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pInputOrderAction->OrderRef, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pInputOrderAction->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->ExchangeID = get_exid_from_ctp(pInputOrderAction->ExchangeID);
		order->LimitPrice = pInputOrderAction->LimitPrice;
		if (smd)
			smd->on_order_cancel(order);
		delete order;
	}
}

///报单通知
void SpiderCtpTdSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	if (pOrder != NULL)
	{
		LOGI(myAccount.account_id << ", CTP:委托推送，OrderRef：" << pOrder->OrderRef << "，OrderSysID：" << pOrder->OrderSysID << "， Code：" << pOrder->InstrumentID << "， Direction：" << pOrder->Direction << "， OrderStatus：" << pOrder->OrderStatus << "，StatusMsg：" << pOrder->StatusMsg);
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, pOrder->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pOrder->OrderRef, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pOrder->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->LimitPrice = pOrder->LimitPrice;
		order->VolumeTotalOriginal = pOrder->VolumeTotalOriginal;
		order->VolumeTraded = pOrder->VolumeTraded;
		order->VolumeTotal = pOrder->VolumeTotal;
		order->ExchangeID = get_exid_from_ctp(pOrder->ExchangeID);
		order->Direction = get_direction_from_ctp(pOrder->Direction);
		order->OrderStatus = get_orderstatus_from_ctp(pOrder->OrderStatus);
		order->HedgeFlag = Speculation;
		order->Offset = get_offset_from_ctp(pOrder->CombOffsetFlag[0]);
		strncpy(order->StatusMsg, pOrder->StatusMsg, sizeof(order->StatusMsg) - 1);
		sprintf_s(order->OrderTime,sizeof(order->OrderTime)-1,"%s.000",pOrder->InsertTime);
		if (smd)
			smd->on_order_change(order);
		delete order;
	}
}

///成交通知
void SpiderCtpTdSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	if (pTrade != NULL)
	{
		LOGI(myAccount.account_id << ", CTP:成交回报，OrderRef：" << pTrade->OrderRef << "，OrderSysID：" << pTrade->OrderSysID << "， Code：" << pTrade->InstrumentID << "， Price：" << pTrade->Price << "，Volume：" << pTrade->Volume <<"，SequenceNo："<<pTrade->SequenceNo);
		TradeInfo * trade = new TradeInfo();
		memset(trade,0,sizeof(TradeInfo));
		strncpy(trade->Code, pTrade->InstrumentID, sizeof(trade->Code) - 1);
		strncpy(trade->OrderRef, pTrade->OrderRef, sizeof(trade->OrderRef) - 1);
		strncpy(trade->OrderSysID, pTrade->OrderSysID, sizeof(trade->OrderSysID) - 1);
		strncpy(trade->TradeID, pTrade->TradeID, sizeof(trade->TradeID) - 1);
		sprintf_s(trade->TradeTime, sizeof(trade->TradeTime) - 1, "%s.000", pTrade->TradeTime);
		//strncpy(trade->TradeTime, pTrade->TradeTime, sizeof(trade->TradeTime) - 1);
		trade->ExchangeID = get_exid_from_ctp(pTrade->ExchangeID);;
		trade->Direction = get_direction_from_ctp(pTrade->Direction);
		trade->HedgeFlag = Speculation;
		trade->Offset = get_offset_from_ctp(pTrade->OffsetFlag);
		trade->Price = pTrade->Price;
		trade->Volume = pTrade->Volume;
		if (smd)
			smd->on_trade_change(trade);
		delete trade;
	}
}

///报单录入错误回报
void SpiderCtpTdSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo != NULL)
	{		
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnErrRtnOrderInsert: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			//std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			//_err->ErrCode = pRspInfo->ErrorID;
			//strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			//smd->on_error(_err);
			//return;
		}
	}
	if (pInputOrder != NULL)
	{
		LOGW(myAccount.account_id << ", CTP:报单失败，OrderRef：" << pInputOrder->OrderRef << "，Code：" << pInputOrder->InstrumentID);

		OrderInfo * order = new OrderInfo();
		strncpy(order->Code, pInputOrder->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pInputOrder->OrderRef, sizeof(order->OrderRef) - 1);
		order->ExchangeID = get_exid_from_ctp(pInputOrder->ExchangeID);
		order->Direction = get_direction_from_ctp(pInputOrder->Direction);
		order->Offset = get_offset_from_ctp(pInputOrder->CombOffsetFlag[0]);
		order->HedgeFlag = Speculation;
		order->LimitPrice = pInputOrder->LimitPrice;
		order->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
		order->OrderStatus = -1; //约定好的失败
		strncpy(order->StatusMsg, pRspInfo->ErrorMsg, sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_insert(order);
		delete order;
	}
}

///报单操作错误回报
void SpiderCtpTdSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnErrRtnOrderAction: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			//std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			//_err->ErrCode = pRspInfo->ErrorID;
			//strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			//smd->on_error(_err);
			//return;
		}
	}
	if (pOrderAction != NULL)
	{
		LOGW(myAccount.account_id << ", CTP:撤单失败，OrderRef：" << pOrderAction->OrderRef << "，OrderSysID：" << pOrderAction->OrderSysID << "， ActionRef：" << pOrderAction->OrderActionRef << "，Code：" << pOrderAction->InstrumentID << "，Status："<< pOrderAction->StatusMsg);
		OrderInfo * order = new OrderInfo();
		strncpy(order->Code, pOrderAction->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pOrderAction->OrderRef, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pOrderAction->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->ExchangeID = get_exid_from_ctp(pOrderAction->ExchangeID);
		order->LimitPrice = pOrderAction->LimitPrice;
		order->OrderStatus = -1; //约定好的失败
		strncpy(order->StatusMsg, pRspInfo->ErrorMsg, sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_cancel(order);
		delete order;
	}
}

///请求查询报单响应
void SpiderCtpTdSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspQryOrder: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pOrder != NULL)
	{
		LOGD(myAccount.account_id << ", CTP:OnRspQryOrder: " << pOrder->InstrumentID << ":" << bIsLast);
		OrderInfo * order = new OrderInfo;
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, pOrder->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pOrder->OrderRef, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pOrder->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->LimitPrice = pOrder->LimitPrice;
		order->VolumeTotalOriginal = pOrder->VolumeTotalOriginal;
		order->VolumeTraded = pOrder->VolumeTraded;
		order->VolumeTotal = pOrder->VolumeTotal;
		order->ExchangeID = get_exid_from_ctp(pOrder->ExchangeID);;
		order->Direction = get_direction_from_ctp(pOrder->Direction);
		order->OrderStatus = get_orderstatus_from_ctp(pOrder->OrderStatus);
		order->HedgeFlag = Speculation;
		order->Offset = get_offset_from_ctp(pOrder->CombOffsetFlag[0]);
		sprintf_s(order->OrderTime, sizeof(order->OrderTime) - 1, "%s.000", pOrder->InsertTime);
		if (smd)
		{
			smd->on_rsp_query_order(order, nRequestID, bIsLast);
		}
	}
}

///请求查询成交响应
void SpiderCtpTdSpi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspQryTrade: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pTrade != NULL)
	{
		LOGD(myAccount.account_id << ", CTP:OnRspQryTrade: " << pTrade->InstrumentID << ":" << bIsLast);
		TradeInfo * trade = new TradeInfo;
		memset(trade, 0, sizeof(TradeInfo));
		strncpy(trade->Code, pTrade->InstrumentID, sizeof(trade->Code) - 1);
		strncpy(trade->OrderRef, pTrade->OrderRef, sizeof(trade->OrderRef) - 1);
		strncpy(trade->OrderSysID, pTrade->OrderSysID, sizeof(trade->OrderSysID) - 1);
		strncpy(trade->TradeID, pTrade->TradeID, sizeof(trade->TradeID) - 1);
		//strncpy(trade->TradeTime, pTrade->TradeTime, sizeof(trade->TradeTime) - 1);
		sprintf_s(trade->TradeTime, sizeof(trade->TradeTime) - 1, "%s.000", pTrade->TradeTime);
		trade->ExchangeID = get_exid_from_ctp(pTrade->ExchangeID);;
		trade->Direction = get_direction_from_ctp(pTrade->Direction);
		trade->HedgeFlag = Speculation;
		trade->Offset = get_offset_from_ctp(pTrade->OffsetFlag);
		trade->Price = pTrade->Price;
		trade->Volume = pTrade->Volume;
		if (smd)
		{
			smd->on_rsp_query_trade(trade, nRequestID, bIsLast);
		}
	}
}

///请求查询投资者持仓响应
void SpiderCtpTdSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspQryInvestorPosition: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pInvestorPosition != NULL)
	{
		LOGD(myAccount.account_id << ", CTP:OnRspQryInvestorPosition: " << pInvestorPosition->InstrumentID << ":" << bIsLast << ":" << pInvestorPosition->PosiDirection << ":" << pInvestorPosition->Position);
		InvestorPosition * pos = new InvestorPosition;
		memset(pos, 0, sizeof(InvestorPosition));
		strncpy(pos->AccountID, pInvestorPosition->InvestorID, sizeof(pos->AccountID) - 1);
		strncpy(pos->Code, pInvestorPosition->InstrumentID, sizeof(pos->Code) - 1);
		pos->HedgeFlag = Speculation;
		pos->PosiDirection = get_position_direct_from_ctp(pInvestorPosition->PosiDirection);
		pos->OpenCost = pInvestorPosition->OpenCost;
		pos->Position = pInvestorPosition->Position;
		pos->PositionCost = pInvestorPosition->PositionCost;
		pos->ProfitLoss = pInvestorPosition->PositionProfit;
		//pos->StockAvailable = pInvestorPosition->StockAvailable;
		pos->TodayPosition = pInvestorPosition->TodayPosition;
		pos->YesterdayPosition = pos->Position - pos->TodayPosition; //CTP的YdPosition是上一日的快照持仓数量
		if (smd)
		{
			smd->on_rsp_query_position(pos, nRequestID, bIsLast);
		}
	}
}

///请求查询资金账户响应
void SpiderCtpTdSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", CTP:OnRspQryTradingAccount: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pTradingAccount != NULL)
	{
		LOGD(myAccount.account_id << ", CTP:OnRspQryTradingAccount: " << pTradingAccount->Available << ":" << bIsLast);
		TradingAccount * account = new TradingAccount();
		memset(account, 0, sizeof(TradingAccount));
		strncpy(account->AccountID, pTradingAccount->AccountID, sizeof(account->AccountID) - 1);
		strncpy(account->Currency, pTradingAccount->CurrencyID, sizeof(account->Currency) - 1);
		strncpy(account->TradingDay, pTradingAccount->TradingDay, sizeof(account->TradingDay) - 1);
		account->Available = pTradingAccount->Available;
		account->Balance = pTradingAccount->Balance;
		account->CloseProfit = pTradingAccount->CloseProfit;
		account->Commission = pTradingAccount->Commission;
		account->CurrMargin = pTradingAccount->CurrMargin;
		account->Deposit = pTradingAccount->Deposit;
		account->FrozenCash = pTradingAccount->FrozenCash;
		account->FrozenCommission = pTradingAccount->FrozenCommission;
		account->FrozenMargin = pTradingAccount->FrozenMargin;
		account->PositionProfit = pTradingAccount->PositionProfit;
		account->PreBalance = pTradingAccount->PreBalance;
		account->Withdraw = pTradingAccount->Withdraw;
		account->WithdrawQuota = pTradingAccount->WithdrawQuota;
		if (smd)
		{
			smd->on_rsp_query_account(account, nRequestID, bIsLast);
		}
	}
}

//*********************************************************

SpiderCtpTdSession::SpiderCtpTdSession(SpiderCommonApi * sci, AccountInfo & ai)
	:BaseTradeSession(sci, ai)
{
	tradeConnection.reset(new SpiderCtpTdSpi());
}

SpiderCtpTdSession::~SpiderCtpTdSession()
{

}

bool SpiderCtpTdSession::init()
{
	return tradeConnection->init(this);
}

void SpiderCtpTdSession::start()
{
	tradeConnection->start();
}

void SpiderCtpTdSession::stop()
{
	tradeConnection->stop();
}

const char * SpiderCtpTdSession::insert_order(OrderInsert * order)
{
	CThostFtdcInputOrderField field;
	memset(&field,0,sizeof(field));
	//LOGD(get_exid((EnumExchangeIDType)order->ExchangeID));
	strncpy(field.BrokerID, get_account().broker_id, sizeof(field.BrokerID) -1 );
	strncpy(field.InvestorID, get_account().account_id, sizeof(field.InvestorID) - 1);
	strncpy(field.UserID, get_account().account_id, sizeof(field.UserID) - 1);
	strncpy(field.InstrumentID, order->Code, sizeof(field.InstrumentID) - 1);
	strncpy(field.ExchangeID, get_exid_to_ctp((EnumExchangeIDType)order->ExchangeID), sizeof(field.ExchangeID) - 1);
	field.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	field.Direction = order->Direction == EnumDirectionType::Buy ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
	switch (order->Offset)
	{
	case EnumOffsetFlagType::Open:
	{
		field.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		break;
	}
	case EnumOffsetFlagType::Close:
	{
		if (order->ExchangeID == EnumExchangeIDType::SHFE)
		{
			field.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
		}
		else {
			field.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
		}	
		break;
	}	
	case EnumOffsetFlagType::CloseToday:
	{
		field.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
		break;
	}	
	case EnumOffsetFlagType::CloseYesterday:
	{
		field.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
		break;
	}
	default:
	{
		LOGE("错误的开平方向：" << order->Offset);
		field.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
		break;
	}
	}
	field.CombOffsetFlag[1] = 0;
	strcpy(field.CombHedgeFlag, "1");// 组合投机套保标志
	field.LimitPrice = order->LimitPrice;// 价格
	field.VolumeTotalOriginal = order->VolumeTotalOriginal;// 数量
	field.TimeCondition = THOST_FTDC_TC_GFD;// 有效期类型
	strcpy(field.GTDDate, "");// GTD日期
	field.VolumeCondition = THOST_FTDC_VC_AV;// 成交量类型
	field.MinVolume = 0;// 最小成交量
	field.ContingentCondition = THOST_FTDC_CC_Immediately;// 触发条件
	field.StopPrice = 0;// 止损价
	field.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;// 强平原因
	field.IsAutoSuspend = 0;// 自动挂起标志

	//sprintf(field.OrderRef, "%d", get_real_order_ref(atoi(order->OrderRef)));
	const char * _order_ref = get_order_ref(order->OrderRef);
	strncpy(field.OrderRef, _order_ref,sizeof(field.OrderRef)-1);
	//delete[] _order_ref; //要在使用的地方删除掉,api使用者自己释放

	int r = tradeConnection->getUserApi()->ReqOrderInsert(&field,get_seq());
	if (r != 0)
	{
		LOGW("Warning: CTP ReqOrderInsert, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "ReqOrderInsert调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}

	return _order_ref;
}

void SpiderCtpTdSession::cancel_order(OrderCancel * order)
{
	CThostFtdcInputOrderActionField action;
	memset(&action, 0, sizeof(action));
	strncpy(action.ExchangeID, get_exid_to_ctp((EnumExchangeIDType)order->ExchangeID), sizeof(action.ExchangeID) - 1);
	strncpy(action.InstrumentID, order->Code, sizeof(action.InstrumentID) -1);// 合约代码
	strncpy(action.OrderSysID, order->OrderSysID, sizeof(action.OrderSysID)-1);
	//strncpy(action.OrderRef, order->OrderRef, sizeof(action.OrderRef) - 1);
	action.ActionFlag = THOST_FTDC_AF_Delete;
	strncpy(action.BrokerID, get_account().broker_id, sizeof(action.BrokerID)-1);
	strncpy(action.InvestorID, get_account().account_id, sizeof(action.InvestorID) - 1);
	strncpy(action.UserID, get_account().account_id, sizeof(action.UserID) - 1);
	action.OrderActionRef = get_seq();
	LOGD(get_account().account_id << ":CTP Withdraw: "<< action.ExchangeID <<", " << action.InstrumentID <<", "<< action.OrderSysID <<", "<< action.OrderRef);
	int r = tradeConnection->getUserApi()->ReqOrderAction(&action, get_seq());
	if (r != 0)
	{
		LOGW("Warning: CTP ReqOrderAction, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "ReqOrderAction调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}

void SpiderCtpTdSession::query_trading_account(int request_id)
{
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_account_info.find(request_id);
		if (it != cache_account_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " Warning: CTP 上一次查询账户资金尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户资金尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}
	CThostFtdcQryTradingAccountField query;
	memset(&query, 0, sizeof(query));
	strncpy(query.BrokerID, get_account().broker_id, sizeof(query.BrokerID) - 1);
	strncpy(query.InvestorID, get_account().account_id, sizeof(query.InvestorID) - 1);
	strncpy(query.AccountID, get_account().account_id, sizeof(query.AccountID) - 1);
	//strncpy(query.CurrencyID, get_account().account_id, sizeof(query.CurrencyID) - 1);
	query.BizType = '1';

	LOGI(get_account().account_id << ":CTP query clients trading_account, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->ReqQryTradingAccount(&query, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: CTPTradeConnection failed to query trading_account, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "ReqQryTradingAccount调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}

}
void SpiderCtpTdSession::query_positions(int request_id)
{
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_positions_info.find(request_id);
		if (it != cache_positions_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id<<" Warning: CTP 上一次查询账户持仓尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户持仓尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}
	CThostFtdcQryInvestorPositionField query;
	memset(&query, 0, sizeof(query));
	strncpy(query.BrokerID, get_account().broker_id, sizeof(query.BrokerID) - 1);
	strncpy(query.InvestorID, get_account().account_id, sizeof(query.InvestorID) - 1);

	LOGI(get_account().account_id<<":CTP query clients positions, requestid:"<<request_id);
	int result = tradeConnection->getUserApi()->ReqQryInvestorPosition(&query, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: CTPTradeConnection failed to query position, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "ReqQryInvestorPosition调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}
void SpiderCtpTdSession::query_orders(int request_id)
{
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_orders_info.find(request_id);
		if (it != cache_orders_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " Warning: CTP 上一次查询账户委托尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户委托尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}
	CThostFtdcQryOrderField query;
	memset(&query, 0, sizeof(query));
	strncpy(query.BrokerID, get_account().broker_id, sizeof(query.BrokerID) - 1);
	strncpy(query.InvestorID, get_account().account_id, sizeof(query.InvestorID) - 1);

	LOGI(get_account().account_id << ":CTP query clients orders, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->ReqQryOrder(&query, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: CTPTradeConnection failed to query orders, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "ReqQryOrder调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}
void SpiderCtpTdSession::query_trades(int request_id)
{
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_trades_info.find(request_id);
		if (it != cache_trades_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " Warning: CTP 上一次查询账户成交尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户成交尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}
	CThostFtdcQryTradeField query;
	memset(&query, 0, sizeof(query));
	strncpy(query.BrokerID, get_account().broker_id, sizeof(query.BrokerID) - 1);
	strncpy(query.InvestorID, get_account().account_id, sizeof(query.InvestorID) - 1);

	LOGI(get_account().account_id << ":CTP query clients trades, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->ReqQryTrade(&query, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: CTPTradeConnection failed to query trades, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "ReqQryTrade调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}

void SpiderCtpTdSession::on_connected()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnConnect();
		}
	}
}

void SpiderCtpTdSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnDisconnect();
		}
	}
}

void SpiderCtpTdSession::on_log_in()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderCtpTdSession::on_log_out()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnLogout();
		}
	}
}

void SpiderCtpTdSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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

//int SpiderCtpTdSession::get_real_order_ref(int order_ref)
//{
//	if (order_ref >= 1000) //和C#策略那边约定好了，他只负责生成3位的委托编号
//	{
//		order_ref = order_ref % 1000;
//	}
//	int real_order_ref = (++exoid) * 1000 + order_ref;
//	return real_order_ref;
//}

void SpiderCtpTdSession::on_order_change(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeRtnOrder(order);
		}
	}
}
void SpiderCtpTdSession::on_trade_change(TradeInfo * trade)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeRtnTrade(trade);
		}
	}
}
void SpiderCtpTdSession::on_order_cancel(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnOrderCancel(order);
		}
	}
}
void SpiderCtpTdSession::on_order_insert(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnOrderInsert(order);
		}
	}
}

void SpiderCtpTdSession::on_rsp_query_account(TradingAccount * account, int request_id, bool last)
{
	TradingAccount* * send_list = NULL;
	int send_count = 0;
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_account_info.find(request_id);
		if (it != cache_account_info.end())
		{
			it->second.push_back(account);
		}
		else {
			std::vector<TradingAccount* > tmp;
			tmp.push_back(account);
			cache_account_info.insert(std::make_pair(request_id,tmp));
		}
		if (last)
		{
			auto sendit = cache_account_info.find(request_id);
			std::vector<TradingAccount* >& _vec = sendit->second;
			send_count = (int)_vec.size();
			send_list = new TradingAccount *[send_count];
			for (int i = 0; i < send_count; ++i)
			{
				send_list[i] = _vec[i];
			}
			cache_account_info.erase(sendit);
		}
	}

	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL && send_count > 0 && send_list != NULL)
		{
			spider_common_api->notifySpi->tradeRspQueryAccount(send_list, send_count, request_id);
		}
	}
	if (send_count > 0 && send_list != NULL)
	{
		for (int i = 0; i < send_count; ++i)
		{
			delete send_list[i];
		}
		delete[] send_list;
	}
}
void SpiderCtpTdSession::on_rsp_query_position(InvestorPosition * position, int request_id, bool last)
{
	InvestorPosition* * send_list = NULL;
	int send_count = 0;
	std::string pos_key(position->Code);
	pos_key += "#";
	pos_key += std::to_string(position->PosiDirection);
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_positions_info.find(request_id);
		if (it != cache_positions_info.end())
		{
			if (position->Position > 0) //持仓大于零的再发送
			{
				//it->second.push_back(position);
				if (it->second.find(pos_key) == it->second.end())
				{
					it->second[pos_key] = position;
				}
				else {
					it->second[pos_key]->OpenCost += position->OpenCost;
					it->second[pos_key]->PositionCost += position->PositionCost;
					it->second[pos_key]->ProfitLoss += position->ProfitLoss;
					it->second[pos_key]->Position += position->Position;
					it->second[pos_key]->TodayPosition += position->TodayPosition;
					it->second[pos_key]->YesterdayPosition += position->YesterdayPosition;
					delete position;
				}
			}
			else {
				delete position;
			}
		}
		else {
			if (position->Position > 0) //持仓大于零的再发送
			{
				//std::vector<InvestorPosition* > tmp;
				//tmp.push_back(position);
				//cache_positions_info.insert(std::make_pair(request_id, tmp));
				std::map<std::string, InvestorPosition* > tmp;
				tmp.insert(std::make_pair(pos_key, position));
				cache_positions_info.insert(std::make_pair(request_id, tmp));
			}
			else {
				delete position;
			}
		}
		if (last)
		{
			auto sendit = cache_positions_info.find(request_id);
			if (sendit != cache_positions_info.end())
			{
				/*std::vector<InvestorPosition* >& _vec = sendit->second;*/
				//send_count = (int)_vec.size();
				//send_list = new InvestorPosition *[send_count];
				//for (int i = 0; i < send_count; ++i)
				//{
				//	send_list[i] = _vec[i];
				//}
				//cache_positions_info.erase(sendit);

				std::map<std::string, InvestorPosition* > & _map = sendit->second;
				send_count = (int)_map.size();
				send_list = new InvestorPosition *[send_count];
				int i = 0;
				for (auto it = _map.begin(); it != _map.end(); ++it)
				{
					send_list[i] = it->second;
					++i;
				}
				cache_positions_info.erase(sendit);
			}
		}
	}
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL && send_count > 0 && send_list != NULL)
		{
			spider_common_api->notifySpi->tradeRspQueryPosition(send_list, send_count, request_id);
		}
	}
	if (send_count > 0 && send_list != NULL)
	{
		for (int i = 0; i < send_count; ++i)
		{
			delete send_list[i];
		}
		delete[] send_list;
	}
}
void SpiderCtpTdSession::on_rsp_query_trade(TradeInfo * trade, int request_id, bool last)
{
	TradeInfo* * send_list = NULL;
	int send_count = 0;
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_trades_info.find(request_id);
		if (it != cache_trades_info.end())
		{
			it->second.push_back(trade);
		}
		else {
			std::vector<TradeInfo* > tmp;
			tmp.push_back(trade);
			cache_trades_info.insert(std::make_pair(request_id, tmp));
		}
		if (last)
		{
			auto sendit = cache_trades_info.find(request_id);
			std::vector<TradeInfo* >& _vec = sendit->second;
			send_count = (int)_vec.size();
			send_list = new TradeInfo *[send_count];
			for (int i = 0; i < send_count; ++i)
			{
				send_list[i] = _vec[i];
			}
			cache_trades_info.erase(sendit);
		}
	}
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL && send_count > 0 && send_list != NULL)
		{
			spider_common_api->notifySpi->tradeRspQueryTrade(send_list, send_count, request_id);
		}
	}
	if (send_count > 0 && send_list != NULL)
	{
		for (int i = 0; i < send_count; ++i)
		{
			delete send_list[i];
		}
		delete[] send_list;
	}
}
void SpiderCtpTdSession::on_rsp_query_order(OrderInfo * order, int request_id, bool last)
{
	OrderInfo* * send_list = NULL;
	int send_count = 0;
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_orders_info.find(request_id);
		if (it != cache_orders_info.end())
		{
			it->second.push_back(order);
		}
		else {
			std::vector<OrderInfo* > tmp;
			tmp.push_back(order);
			cache_orders_info.insert(std::make_pair(request_id, tmp));
		}
		if (last)
		{
			auto sendit = cache_orders_info.find(request_id);
			std::vector<OrderInfo* >& _vec = sendit->second;
			send_count = (int)_vec.size();
			send_list = new OrderInfo *[send_count];
			for (int i = 0; i < send_count; ++i)
			{
				send_list[i] = _vec[i];
			}
			cache_orders_info.erase(sendit);
		}
	}
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL && send_count > 0 && send_list != NULL)
		{
			spider_common_api->notifySpi->tradeRspQueryOrder(send_list, send_count, request_id);
		}
	}
	if (send_count > 0 && send_list != NULL)
	{
		for (int i = 0; i < send_count; ++i)
		{
			delete send_list[i];
		}
		delete[] send_list;
	}
}