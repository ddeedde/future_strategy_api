#include "XeleTdApi.h"
#include "LogWrapper.h"
#include "FutureApi.h"
#include "CommonDefine.h"


SpiderXeleTdSpi::SpiderXeleTdSpi()
	: userApi(NULL)
	, smd(NULL)
	, isReady(false)
{
	memset(trade_url, 0, sizeof(trade_url));
	memset(query_url, 0, sizeof(query_url));
}

SpiderXeleTdSpi::~SpiderXeleTdSpi()
{

}

bool SpiderXeleTdSpi::init(SpiderXeleTdSession * sm)
{
	//account->front_id 作为 ParticipantID
	//account->broker_id 作为 ClientID

	smd = sm;

	myAccount = sm->get_account();

	if (strlen(myAccount.uri_list[0]) > 0)
	{
		LOGI("Info: Xele TCP Trade Server: " << myAccount.uri_list[0]);
		std::vector<std::string> ips;
		split_str(myAccount.uri_list[0], ips, ":");

		if (ips.size() == 5) //tcp://127.0.0.1:31001:127.0.0.1:31002 //普通版，分别是交易地址和查询地址
		{
			replace_all(ips[1], "/", "");

			sprintf_s(trade_url, sizeof(trade_url), "tcp://%s:%s", ips[1].c_str(), ips[2].c_str());
			sprintf_s(query_url, sizeof(query_url), "tcp://%s:%s", ips[3].c_str(), ips[4].c_str());

		}
		else {
			LOGE("Xele 配置文件中交易地址格式错误");
			return false;
		}
	}
	else {
		LOGE("配置文件中服务器地址不能为空");
		return false;
	}

	x_handle = LoadLibrary(TEXT("libXeleTdAPI.dll"));
	if (!x_handle) {
		LOGE("Load library: " << XELE_API_DLL_NAME <<" failed, errno:"<< GetLastError());
		return false;
	}

	funcCreateTraderApi ApiCreateFunc = (funcCreateTraderApi)GetProcAddress(x_handle, XELE_API_CREATE_FUNC_NAME);
	if (!ApiCreateFunc) {
		LOGE("Get function failed: " << XELE_API_CREATE_FUNC_NAME);
		return false;
	}

	userApi = ApiCreateFunc(0);
	if (userApi == NULL)
	{
		LOGE("创建XeleTraderApi 失败");
		return false;
	}
	userApi->RegisterSpi(this);


	return true;
}

void SpiderXeleTdSpi::start()
{
	if (isReady) //已经登录成功了
	{
		return;
	}
	userApi->RegisterFront(trade_url, query_url);
	//userApi->SubscribePrivateTopic(XELE_TERT_RESTART); //艾科还未实现
	userApi->SubscribePublicTopic(XELE_TERT_RESTART);
	LOGI("Xele API Version = " << userApi->GetVersion() <<"," << trade_url << "," << query_url);
	userApi->Init(true); //入参false，只连接查询服务器
}

void SpiderXeleTdSpi::stop()
{
	isReady = false;
	if (userApi)
	{
		userApi->Release();
		userApi->Join();
		userApi = NULL;
	}
}

void SpiderXeleTdSpi::authenticate()
{
	CXeleFtdcAuthenticationInfoField auth_info;
	memset(&auth_info, 0, sizeof(auth_info));
	sprintf_s(auth_info.AuthCode, sizeof(auth_info.AuthCode), "%s", smd->get_account().auth_code);
	sprintf_s(auth_info.AppID, sizeof(auth_info.AppID), "%s", smd->get_account().app_id);
	if (userApi)
	{
		userApi->RegisterAuthentication(&auth_info);
	}
	
}

void SpiderXeleTdSpi::login()
{
	LOGI("XeleTradeDataSession ReqUserLogin.UserProductInfo is: " << smd->get_account().app_id << ", " << smd->get_account().account_id << ", " << smd->get_account().auth_code);

	CXeleFtdcReqUserLoginField login_info;
	memset(&login_info, 0, sizeof(login_info));
	sprintf_s(login_info.AccountID, sizeof(login_info.AccountID), "%s", smd->get_account().account_id);
	sprintf_s(login_info.Password, sizeof(login_info.Password), "%s", smd->get_account().password);

	int ret = userApi->ReqUserLogin(&login_info, 1);
	if (ret != 0)
	{
		LOGW("Warning: XeleTradeDataSession encountered an error when try to login, error no: " << ret);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = ret;
		strncpy(_err->ErrMsg, "UserLogon调用失败", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

// 连接成功回调
void SpiderXeleTdSpi::OnFrontConnected() 
{

	LOGI(smd->get_account().account_id << ", Xele:连接成功");
	smd->on_connected();
	authenticate();
	login();
}

// 连接断开回调
void SpiderXeleTdSpi::OnFrontDisconnected(int nReason) 
{
	LOGE(smd->get_account().account_id << ", Xele:连接中断:" << nReason ); 
	if (isReady)
	{
		isReady = false;
		smd->on_disconnected();

	}

}

// 登陆回调
void SpiderXeleTdSpi::OnRspUserLogin(CXeleFtdcRspUserLoginField *pRspUserLogin,
	CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0) {
			isReady = false;
			LOGW(myAccount.account_id << ", Xele:登录失败: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pRspUserLogin != NULL) {
		isReady = true;
		LOGI(myAccount.account_id << ", Xele:登陆成功，" << pRspUserLogin->AccountID << "，最大单号：" << pRspUserLogin->MaxOrderLocalID << "，其他参数：" << pRspUserLogin->ParticipantID << "," << pRspUserLogin->DataCenterID << "," << pRspUserLogin->PrivateFlowSize << "," << pRspUserLogin->UserFlowSize << ",nRequestID:" << nRequestID);
		//测试环境下，MaxOrderLocalID为空，ParticipantID为空
		if (smd)
		{
			//smd->init_exoid(pRspUserLogin->MaxOrderLocalID);

			smd->on_log_in(); //挪动到查询账户信息返回后再通知
		}
	}
}

void SpiderXeleTdSpi::OnRspUserLogout(CXeleFtdcRspUserLogoutField *pRspUserLogout, CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0) {
			LOGW(myAccount.account_id << ", Xele:登出失败: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}

	if(pRspUserLogout != NULL)
	{
		isReady = false;
		LOGI(myAccount.account_id << ", Xele:登出成功，" << pRspUserLogout->AccountID << "," << pRspUserLogout->ParticipantID);
		if (smd)
		{
			smd->on_log_out();
		}
	}
}

// 错误响应回调
void SpiderXeleTdSpi::OnRspError(CXeleFtdcRspInfoField *pRspInfo, int nRequestID,
	bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		LOGE(myAccount.account_id << ", Xele:收到错误消息," << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = pRspInfo->ErrorID;
		strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

// 报单回调
void SpiderXeleTdSpi::OnRspOrderInsert(CXeleFtdcInputOrderField *pInputOrder,
	CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", Xele:OnRspOrderInsert: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			return;
		}
	}

	if (pInputOrder != NULL)
	{
		LOGI(myAccount.account_id << ", Xele:报单成功，OrderRef：" << pInputOrder->OrderLocalID << "，OrderSysID：" << pInputOrder->OrderSysID << "，Code：" << pInputOrder->InstrumentID << "，[" << pInputOrder->ParticipantID << ":" << pInputOrder->ClientID << ":" << pInputOrder->UserID << "]");
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, pInputOrder->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pInputOrder->OrderLocalID, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pInputOrder->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		order->Direction = get_direction_from_ctp(pInputOrder->Direction);
		order->Offset = get_offset_from_ctp(pInputOrder->CombOffsetFlag[0]);
		order->HedgeFlag = Speculation;
		order->LimitPrice = pInputOrder->LimitPrice;
		order->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
		order->OrderStatus = 0;
		strncpy(order->StatusMsg, "柜台确认", sizeof(order->StatusMsg) - 1);
		strncpy(order->OrderTime, getNowString(), sizeof(order->OrderTime) - 1);
		if (smd)
			smd->on_order_insert(order);
		delete order;
	}
}

// 撤单回调
void SpiderXeleTdSpi::OnRspOrderAction(CXeleFtdcOrderActionField *pOrderAction,
	CXeleFtdcRspInfoField *pRspInfo,
	int nRequestID, bool bIsLast) 
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", Xele:OnRspOrderAction: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);

			return;
		}
	}

	if (pOrderAction != NULL)
	{
		LOGI(myAccount.account_id << ", Xele:撤单成功，OrderRef：" << pOrderAction->OrderLocalID << "，OrderSysID：" << pOrderAction->OrderSysID << "， ActionRef：" << pOrderAction->ActionLocalID << "，[" << pOrderAction->ParticipantID << ":" << pOrderAction->ClientID << ":" << pOrderAction->UserID <<"]");
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		//strncpy(order->Code, pInputOrderAction->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pOrderAction->OrderLocalID, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pOrderAction->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		order->LimitPrice = pOrderAction->LimitPrice;
		order->OrderStatus = (int)EnumOrderStatusType::Canceled;
		if (smd)
			smd->on_order_cancel(order);
		delete order;
	}
}

// 报单回报回调
void SpiderXeleTdSpi::OnRtnOrder(CXeleFtdcOrderField *pOrder) 
{
	if (pOrder != NULL)
	{
		LOGI(myAccount.account_id << ", Xele:委托推送，OrderRef：" << pOrder->OrderLocalID << "，OrderSysID：" << pOrder->OrderSysID << "， Code：" << pOrder->InstrumentID << "， Direction：" << pOrder->Direction << "， OrderStatus：" << pOrder->OrderStatus << "，[" << pOrder->ParticipantID << ":" << pOrder->ClientID << ":" << pOrder->UserID << "]");
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, pOrder->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pOrder->OrderLocalID, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pOrder->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->LimitPrice = pOrder->LimitPrice;
		order->VolumeTotalOriginal = pOrder->VolumeTotalOriginal;
		order->VolumeTraded = pOrder->VolumeTraded;
		order->VolumeTotal = pOrder->VolumeTotal;
		order->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		order->Direction = get_direction_from_ctp(pOrder->Direction);
		order->OrderStatus = get_orderstatus_from_ctp(pOrder->OrderStatus);
		order->HedgeFlag = Speculation;
		order->Offset = get_offset_from_ctp(pOrder->CombOffsetFlag[0]);
		//strncpy(order->StatusMsg, pOrder->StatusMsg, sizeof(order->StatusMsg) - 1);
		sprintf_s(order->OrderTime, sizeof(order->OrderTime) - 1, "%s.000", pOrder->InsertTime);
		if (smd)
			smd->on_order_change(order);
		delete order;
	}
}

///
void SpiderXeleTdSpi::OnRtnTrade(CXeleFtdcTradeField *pTrade)
{
	if (pTrade != NULL)
	{
		LOGI(myAccount.account_id << ", Xele:成交回报，OrderRef：" << pTrade->OrderLocalID << "，OrderSysID：" << pTrade->OrderSysID << "， Code：" << pTrade->InstrumentID << "， Price：" << pTrade->Price << "，Volume：" << pTrade->Volume << "，SequenceNo：" << pTrade->TradeID << "，[" << pTrade->ParticipantID << ":" << pTrade->ClientID << ":" << pTrade->UserID << "]");
		TradeInfo * trade = new TradeInfo();
		memset(trade, 0, sizeof(TradeInfo));
		strncpy(trade->Code, pTrade->InstrumentID, sizeof(trade->Code) - 1);
		strncpy(trade->OrderRef, pTrade->OrderLocalID, sizeof(trade->OrderRef) - 1);
		strncpy(trade->OrderSysID, pTrade->OrderSysID, sizeof(trade->OrderSysID) - 1);
		strncpy(trade->TradeID, pTrade->TradeID, sizeof(trade->TradeID) - 1);
		sprintf_s(trade->TradeTime, sizeof(trade->TradeTime) - 1, "%s.000", pTrade->TradeTime);
		//strncpy(trade->TradeTime, pTrade->TradeTime, sizeof(trade->TradeTime) - 1);
		trade->ExchangeID = (int)EnumExchangeIDType::CFFEX;
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

///
void SpiderXeleTdSpi::OnErrRtnOrderInsert(CXeleFtdcInputOrderField *pInputOrder, CXeleFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", Xele:OnErrRtnOrderInsert: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			//std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			//_err->ErrCode = pRspInfo->ErrorID;
			//strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			//smd->on_error(_err);
			//return;
		}
	}
	if (pInputOrder != NULL)
	{
		LOGW(myAccount.account_id << ", Xele:报单失败，OrderRef：" << pInputOrder->OrderLocalID << "，Code：" << pInputOrder->InstrumentID << "，[" << pInputOrder->ParticipantID << ":" << pInputOrder->ClientID << ":" << pInputOrder->UserID << "]");

		OrderInfo * order = new OrderInfo();
		strncpy(order->Code, pInputOrder->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pInputOrder->OrderLocalID, sizeof(order->OrderRef) - 1);
		order->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		order->Direction = get_direction_from_ctp(pInputOrder->Direction);
		order->Offset = get_offset_from_ctp(pInputOrder->CombOffsetFlag[0]);
		order->HedgeFlag = Speculation;
		order->LimitPrice = pInputOrder->LimitPrice;
		order->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
		order->OrderStatus = -1; //约定好的失败
		strncpy(order->StatusMsg, "报单失败", sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_insert(order);
		delete order;
	}
}

///
void SpiderXeleTdSpi::OnErrRtnOrderAction(CXeleFtdcOrderActionField *pOrderAction, CXeleFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", Xele:OnErrRtnOrderAction: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			//std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			//_err->ErrCode = pRspInfo->ErrorID;
			//strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			//smd->on_error(_err);
			//return;
		}
	}
	if (pOrderAction != NULL)
	{
		LOGW(myAccount.account_id << ", Xele:撤单失败，OrderRef：" << pOrderAction->OrderLocalID << "，OrderSysID：" << pOrderAction->OrderSysID << "， ActionRef：" << pOrderAction->ActionLocalID << "，[" << pOrderAction->ParticipantID << ":" << pOrderAction->ClientID << ":" << pOrderAction->UserID << "]");
		OrderInfo * order = new OrderInfo();
		//strncpy(order->Code, pOrderAction->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pOrderAction->OrderLocalID, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pOrderAction->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		order->LimitPrice = pOrderAction->LimitPrice;
		order->OrderStatus = -1; //约定好的失败
		strncpy(order->StatusMsg, "撤单失败", sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_cancel(order);
		delete order;
	}
}


// 没有持仓数据时不会调用
void SpiderXeleTdSpi::OnRspQryClientPosition(CXeleFtdcRspClientPositionField *pRspClientPosition, CXeleFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", Xele:OnRspQryClientPosition: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pRspClientPosition != NULL)
	{
		LOGI(myAccount.account_id << ", CTP:OnRspQryInvestorPosition: " << pRspClientPosition->InstrumentID << ":" << bIsLast << ":" << pRspClientPosition->LongYdPosition << ":" << pRspClientPosition->LongPosition << ":" << pRspClientPosition->ShortYdPosition << ":" << pRspClientPosition->ShortPosition);
		//多头
		InvestorPosition * pos = new InvestorPosition;
		memset(pos, 0, sizeof(InvestorPosition));
		strncpy(pos->AccountID, pRspClientPosition->AccountID, sizeof(pos->AccountID) - 1);
		strncpy(pos->Code, pRspClientPosition->InstrumentID, sizeof(pos->Code) - 1);
		pos->HedgeFlag = Speculation;
		pos->PosiDirection = (int)EnumPosiDirectionType::Long;
		//pos->OpenCost = pRspClientPosition->OpenCost;
		pos->Position = pRspClientPosition->LongYdPosition;
		//pos->PositionCost = pRspClientPosition->PositionCost;
		//pos->ProfitLoss = pRspClientPosition->PositionProfit;
		//pos->StockAvailable = pInvestorPosition->StockAvailable;
		pos->TodayPosition = pRspClientPosition->LongPosition;
		pos->YesterdayPosition = pos->Position - pos->TodayPosition; //CTP的YdPosition是上一日的快照持仓数量
		if (smd)
		{
			smd->on_rsp_query_position(pos, nRequestID, bIsLast);
		}

		//空头
		InvestorPosition * pos1 = new InvestorPosition;
		memset(pos1, 0, sizeof(InvestorPosition));
		strncpy(pos1->AccountID, pRspClientPosition->AccountID, sizeof(pos1->AccountID) - 1);
		strncpy(pos1->Code, pRspClientPosition->InstrumentID, sizeof(pos1->Code) - 1);
		pos1->HedgeFlag = Speculation;
		pos1->PosiDirection = (int)EnumPosiDirectionType::Short;
		//pos->OpenCost = pRspClientPosition->OpenCost;
		pos1->Position = pRspClientPosition->ShortYdPosition;
		//pos->PositionCost = pRspClientPosition->PositionCost;
		//pos->ProfitLoss = pRspClientPosition->PositionProfit;
		//pos->StockAvailable = pInvestorPosition->StockAvailable;
		pos1->TodayPosition = pRspClientPosition->ShortPosition;
		pos1->YesterdayPosition = pos1->Position - pos1->TodayPosition; //CTP的YdPosition是上一日的快照持仓数量
		if (smd)
		{
			smd->on_rsp_query_position(pos1, nRequestID, bIsLast);
		}
	}
}


// 客户资金查询回调
void SpiderXeleTdSpi::OnRspQryClientAccount(CXeleFtdcRspClientAccountField *pClientAccount, CXeleFtdcRspInfoField *pRspInfo, int nRquestID, bool bIsLast) 
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", Xele:OnRspQryClientAccount: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pRspInfo != NULL)
	{
		LOGI(myAccount.account_id << ", Xele:OnRspQryTradingAccount: " << pClientAccount->Available << ":" << bIsLast);
		TradingAccount * account = new TradingAccount();
		memset(account, 0, sizeof(TradingAccount));
		strncpy(account->AccountID, pClientAccount->AccountID, sizeof(account->AccountID) - 1);
		//strncpy(account->Currency, pClientAccount->CurrencyID, sizeof(account->Currency) - 1);
		strncpy(account->TradingDay, pClientAccount->TradingDay, sizeof(account->TradingDay) - 1);
		account->Available = pClientAccount->Available;
		account->Balance = pClientAccount->Balance;
		account->CloseProfit = pClientAccount->CloseProfit;
		//account->Commission = pClientAccount->Commission;
		account->CurrMargin = pClientAccount->CurrMargin;
		account->Deposit = pClientAccount->Deposit;
		//account->FrozenCash = pClientAccount->FrozenCash;
		//account->FrozenCommission = pClientAccount->FrozenCommission;
		account->FrozenMargin = pClientAccount->FrozenMargin;
		account->PositionProfit = pClientAccount->floatProfitAndLoss;
		account->PreBalance = pClientAccount->PreBalance;
		account->Withdraw = pClientAccount->Withdraw;
		//account->WithdrawQuota = pClientAccount->WithdrawQuota;
		if (smd)
		{
			smd->on_rsp_query_account(account, nRquestID, bIsLast);
		}
	}
}

///没有委托数据时不会调用
void SpiderXeleTdSpi::OnRspQryOrder(CXeleFtdcOrderField* pOrderField, CXeleFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", Xele:OnRspQryOrder: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}

	if (pOrderField != NULL)
	{
		LOGI(myAccount.account_id << ", Xele:OnRspQryOrder: " << pOrderField->InstrumentID << ":" << bIsLast);
		OrderInfo * order = new OrderInfo;
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, pOrderField->InstrumentID, sizeof(order->Code) - 1);
		strncpy(order->OrderRef, pOrderField->OrderLocalID, sizeof(order->OrderRef) - 1);
		strncpy(order->OrderSysID, pOrderField->OrderSysID, sizeof(order->OrderSysID) - 1);
		order->LimitPrice = pOrderField->LimitPrice;
		order->VolumeTotalOriginal = pOrderField->VolumeTotalOriginal;
		order->VolumeTraded = pOrderField->VolumeTraded;
		order->VolumeTotal = pOrderField->VolumeTotal;
		order->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		order->Direction = get_direction_from_ctp(pOrderField->Direction);
		order->OrderStatus = get_orderstatus_from_ctp(pOrderField->OrderStatus);
		order->HedgeFlag = Speculation;
		order->Offset = get_offset_from_ctp(pOrderField->CombOffsetFlag[0]);
		sprintf_s(order->OrderTime, sizeof(order->OrderTime) - 1, "%s.000", pOrderField->InsertTime);
		if (smd)
		{
			smd->on_rsp_query_order(order, nRequestID, bIsLast);
		}
	}
}

///没有成交数据时不会调用
void SpiderXeleTdSpi::OnRspQryTrade(CXeleFtdcTradeField* pTradeField, CXeleFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL)
	{
		if (pRspInfo->ErrorID != 0)
		{
			LOGW(myAccount.account_id << ", Xele:OnRspQryTrade: " << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = pRspInfo->ErrorID;
			strncpy(_err->ErrMsg, pRspInfo->ErrorMsg, sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
			return;
		}
	}
	if (pTradeField != NULL)
	{
		LOGI(myAccount.account_id << ", Xele:OnRspQryTrade: " << pTradeField->InstrumentID << ":" << bIsLast);
		TradeInfo * trade = new TradeInfo;
		memset(trade, 0, sizeof(TradeInfo));
		strncpy(trade->Code, pTradeField->InstrumentID, sizeof(trade->Code) - 1);
		strncpy(trade->OrderRef, pTradeField->OrderLocalID, sizeof(trade->OrderRef) - 1);
		strncpy(trade->OrderSysID, pTradeField->OrderSysID, sizeof(trade->OrderSysID) - 1);
		strncpy(trade->TradeID, pTradeField->TradeID, sizeof(trade->TradeID) - 1);
		//strncpy(trade->TradeTime, pTrade->TradeTime, sizeof(trade->TradeTime) - 1);
		sprintf_s(trade->TradeTime, sizeof(trade->TradeTime) - 1, "%s.000", pTradeField->TradeTime);
		trade->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		trade->Direction = get_direction_from_ctp(pTradeField->Direction);
		trade->HedgeFlag = Speculation;
		trade->Offset = get_offset_from_ctp(pTradeField->OffsetFlag);
		trade->Price = pTradeField->Price;
		trade->Volume = pTradeField->Volume;
		if (smd)
		{
			smd->on_rsp_query_trade(trade, nRequestID, bIsLast);
		}
	}
}



//====================================================================================================================================


SpiderXeleTdSession::SpiderXeleTdSession(SpiderCommonApi * sci, AccountInfo & ai)
	: exoid(0)
	, BaseTradeSession(sci, ai)
{
	tradeConnection.reset(new SpiderXeleTdSpi());
}

SpiderXeleTdSession::~SpiderXeleTdSession()
{

}

bool SpiderXeleTdSession::init()
{
	return tradeConnection->init(this);
}

void SpiderXeleTdSession::start()
{
	tradeConnection->start();
}

void SpiderXeleTdSession::stop()
{
	tradeConnection->stop();
}

const char * SpiderXeleTdSession::insert_order(OrderInsert * order)
{
	//char * _order_ref = new char[13];
	//memset(_order_ref, 0, sizeof(_order_ref));
	const char * _order_ref = get_order_ref(order->OrderRef);

	CXeleFtdcInputOrderField field;
	memset(&field, 0, sizeof(field));


	//sprintf_s(field.OrderLocalID, sizeof(field.OrderLocalID), "%d", get_real_order_ref(atoi(order->OrderRef)));
	strncpy(field.OrderLocalID, _order_ref, sizeof(field.OrderLocalID) - 1);

	//返回值
	//sprintf_s(_order_ref, 13, "%s", field.OrderLocalID);
	//delete[] _order_ref; //要在使用的地方删除掉,api使用者自己释放

	//strncpy(field.InvestorID, get_account().account_id, sizeof(field.InvestorID) - 1);
	//strncpy(field.UserID, get_account().account_id, sizeof(field.UserID) - 1);
	strncpy(field.ParticipantID, get_account().front_id, sizeof(field.ParticipantID) - 1);
	strncpy(field.ClientID, get_account().broker_id, sizeof(field.ClientID) - 1);
	strncpy(field.InstrumentID, order->Code, sizeof(field.InstrumentID) - 1);
	//strncpy(field.ExchangeID, get_exid_to_ctp((EnumExchangeIDType)order->ExchangeID), sizeof(field.ExchangeID) - 1);
	field.OrderPriceType = XELE_FTDC_OPT_LimitPrice;
	field.Direction = order->Direction == EnumDirectionType::Buy ? XELE_FTDC_D_Buy : XELE_FTDC_D_Sell;
	switch (order->Offset)
	{
	case EnumOffsetFlagType::Open:
	{
		field.CombOffsetFlag[0] = XELE_FTDC_OF_Open;
		break;
	}
	case EnumOffsetFlagType::Close:
	{
		if (order->ExchangeID == EnumExchangeIDType::SHFE)
		{
			field.CombOffsetFlag[0] = XELE_FTDC_OF_CloseToday;
		}
		else {
			field.CombOffsetFlag[0] = XELE_FTDC_OF_Close;
		}
		break;
	}
	case EnumOffsetFlagType::CloseToday:
	{
		field.CombOffsetFlag[0] = XELE_FTDC_OF_CloseToday;
		break;
	}
	case EnumOffsetFlagType::CloseYesterday:
	{
		field.CombOffsetFlag[0] = XELE_FTDC_OF_CloseYesterday;
		break;
	}
	default:
	{
		LOGE("错误的开平方向：" << order->Offset);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "错误的开平方向", sizeof(_err->ErrMsg) - 1);
		on_error(_err);

		OrderInfo * failorder = new OrderInfo();
		memset(failorder, 0, sizeof(OrderInfo));
		strncpy(failorder->Code, order->Code, sizeof(failorder->Code) - 1);
		strncpy(failorder->OrderRef, _order_ref, sizeof(failorder->OrderRef) - 1);
		failorder->ExchangeID = order->ExchangeID;
		failorder->Direction = order->Direction;
		failorder->Offset = order->Offset;
		failorder->HedgeFlag = order->HedgeFlag;
		failorder->LimitPrice = order->LimitPrice;
		failorder->VolumeTotalOriginal = order->VolumeTotalOriginal;
		failorder->OrderStatus = -1; //约定好的失败
		strncpy(failorder->StatusMsg, "错误的开平方向", sizeof(failorder->StatusMsg) - 1);
		on_order_insert(failorder);

		return _order_ref;
	}
	}
	field.CombOffsetFlag[1] = 0;
	strcpy(field.CombHedgeFlag, "1");// 组合投机套保标志
	field.LimitPrice = order->LimitPrice;// 价格
	field.VolumeTotalOriginal = order->VolumeTotalOriginal;// 数量
	field.TimeCondition = XELE_FTDC_TC_GFD;// 有效期类型
	strcpy(field.GTDDate, "");// GTD日期
	field.VolumeCondition = XELE_FTDC_VC_AV;// 成交量类型
	field.MinVolume = 0;// 最小成交量
	field.ContingentCondition = XELE_FTDC_CC_Immediately;// 触发条件
	field.StopPrice = 0;// 止损价
	field.ForceCloseReason = XELE_FTDC_FCC_NotForceClose;// 强平原因
	field.IsAutoSuspend = 0;// 自动挂起标志

	if (!tradeConnection->ready())
	{
		LOGW("Xele EnterOrder, 服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);

		OrderInfo * failorder = new OrderInfo();
		memset(failorder, 0, sizeof(OrderInfo));
		strncpy(failorder->Code, order->Code, sizeof(failorder->Code) - 1);
		strncpy(failorder->OrderRef, _order_ref, sizeof(failorder->OrderRef) - 1);
		failorder->ExchangeID = order->ExchangeID;
		failorder->Direction = order->Direction;
		failorder->Offset = order->Offset;
		failorder->HedgeFlag = order->HedgeFlag;
		failorder->LimitPrice = order->LimitPrice;
		failorder->VolumeTotalOriginal = order->VolumeTotalOriginal;
		failorder->OrderStatus = -1; //约定好的失败
		strncpy(failorder->StatusMsg, "服务器尚未登录", sizeof(failorder->StatusMsg) - 1);	
		on_order_insert(failorder);

		return _order_ref;
	}

	int r = tradeConnection->getUserApi()->ReqOrderInsert(&field, get_seq());
	if (r != 0)
	{
		LOGW("Xele EnterOrder, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "EnterOrder调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);

		OrderInfo * failorder = new OrderInfo();
		memset(failorder, 0, sizeof(OrderInfo));
		strncpy(failorder->Code, order->Code, sizeof(failorder->Code) - 1);
		strncpy(failorder->OrderRef, _order_ref, sizeof(failorder->OrderRef) - 1);
		failorder->ExchangeID = order->ExchangeID;
		failorder->Direction = order->Direction;
		failorder->Offset = order->Offset;
		failorder->HedgeFlag = order->HedgeFlag;
		failorder->LimitPrice = order->LimitPrice;
		failorder->VolumeTotalOriginal = order->VolumeTotalOriginal;
		failorder->OrderStatus = -1; //约定好的失败
		strncpy(failorder->StatusMsg, "EnterOrder调用失败", sizeof(failorder->StatusMsg) - 1);
		on_order_insert(failorder);

	}
	LOGI(get_account().account_id << "," << get_account().broker_id << ":Xele EnterOrder: " << _order_ref);
	return _order_ref;
}

void SpiderXeleTdSession::cancel_order(OrderCancel * order)
{

	if (!tradeConnection->ready())
	{
		LOGW("Xele Withdraw, 服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	CXeleFtdcOrderActionField action;
	memset(&action, 0, sizeof(action));
	strncpy(action.ParticipantID, get_account().front_id, sizeof(action.ParticipantID) - 1);
	strncpy(action.ClientID, get_account().broker_id, sizeof(action.ClientID) - 1);
	strncpy(action.OrderSysID, order->OrderSysID, sizeof(TXeleFtdcOrderSysIDType));
	strncpy(action.OrderLocalID, order->OrderRef, sizeof(TXeleFtdcOrderLocalIDType));
	action.ActionFlag = 0x30;

	LOGI(get_account().account_id <<","<< get_account().broker_id << ":Xele Withdraw: " << action.OrderSysID);

	int r = tradeConnection->getUserApi()->ReqOrderAction(&action, get_seq());
	if (r != 0)
	{
		LOGW("Xele CancelOrder, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "CancelOrder调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}

void SpiderXeleTdSession::query_trading_account(int request_id)
{
	if (!tradeConnection->ready())
	{
		LOGW("Xele QueryAccount, 服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_account_info.find(request_id);
		if (it != cache_account_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " Xele 上一次查询账户资金尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户资金尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	CXeleFtdcQryClientAccountField qryAccount;
	memset(&qryAccount, 0, sizeof(CXeleFtdcQryClientAccountField));
	snprintf(qryAccount.AccountID, sizeof(qryAccount.AccountID), "%s", get_account().account_id);
	LOGI(get_account().account_id << "," << get_account().broker_account_id << ":Xele query clients trading_account, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->ReqQryClientAccount(&qryAccount, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ": Xele failed to query trading_account, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccount调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}

}
void SpiderXeleTdSession::query_positions(int request_id)
{
	if (!tradeConnection->ready())
	{
		LOGW("Xele QueryAccountPosition, 服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_positions_info.find(request_id);
		if (it != cache_positions_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " Xele 上一次查询账户持仓尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户持仓尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	CXeleFtdcQryClientPositionField qryPosition;
	memset(&qryPosition, 0, sizeof(CXeleFtdcQryClientPositionField));
	snprintf(qryPosition.AccountID, sizeof(qryPosition.AccountID), "%s", get_account().account_id);
	LOGI(get_account().account_id << ":Xele query clients positions, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->ReqQryClientPosition(&qryPosition, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: Xele failed to query position, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountPosition调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}
void SpiderXeleTdSession::query_orders(int request_id)
{
	if (!tradeConnection->ready())
	{
		LOGW("Xele QueryAccountOrder, 服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_orders_info.find(request_id);
		if (it != cache_orders_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " Xele 上一次查询账户委托尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户委托尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	CXeleFtdcQryOrderField qryOrder;
	memset(&qryOrder, 0, sizeof(CXeleFtdcQryOrderField));
	snprintf(qryOrder.AccountID, sizeof(qryOrder.AccountID), "%s", get_account().account_id);
	LOGI(get_account().account_id << ":Xele query clients orders, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->ReqQryOrder(&qryOrder, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Xele failed to query orders, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountOrder调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}

void SpiderXeleTdSession::query_trades(int request_id)
{
	if (!tradeConnection->ready())
	{
		LOGW("Xele QueryAccountOrderExecution, 服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_trades_info.find(request_id);
		if (it != cache_trades_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " Xele 上一次查询账户成交尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户成交尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}


	CXeleFtdcQryTradeField qryTrade;
	memset(&qryTrade, 0, sizeof(CXeleFtdcQryTradeField));
	snprintf(qryTrade.AccountID, sizeof(qryTrade.AccountID), "%s", get_account().account_id);
	LOGI(get_account().account_id << ":Xele query clients trades, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->ReqQryTrade(&qryTrade, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Xele failed to query trades, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountTrade调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}

void SpiderXeleTdSession::on_connected()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnConnect();
		}
	}
}

void SpiderXeleTdSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnDisconnect();
		}
	}
}

void SpiderXeleTdSession::on_log_in()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderXeleTdSession::on_log_out()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnLogout();
		}
	}
}

void SpiderXeleTdSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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


void SpiderXeleTdSession::on_order_change(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeRtnOrder(order);
		}
	}
}
void SpiderXeleTdSession::on_trade_change(TradeInfo * trade)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeRtnTrade(trade);
		}
	}
}
void SpiderXeleTdSession::on_order_cancel(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnOrderCancel(order);
		}
	}
}
void SpiderXeleTdSession::on_order_insert(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnOrderInsert(order);
		}
	}
}

void SpiderXeleTdSession::on_rsp_query_account(TradingAccount * account, int request_id, bool last)
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
			cache_account_info.insert(std::make_pair(request_id, tmp));
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
void SpiderXeleTdSession::on_rsp_query_position(InvestorPosition * position, int request_id, bool last)
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
void SpiderXeleTdSession::on_rsp_query_trade(TradeInfo * trade, int request_id, bool last)
{
	TradeInfo* * send_list = NULL;
	int send_count = 0;
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		if (trade) //入参trade可能是NULL
		{
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
		}
		if (last)
		{
			auto sendit = cache_trades_info.find(request_id);
			if (sendit != cache_trades_info.end()) //cache_trades_info 可能找不到记录
			{
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
void SpiderXeleTdSession::on_rsp_query_order(OrderInfo * order, int request_id, bool last)
{
	OrderInfo* * send_list = NULL;
	int send_count = 0;
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		if (order) //入参order可能是NULL
		{
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
		}

		if (last)
		{
			auto sendit = cache_orders_info.find(request_id);
			if (sendit != cache_orders_info.end()) //cache_orders_info 可能找不到记录
			{
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

void SpiderXeleTdSession::init_exoid(const char * c_exoid)
{
	int max_exoid = atoi(c_exoid);
	if (max_exoid > 0 && max_exoid < INT_MAX)
	{
		std::unique_lock<std::mutex> l(exoid_mutex);
		exoid = max_exoid / 1000;
	}
}
int SpiderXeleTdSession::get_real_order_ref(int order_ref)
{
	if (order_ref >= 1000) //和C#策略那边约定好了，他只负责生成3位的委托编号
	{
		order_ref = order_ref % 1000;
	}
	std::unique_lock<std::mutex> l(exoid_mutex);
	int real_order_ref = (++exoid) * 1000 + order_ref;
	return real_order_ref;
}
