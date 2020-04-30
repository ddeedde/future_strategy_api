#include "EesTdApi.h"
#include "LogWrapper.h"
#include "FutureApi.h"
#include "CommonDefine.h"

SpiderEesTdSpi::SpiderEesTdSpi()
	: userApi(NULL)
	, smd(NULL)
	, isReady(false)
{

}

SpiderEesTdSpi::~SpiderEesTdSpi()
{

}


bool SpiderEesTdSpi::init(SpiderEesTdSession * sm)
{
	smd = sm;

	myAccount = sm->get_account();

	if (strlen(myAccount.uri_list[0]) > 0)
	{
		LOGI("Info: Ees TCP Trade Server: " << myAccount.uri_list[0]);
		std::vector<std::string> ips;
		split_str(myAccount.uri_list[0], ips, ":");

		if (ips.size() == 5) //tcp://127.0.0.1:31001:127.0.0.1:31002 //普通版，分别是交易地址和查询地址
		{
			replace_all(ips[1], "/", "");

			strncpy(myServer.m_remoteTradeIp, ips[1].c_str(), sizeof(myServer.m_remoteTradeIp) - 1);
			myServer.m_remoteTradeTCPPort = (unsigned short)atoi(ips[2].c_str());
			strncpy(myServer.m_remoteQueryIp, ips[3].c_str(), sizeof(myServer.m_remoteQueryIp) - 1);
			myServer.m_remoteQueryTCPPort = (unsigned short)atoi(ips[4].c_str());

		}
		else if (ips.size() == 8) //fast://127.0.0.1:31001:31001:127.0.0.1:31002:0.0.0.0:65000 //极致版，分别是交易服务器ip，TCP端口号，UDP端口号，查询服务器地址，本地地址
		{
			replace_all(ips[1], "/", "");

			strncpy(myServer.m_remoteTradeIp, ips[1].c_str(), sizeof(myServer.m_remoteTradeIp) - 1);
			myServer.m_remoteTradeTCPPort = (unsigned short)atoi(ips[2].c_str());
			myServer.m_remoteTradeUDPPort = (unsigned short)atoi(ips[3].c_str());
			strncpy(myServer.m_remoteQueryIp, ips[4].c_str(), sizeof(myServer.m_remoteQueryIp) - 1);
			myServer.m_remoteQueryTCPPort = (unsigned short)atoi(ips[5].c_str());
			strncpy(myServer.m_LocalTradeIp, ips[6].c_str(), sizeof(myServer.m_LocalTradeIp) - 1);
			myServer.m_LocalTradeUDPPort = (unsigned short)atoi(ips[7].c_str());
		}
		else {
			LOGE("Ees 配置文件中交易地址格式错误");
			return false;
		}
	}
	else {
		LOGE("配置文件中服务器地址不能为空");
		return false;
	}
	
	if (strlen(myServer.m_remoteTradeIp) <= 0)
	{
		LOGE("Ees 配置文件中交易地址为空");
		return false;
	}

	userApi = CreateEESTraderApi();
	if (userApi == NULL)
	{
		LOGE("创建EES EESTraderApi 失败");
		return false;
	}
	userApi->SetLoggerSwitch(true); //开启本地日志

	independent_thread.reset(new std::thread(std::bind(&SpiderEesTdSpi::process_task,this)));

	return true;
}

void SpiderEesTdSpi::add_task(async_task _t)
{
	std::unique_lock<std::mutex> l(task_mutex);
	task_queue.push_back(_t);
}
void SpiderEesTdSpi::process_task()
{
	while (userApi)
	{
		std::this_thread::sleep_for(std:: chrono::milliseconds(500));	
		async_task _task;
		{
			std::unique_lock<std::mutex> l(task_mutex);
			if (task_queue.empty())
			{
				continue;
			}
			_task = task_queue.front();
			task_queue.pop_front();
		}
		switch (_task.task_id)
		{
		case async_task_login:
		{
			LOGI(myAccount.account_id << ", EES:开始重新连接");
			if (userApi)
			{
				userApi->DisConnServer();
			}
			start();
			break;
		}
		case async_task_send_order_fail:
		{
			LOGI(myAccount.account_id << ", EES:发送直接委托失败");
			if (smd && _task.task_data)
			{
				smd->on_order_insert((OrderInfo*)_task.task_data);
			};
			if (_task.task_data)
			{
				delete _task.task_data;
				_task.task_data = NULL;
			}
			break;
		}
		default:
			break;
		}
	}
}

void SpiderEesTdSpi::start()
{
	if (isReady) //已经登录成功了
	{
		return;
	}
	int ret = 0;
	if (myServer.m_remoteTradeUDPPort != 0) //极致版
	{
		ret = userApi->ConnServer(myServer, this);
	}
	else {
		ret = userApi->ConnServer(myServer.m_remoteTradeIp,myServer.m_remoteTradeTCPPort, this, myServer.m_remoteQueryIp, myServer.m_remoteQueryTCPPort);
	}
	if (ret != 0)
	{
		LOGE("连接盛利交易和查询服务器失败");
	}
}

void SpiderEesTdSpi::stop()
{
	isReady = false;
	if (userApi)
	{
		userApi->LoggerFlush();
		userApi->DisConnServer();
		DestroyEESTraderApi(userApi);
		userApi = NULL;
	}
	if (independent_thread.get())
	{
		independent_thread->join();
	}
}

void SpiderEesTdSpi::login()
{
	LOGI("EESTradeDataSession ReqUserLogin.UserProductInfo is: " << smd->get_account().app_id << ", " << smd->get_account().account_id << ", " << smd->get_account().auth_code);

	int r = userApi->UserLogon(smd->get_account().account_id, smd->get_account().password, smd->get_account().app_id, smd->get_account().auth_code);
	if (r != 0)
	{
		LOGW("Warning: EESTradeDataSession encountered an error when try to login, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "UserLogon调用失败", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

///	\brief	服务器连接事件
///	\param  errNo                   连接成功能与否的消息
///	\param  pErrStr                 错误信息
///	\return void  
void SpiderEesTdSpi::OnConnection(ERR_NO errNo, const char* pErrStr)
{
	LOGI(smd->get_account().account_id << ", EES:连接成功:"<< errNo <<", "<< pErrStr);
	smd->on_connected();
	login();
}

/// 连接断开消息的回调

/// \brief	服务器主动断开，会收到这个消息
/// \param  ERR_NO errNo         连接成功能与否的消息
/// \param  const char* pErrStr  错误信息
/// \return void  
void SpiderEesTdSpi::OnDisConnection(ERR_NO errNo, const char* pErrStr)
{
	LOGE(smd->get_account().account_id << ", EES:连接中断:" << errNo << ", " << pErrStr); //因为盛立接口中没有心跳，所以该回调不靠谱，应该在每个接口调用的返回值中判断是否断连，然后直接重连
	if (isReady)
	{
		isReady = false;
		smd->on_disconnected();
		//盛立接口不会自动重连，手动重连	
		add_task(async_task(async_task_login, NULL));
	}
}

/// 登录消息的回调

/// \param  pLogon                  登录成功或是失败的结构
/// \return void 
void SpiderEesTdSpi::OnUserLogon(EES_LogonResponse* pLogon)
{
	if (pLogon)
	{
		if (pLogon->m_Result != 0)
		{
			isReady = false;
			LOGW(myAccount.account_id << ", EES:登录失败: " << pLogon->m_Result);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "EES登录失败", sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
		}
		else {
			isReady = true;
			LOGI(myAccount.account_id << ", EES:登陆成功，"<< pLogon->m_UserId <<"，最大单号：" << pLogon->m_MaxToken << "，流控参数：" << pLogon->m_OrderFCCount << "," << pLogon->m_OrderFCInterval << "," << pLogon->m_CancelFCCount << "," << pLogon->m_CancelFCInterval);
			memset(myAccount.account_id, 0, sizeof(myAccount.account_id));
			sprintf_s(myAccount.account_id, sizeof(myAccount.account_id) - 1, "%d", pLogon->m_UserId); //将account_id转换为user_id
			LOGI(smd->get_account().account_id << ",转换为："<< myAccount.account_id);
			userApi->QueryUserAccount(); //有可能结果还没返回，客户就开始查询或下单了
			if (smd)
			{
				smd->init_exoid((int)pLogon->m_MaxToken);
				//查询真正的资金账户，因为委托，撤单，查询都是用的真正的资金账户ID
				//smd->on_log_in(); //挪动到查询账户信息返回后再通知
			}
		}
	}
}

/// 查询用户下面帐户的返回事件

/// \param  pAccountInfo	        帐户的信息
/// \param  bFinish	                如果没有传输完成，这个值是 false ，如果完成了，那个这个值为 true 
/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pAccountInfo值无效。
/// \return void 
void SpiderEesTdSpi::OnQueryUserAccount(EES_AccountInfo * pAccoutnInfo, bool bFinish)
{
	//根据userid查询对应的accountid，因为盛立可以对一个资金账户，设置多个子用户（userid），各个用户之间同享所有资金和持仓
	if (pAccoutnInfo)
	{
		//例如 66621197 和 6662119701 这两个子账户，它们的m_Account都是66621197，但是他们的m_UserId分别是：206和209。
		LOGD(myAccount.account_id << ", EES:查询用户账户，"<< pAccoutnInfo->m_Account << "," << pAccoutnInfo->m_InitialBp << "," << pAccoutnInfo->m_AvailableBp << "," << pAccoutnInfo->m_Margin << "," << pAccoutnInfo->m_CommissionFee << "," << bFinish);
		if (strlen(pAccoutnInfo->m_Account) > 0)
		{
			memset(myAccount.broker_account_id, 0, sizeof(myAccount.broker_account_id));
			memcpy(myAccount.broker_account_id, pAccoutnInfo->m_Account, sizeof(myAccount.broker_account_id) - 1);
			memset(smd->get_account().broker_account_id, 0, sizeof(smd->get_account().broker_account_id));
			memcpy(smd->get_account().broker_account_id, pAccoutnInfo->m_Account, sizeof(smd->get_account().broker_account_id) - 1);
		}
		if (bFinish)
		{
			smd->on_log_in();
		}
	}
	else {
		LOGE("EES:查询用户账户失败，pAccoutnInfo为空");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "EES用户账户失败", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

/// 查询帐户下面期货仓位信息的返回事件	
/// \param  pAccount	                帐户ID 	
/// \param  pAccoutnPosition	        帐户的仓位信息					   
/// \param  nReqId		                发送请求消息时候的ID号。
/// \param  bFinish	                    如果没有传输完成，这个值是false，如果完成了，那个这个值为 true 
/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pAccountInfo值无效。
/// \return void 	
void SpiderEesTdSpi::OnQueryAccountPosition(const char* pAccount, EES_AccountPosition* pAccoutnPosition, int nReqId, bool bFinish)
{
	if (pAccoutnPosition)
	{
		LOGD(myAccount.account_id << ", EES:查询持仓，" << pAccount << "," << pAccoutnPosition->m_Symbol << "," << pAccoutnPosition->m_PosiDirection << "," << pAccoutnPosition->m_TodayQty<<","<< pAccoutnPosition->m_OvnQty);
		InvestorPosition * pos = new InvestorPosition;
		memset(pos, 0, sizeof(InvestorPosition));
		strncpy(pos->AccountID, pAccount, sizeof(pos->AccountID) - 1);
		strncpy(pos->Code, pAccoutnPosition->m_Symbol, sizeof(pos->Code) - 1);
		pos->HedgeFlag = Speculation;
		pos->PosiDirection = get_position_direct_from_ees(pAccoutnPosition->m_PosiDirection);
		pos->OpenCost = 0;
		pos->Position = pAccoutnPosition->m_TodayQty + pAccoutnPosition->m_OvnQty;
		pos->PositionCost = pAccoutnPosition->m_PositionCost;
		pos->ProfitLoss = 0;
		//pos->StockAvailable = pInvestorPosition->StockAvailable;
		pos->TodayPosition = pAccoutnPosition->m_TodayQty;
		pos->YesterdayPosition = pAccoutnPosition->m_OvnQty; 
		if (smd)
		{
			smd->on_rsp_query_position(pos, nReqId, bFinish);
		}
	}
}


/// 查询帐户下面资金信息的返回事件
/// \param  pAccount	                帐户ID 	
/// \param  pAccoutnPosition	        帐户的仓位信息					   
/// \param  nReqId		                发送请求消息时候的ID号
/// \return void 
void SpiderEesTdSpi::OnQueryAccountBP(const char* pAccount, EES_AccountBP* pAccoutnPosition, int nReqId)
{
	if (pAccoutnPosition)
	{
		LOGD(myAccount.account_id << ", EES:查询资金，"<< pAccount<<","<< pAccoutnPosition->m_AvailableBp<<","<<pAccoutnPosition->m_InitialBp<<","<< pAccoutnPosition->m_Margin);
		TradingAccount * account = new TradingAccount();
		memset(account, 0, sizeof(TradingAccount));
		strncpy(account->AccountID, pAccount, sizeof(account->AccountID) - 1);
		strncpy(account->TradingDay, getTodayString(), sizeof(account->TradingDay) - 1);
		account->Available = pAccoutnPosition->m_AvailableBp;
		account->Balance = pAccoutnPosition->m_InitialBp;
		account->CloseProfit = pAccoutnPosition->m_TotalLiquidPL;
		account->Commission = pAccoutnPosition->m_CommissionFee;
		account->CurrMargin = pAccoutnPosition->m_Margin;
		account->FrozenCommission = pAccoutnPosition->m_FrozenCommission;
		account->FrozenMargin = pAccoutnPosition->m_FrozenMargin;
		account->PositionProfit = pAccoutnPosition->m_TotalMarketPL;

		if (smd)
		{
			smd->on_rsp_query_account(account, nReqId, true);
		}
	}
}

/// 下单被柜台系统接受的事件
/// \brief 表示这个订单已经被柜台系统正式的接受
/// \param  pAccept	                    订单被接受以后的消息体
/// \return void 
void SpiderEesTdSpi::OnOrderAccept(EES_OrderAcceptField* pAccept)
{
	if (pAccept)
	{
		LOGI(myAccount.account_id << ", EES:柜台确认：" << pAccept->m_UserID <<","<< pAccept->m_Account<<"," << pAccept->m_ClientOrderToken << "," << pAccept->m_MarketOrderToken << "," << pAccept->m_OrderState);
		if (strcmp(myAccount.account_id,std::to_string(pAccept->m_UserID).c_str()) != 0)
		{
			LOGW("EES收到非本用户的OrderAccept："<< myAccount.account_id<<","<< pAccept->m_UserID<<","<< pAccept->m_Account);
			return;
		}
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			smd->update_order_accept_tm(pAccept->m_ClientOrderToken, pAccept->m_AcceptTime);
			smd->insert_market_token(pAccept->m_ClientOrderToken, pAccept->m_MarketOrderToken);			
			_ees_order = smd->get_ees_order(pAccept->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("逻辑错误：EES OnOrderAccept 找不到委托信息："<< pAccept->m_ClientOrderToken);
			return;
		}
		
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, _ees_order->symbol.c_str(), sizeof(order->Code) - 1);
		sprintf_s(order->OrderRef, sizeof(order->OrderRef) - 1,"%d",_ees_order->client_token);
		sprintf_s(order->OrderSysID, sizeof(order->OrderSysID) - 1, "%lld", _ees_order->market_token);
		order->LimitPrice = _ees_order->order_px;
		order->VolumeTotalOriginal = _ees_order->order_qty;
		order->VolumeTraded = 0;
		order->VolumeTotal = 0;
		order->ExchangeID = _ees_order->ex;
		order->Direction = _ees_order->direct;
		order->OrderStatus = 0;
		order->HedgeFlag = _ees_order->hedge;
		order->Offset = _ees_order->offset;
		strncpy(order->StatusMsg, "柜台确认", sizeof(order->StatusMsg) - 1);
		strncpy(order->OrderTime, smd->get_the_time(pAccept->m_AcceptTime), sizeof(order->OrderTime) - 1);
		if (smd)
			smd->on_order_change(order);
		delete order;
	}
}


/// 下单被市场接受的事件
/// \brief 表示这个订单已经被交易所正式的接受
/// \param  pAccept	                    订单被接受以后的消息体，里面包含了市场订单ID
/// \return void 
void SpiderEesTdSpi::OnOrderMarketAccept(EES_OrderMarketAcceptField* pAccept)
{
	if (pAccept)
	{
		LOGI(myAccount.account_id << ", EES:交易所确认：" << pAccept->m_UserID << "," << pAccept->m_Account << "," << pAccept->m_ClientOrderToken << "," << pAccept->m_MarketOrderToken << "," << pAccept->m_MarketOrderId);
		if (strcmp(myAccount.account_id, std::to_string(pAccept->m_UserID).c_str()) != 0)
		{
			LOGW("EES收到非本用户的OrderMarketAccept：" << myAccount.account_id << "," << pAccept->m_UserID << "," << pAccept->m_Account);
			return;
		}
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			smd->insert_market_token(pAccept->m_ClientOrderToken, pAccept->m_MarketOrderToken);
			_ees_order = smd->get_ees_order(pAccept->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("逻辑错误：EES OnOrderMarketAccept 找不到委托信息：" << pAccept->m_ClientOrderToken);
			return;
		}

		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, _ees_order->symbol.c_str(), sizeof(order->Code) - 1);
		sprintf_s(order->OrderRef, sizeof(order->OrderRef) - 1, "%d", _ees_order->client_token);
		sprintf_s(order->OrderSysID, sizeof(order->OrderSysID) - 1, "%lld", _ees_order->market_token);
		order->LimitPrice = _ees_order->order_px;
		order->VolumeTotalOriginal = _ees_order->order_qty;
		order->VolumeTraded = 0;
		order->VolumeTotal = 0;
		order->ExchangeID = _ees_order->ex;
		order->Direction = _ees_order->direct;
		order->OrderStatus = (int)EnumOrderStatusType::NoTradeQueueing;
		order->HedgeFlag = _ees_order->hedge;
		order->Offset = _ees_order->offset;
		strncpy(order->StatusMsg, "交易所确认", sizeof(order->StatusMsg) - 1);
		strncpy(order->OrderTime, smd->get_the_time(_ees_order->order_time), sizeof(order->OrderTime) - 1);
		if (smd)
			smd->on_order_change(order);
		delete order;
	}
}


///	下单被柜台系统拒绝的事件
/// \brief	订单被柜台系统拒绝，可以查看语法检查或是风控检查。 
/// \param  pReject	                    订单被接受以后的消息体
/// \return void 
void SpiderEesTdSpi::OnOrderReject(EES_OrderRejectField* pReject)
{
	if (pReject)
	{
		LOGI(myAccount.account_id << ", EES:委托被柜台拒绝：" << pReject->m_Userid <<","<< pReject->m_ClientOrderToken << "," << pReject->m_ReasonCode << "," << pReject->m_GrammerResult << "," << pReject->m_RiskResult << "," << pReject->m_GrammerText << "," << pReject->m_RiskText);
		if (strcmp(myAccount.account_id, std::to_string(pReject->m_Userid).c_str()) != 0)
		{
			LOGW("EES收到非本用户的OrderReject：" << myAccount.account_id << "," << pReject->m_Userid);
			return;
		}
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pReject->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("逻辑错误：EES OnOrderReject 找不到委托信息：" << pReject->m_ClientOrderToken);
			return;
		}
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, _ees_order->symbol.c_str(), sizeof(order->Code) - 1);
		sprintf_s(order->OrderRef, sizeof(order->OrderRef) - 1, "%d", _ees_order->client_token);
		order->ExchangeID = _ees_order->ex;
		order->Direction = _ees_order->direct;
		order->Offset = _ees_order->offset;
		order->HedgeFlag = _ees_order->hedge;
		order->LimitPrice = _ees_order->order_px;
		order->VolumeTotalOriginal = _ees_order->order_qty;
		order->OrderStatus = -1; //约定好的失败
		strncpy(order->StatusMsg, "委托被柜台拒绝", sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_insert(order);
		delete order;
	}
}


///	下单被市场拒绝的事件
/// \brief	订单被市场拒绝，可以查看语法检查或是风控检查。 
/// \param  pReject	                    订单被接受以后的消息体，里面包含了市场订单ID
/// \return void 
void SpiderEesTdSpi::OnOrderMarketReject(EES_OrderMarketRejectField* pReject)
{
	if (pReject)
	{
		LOGI(myAccount.account_id << ", EES:委托被交易所拒绝：" << pReject->m_UserID << "," << pReject->m_Account << "," << pReject->m_ClientOrderToken << "," << pReject->m_MarketOrderToken << "," << pReject->m_ReasonText);
		if (strcmp(myAccount.account_id, std::to_string(pReject->m_UserID).c_str()) != 0)
		{
			LOGW("EES收到非本用户的OrderMarketReject：" << myAccount.account_id << "," << pReject->m_UserID << "," << pReject->m_Account);
			return;
		}
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pReject->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("逻辑错误：EES OnOrderMarketReject 找不到委托信息：" << pReject->m_ClientOrderToken);
			return;
		}
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, _ees_order->symbol.c_str(), sizeof(order->Code) - 1);
		sprintf_s(order->OrderRef, sizeof(order->OrderRef) - 1, "%d", _ees_order->client_token);
		order->ExchangeID = _ees_order->ex;
		order->Direction = _ees_order->direct;
		order->Offset = _ees_order->offset;
		order->HedgeFlag = _ees_order->hedge;
		order->LimitPrice = _ees_order->order_px;
		order->VolumeTotalOriginal = _ees_order->order_qty;
		order->OrderStatus = -1; //约定好的失败
		strncpy(order->StatusMsg, pReject->m_ReasonText, sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_insert(order);
		delete order;
	}
}


///	订单成交的消息事件
/// \brief	成交里面包括了订单市场ID，建议用这个ID查询对应的订单
/// \param  pExec	                   订单被接受以后的消息体，里面包含了市场订单ID
/// \return void 
void SpiderEesTdSpi::OnOrderExecution(EES_OrderExecutionField* pExec)
{
	if (pExec)
	{
		LOGI(myAccount.account_id << ", EES:成交回报：" << pExec->m_Userid <<","<< pExec->m_ClientOrderToken<< "," << pExec->m_MarketOrderToken << "," << pExec->m_Quantity << "," << pExec->m_Price << "," << pExec->m_ExecutionID << "," << pExec->m_MarketExecID);
		if (strcmp(myAccount.account_id, std::to_string(pExec->m_Userid).c_str()) != 0)
		{
			LOGW("EES收到非本用户的OrderExecution：" << myAccount.account_id << "," << pExec->m_Userid);
			return;
		}
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			smd->update_deal_qty(pExec->m_ClientOrderToken, pExec->m_Quantity);
			_ees_order = smd->get_ees_order(pExec->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("逻辑错误：EES OnOrderExecution 找不到委托信息：" << pExec->m_ClientOrderToken);
			return;
		}

		TradeInfo * trade = new TradeInfo();
		memset(trade, 0, sizeof(TradeInfo));
		strncpy(trade->Code, _ees_order->symbol.c_str(), sizeof(trade->Code) - 1);
		sprintf_s(trade->OrderRef, sizeof(trade->OrderRef) - 1, "%d", _ees_order->client_token);
		sprintf_s(trade->OrderSysID, sizeof(trade->OrderSysID) - 1, "%lld", _ees_order->market_token);
		sprintf_s(trade->TradeID, sizeof(trade->TradeID) - 1, "%lld", pExec->m_ExecutionID);
		//strncpy(trade->TradeID, pExec->m_MarketExecID, sizeof(trade->TradeID) - 1);
		strncpy(trade->TradeTime, smd->get_the_time(pExec->m_Timestamp), sizeof(trade->TradeTime) - 1);
		trade->ExchangeID = _ees_order->ex;
		trade->Direction = _ees_order->direct;
		trade->HedgeFlag = _ees_order->hedge;
		trade->Offset = _ees_order->offset;
		trade->Price = pExec->m_Price;
		trade->Volume = pExec->m_Quantity;
		smd->on_trade_change(trade);
		delete trade;

		//开始伪造委托状态推送
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, _ees_order->symbol.c_str(), sizeof(order->Code) - 1);
		sprintf_s(order->OrderRef, sizeof(order->OrderRef) - 1, "%d", _ees_order->client_token);
		sprintf_s(order->OrderSysID, sizeof(order->OrderSysID) - 1, "%lld", _ees_order->market_token);
		order->LimitPrice = _ees_order->order_px;
		order->VolumeTotalOriginal = _ees_order->order_qty;
		order->VolumeTraded = _ees_order->deal_qty;
		order->VolumeTotal = _ees_order->order_qty - _ees_order->deal_qty;
		order->ExchangeID = _ees_order->ex;
		order->Direction = _ees_order->direct;
		order->OrderStatus = _ees_order->deal_qty == _ees_order->order_qty?(int)EnumOrderStatusType::AllTraded : (int)EnumOrderStatusType::NoTradeQueueing;
		order->HedgeFlag = _ees_order->hedge;
		order->Offset = _ees_order->offset;
		strncpy(order->StatusMsg, "成交中", sizeof(order->StatusMsg) - 1);
		strncpy(order->OrderTime, smd->get_the_time(_ees_order->order_time), sizeof(order->OrderTime) - 1);
		smd->on_order_change(order);
		delete order;
	}
}

///	订单成功撤销事件
/// \brief	成交里面包括了订单市场ID，建议用这个ID查询对应的订单
/// \param  pCxled		               订单被接受以后的消息体，里面包含了市场订单ID
/// \return void 
void SpiderEesTdSpi::OnOrderCxled(EES_OrderCxled* pCxled)
{
	if (pCxled)
	{
		LOGI(myAccount.account_id << ", EES:撤单成功："<< pCxled->m_Userid <<","<< pCxled->m_ClientOrderToken<<","<< pCxled->m_MarketOrderToken<<","<< pCxled->m_Reason);
		if (strcmp(myAccount.account_id, std::to_string(pCxled->m_Userid).c_str()) != 0)
		{
			LOGW("EES收到非本用户的OrderCxled：" << myAccount.account_id << "," << pCxled->m_Userid);
			return;
		}
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pCxled->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("逻辑错误：EES OnOrderCxled 找不到委托信息：" << pCxled->m_ClientOrderToken);
			return;
		}
		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, _ees_order->symbol.c_str(), sizeof(order->Code) - 1);
		sprintf_s(order->OrderRef, sizeof(order->OrderRef) - 1, "%d", _ees_order->client_token);
		sprintf_s(order->OrderSysID, sizeof(order->OrderSysID) - 1, "%lld", _ees_order->market_token);
		order->ExchangeID = _ees_order->ex;
		order->LimitPrice = _ees_order->order_px;
		order->VolumeTotalOriginal = _ees_order->order_qty;
		order->OrderStatus = Canceled;
		if (smd)
			smd->on_order_cancel(order);
		delete order;

		//开始伪造委托状态推送
		OrderInfo * order1 = new OrderInfo();
		memset(order1, 0, sizeof(OrderInfo));
		strncpy(order1->Code, _ees_order->symbol.c_str(), sizeof(order1->Code) - 1);
		sprintf_s(order1->OrderRef, sizeof(order1->OrderRef) - 1, "%d", _ees_order->client_token);
		sprintf_s(order1->OrderSysID, sizeof(order1->OrderSysID) - 1, "%lld", _ees_order->market_token);
		order1->LimitPrice = _ees_order->order_px;
		order1->VolumeTotalOriginal = _ees_order->order_qty;
		order1->VolumeTraded = _ees_order->deal_qty;
		order1->VolumeTotal = _ees_order->order_qty - _ees_order->deal_qty;
		order1->ExchangeID = _ees_order->ex;
		order1->Direction = _ees_order->direct;
		order1->OrderStatus = (int)EnumOrderStatusType::Canceled;
		order1->HedgeFlag = _ees_order->hedge;
		order1->Offset = _ees_order->offset;
		strncpy(order1->StatusMsg, "已撤单", sizeof(order1->StatusMsg) - 1);
		strncpy(order1->OrderTime, smd->get_the_time(_ees_order->order_time), sizeof(order1->OrderTime) - 1);
		if (smd)
			smd->on_order_change(order1);
		delete order1;
	}
}

///	撤单被拒绝的消息事件
/// \brief	一般会在发送撤单以后，收到这个消息，表示撤单被拒绝
/// \param  pReject	                   撤单被拒绝消息体
/// \return void 
void SpiderEesTdSpi::OnCxlOrderReject(EES_CxlOrderRej* pReject)
{
	if (pReject)
	{
		LOGI(myAccount.account_id << ", EES:撤单失败："<< pReject->m_UserID<<"," << pReject->m_ClientOrderToken << "," << pReject->m_MarketOrderToken << "," << pReject->m_ReasonCode << "," << pReject->m_ReasonText);
		if (strcmp(myAccount.account_id, std::to_string(pReject->m_UserID).c_str()) != 0)
		{
			LOGW("EES收到非本用户的CxlOrderReject：" << myAccount.account_id << "," << pReject->m_UserID);
			return;
		}
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pReject->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("逻辑错误：EES OnCxlOrderReject 找不到委托信息：" << pReject->m_ClientOrderToken);
			return;
		}

		OrderInfo * order = new OrderInfo();
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, _ees_order->symbol.c_str(), sizeof(order->Code) - 1);
		sprintf_s(order->OrderRef, sizeof(order->OrderRef) - 1, "%d", _ees_order->client_token);
		sprintf_s(order->OrderSysID, sizeof(order->OrderSysID) - 1, "%lld", _ees_order->market_token);
		order->ExchangeID = _ees_order->ex;
		order->LimitPrice = _ees_order->order_px;
		order->VolumeTotalOriginal = _ees_order->order_qty;
		order->OrderStatus = -1; //约定好的失败
		strncpy(order->StatusMsg, pReject->m_ReasonText, sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_cancel(order);
		delete order;
	}
}

///	查询订单的返回事件
/// \brief	查询订单信息时候的回调，这里面也可能包含不是当前用户下的订单
/// \param  pAccount                 帐户ID 
/// \param  pQueryOrder	             查询订单的结构
/// \param  bFinish	                 如果没有传输完成，这个值是 false，如果完成了，那个这个值为 true    
/// \remark 如果碰到 bFinish == true，那么是传输结束，并且 pQueryOrder值无效。
/// \return void 
void SpiderEesTdSpi::OnQueryTradeOrder(const char* pAccount, EES_QueryAccountOrder* pQueryOrder, bool bFinish)
{
	if (pQueryOrder)
	{
		if (strcmp(myAccount.account_id, std::to_string(pQueryOrder->m_Userid).c_str()) != 0)
		{
			LOGW("EES收到非本用户的QueryTradeOrder：" << myAccount.account_id << "," << pQueryOrder->m_Userid);
			if (bFinish)
			{
				smd->on_rsp_query_order(NULL, 88, bFinish);
			}
			return;
		}
		OrderInfo * order = new OrderInfo;
		memset(order, 0, sizeof(OrderInfo));
		strncpy(order->Code, pQueryOrder->m_symbol, sizeof(order->Code) - 1);
		sprintf_s(order->OrderRef, sizeof(order->OrderRef) - 1, "%d", pQueryOrder->m_ClientOrderToken);
		sprintf_s(order->OrderSysID, sizeof(order->OrderSysID) - 1, "%lld", pQueryOrder->m_MarketOrderToken);
		order->LimitPrice = pQueryOrder->m_Price;
		order->VolumeTotalOriginal = pQueryOrder->m_Quantity;
		order->VolumeTraded = pQueryOrder->m_FilledQty;
		order->VolumeTotal = pQueryOrder->m_Quantity - pQueryOrder->m_FilledQty;
		order->ExchangeID = get_exid_from_ees(pQueryOrder->m_ExchengeID);
		order->Direction = get_direction_from_ees(pQueryOrder->m_SideType);
		order->OrderStatus = get_orderstatus_from_ees(pQueryOrder->m_OrderStatus);
		order->HedgeFlag = Speculation;
		order->Offset = get_offset_from_ees(pQueryOrder->m_SideType);
		strncpy(order->OrderTime, smd->get_the_time(pQueryOrder->m_Timestamp), sizeof(order->OrderTime) - 1);
		if (smd)
		{
			smd->on_rsp_query_order(order, 88, bFinish);
		}
	}
}

///	查询订单的返回事件
/// \brief	查询订单信息时候的回调，这里面也可能包含不是当前用户下的订单成交
/// \param  pAccount                        帐户ID 
/// \param  pQueryOrderExec	                查询订单成交的结构
/// \param  bFinish	                        如果没有传输完成，这个值是false，如果完成了，那个这个值为 true    
/// \remark 如果碰到 bFinish == true，那么是传输结束，并且pQueryOrderExec值无效。
/// \return void 
void SpiderEesTdSpi::OnQueryTradeOrderExec(const char* pAccount, EES_QueryOrderExecution* pQueryOrderExec, bool bFinish)
{
	if (pQueryOrderExec)
	{
		if (strcmp(myAccount.account_id, std::to_string(pQueryOrderExec->m_Userid).c_str()) != 0)
		{
			LOGW("EES收到非本用户的QueryTradeOrderExec：" << myAccount.account_id << "," << pQueryOrderExec->m_Userid);
			if (bFinish)
			{
				smd->on_rsp_query_trade(NULL, 88, bFinish);
			}
			return;
		}
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pQueryOrderExec->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("逻辑错误：EES OnQueryTradeOrderExec 找不到委托信息：" << pQueryOrderExec->m_ClientOrderToken);
			return;
		}

		TradeInfo * trade = new TradeInfo;
		memset(trade, 0, sizeof(TradeInfo));
		strncpy(trade->Code, _ees_order->symbol.c_str(), sizeof(trade->Code) - 1);
		sprintf_s(trade->OrderRef, sizeof(trade->OrderRef) - 1, "%d", _ees_order->client_token);
		sprintf_s(trade->OrderSysID, sizeof(trade->OrderSysID) - 1, "%lld", _ees_order->market_token);
		//strncpy(trade->TradeID, pQueryOrderExec->m_MarketExecID, sizeof(trade->TradeID) - 1);
		sprintf_s(trade->TradeID, sizeof(trade->TradeID) - 1, "%lld", pQueryOrderExec->m_ExecutionID);
		strncpy(trade->TradeTime, smd->get_the_time(pQueryOrderExec->m_Timestamp), sizeof(trade->TradeTime) - 1);
		trade->ExchangeID = _ees_order->ex;
		trade->Direction = _ees_order->direct;
		trade->HedgeFlag = _ees_order->hedge;
		trade->Offset = _ees_order->offset;
		trade->Price = pQueryOrderExec->m_ExecutionPrice;
		trade->Volume = pQueryOrderExec->m_ExecutedQuantity;
		smd->on_rsp_query_trade(trade, 89, bFinish);

	}
}

//*********************************************************

SpiderEesTdSession::SpiderEesTdSession(SpiderCommonApi * sci, AccountInfo & ai)
	: exoid(0)
	, BaseTradeSession(sci, ai)
{
	tradeConnection.reset(new SpiderEesTdSpi());
}

SpiderEesTdSession::~SpiderEesTdSession()
{

}

bool SpiderEesTdSession::init()
{
	return tradeConnection->init(this);
}

void SpiderEesTdSession::start()
{
	tradeConnection->start();
}

void SpiderEesTdSession::stop()
{
	tradeConnection->stop();
}

const char * SpiderEesTdSession::insert_order(OrderInsert * order)
{
	char * _order_ref = new char[13];
	memset(_order_ref, 0, sizeof(_order_ref));

	EES_EnterOrderField field;
	memset(&field, 0, sizeof(field));

	field.m_ClientOrderToken = get_real_order_ref(atoi(order->OrderRef));

	//返回值
	sprintf_s(_order_ref, 13, "%d", field.m_ClientOrderToken);
	//delete[] _order_ref; //要在使用的地方删除掉,api使用者自己释放

	if (strlen(get_account().broker_account_id) <= 0)
	{
		LOGW("Warning: EES EnterOrder, 未获得真实资金账户");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "未获得真实资金账户", sizeof(_err->ErrMsg) - 1);
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
		strncpy(failorder->StatusMsg, "未获得真实资金账户", sizeof(failorder->StatusMsg) - 1);
		tradeConnection->add_task(async_task(async_task_send_order_fail, failorder));

		return _order_ref;
	}

	field.m_Tif = EES_OrderTif_Day;
	field.m_HedgeFlag = EES_HedgeFlag_Speculation;

	strncpy(field.m_Account, get_account().broker_account_id,sizeof(field.m_Account) - 1);
	strncpy(field.m_Symbol, order->Code, sizeof(field.m_Symbol) - 1);

	switch (order->Offset)
	{
	case EnumOffsetFlagType::Open:
	{
		field.m_Side = order->Direction == EnumDirectionType::Buy ? EES_SideType_open_long : EES_SideType_open_short;
		break;
	}
	case EnumOffsetFlagType::Close:
	{
		if (order->ExchangeID == EnumExchangeIDType::SHFE)
		{
			field.m_Side = order->Direction == EnumDirectionType::Buy ? EES_SideType_close_today_short : EES_SideType_close_today_long;
		}
		else {
			field.m_Side = order->Direction == EnumDirectionType::Buy ? EES_SideType_close_short : EES_SideType_close_long;
		}
		break;
	}
	case EnumOffsetFlagType::CloseToday:
	{
		field.m_Side = order->Direction == EnumDirectionType::Buy ? EES_SideType_close_today_short : EES_SideType_close_today_long;
		break;
	}
	case EnumOffsetFlagType::CloseYesterday:
	{
		field.m_Side = order->Direction == EnumDirectionType::Buy ? EES_SideType_close_ovn_short : EES_SideType_close_ovn_long;
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
		tradeConnection->add_task(async_task(async_task_send_order_fail, failorder));

		return _order_ref;
	}
	}

	if (!tradeConnection->ready())
	{
		LOGW("EES EnterOrder, 盛立服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "盛立服务器尚未登录", sizeof(_err->ErrMsg) - 1);
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
		strncpy(failorder->StatusMsg, "盛立服务器尚未登录", sizeof(failorder->StatusMsg) - 1);
		tradeConnection->add_task(async_task(async_task_send_order_fail, failorder));

		return _order_ref;
	}

	field.m_Exchange = get_exid_to_ees((EnumExchangeIDType)order->ExchangeID);
	field.m_SecType = EES_SecType_fut;
	field.m_Price = order->LimitPrice;
	field.m_Qty = order->VolumeTotalOriginal;


	std::shared_ptr<ees_order> _ees_order(new ees_order(order));
	{
		std::unique_lock<std::mutex> l(exoid_mutex);
		_ees_order->client_token = (int)field.m_ClientOrderToken;
		orderref_order_map.insert(std::make_pair(field.m_ClientOrderToken,_ees_order));
	}

	int r = tradeConnection->getUserApi()->EnterOrder(&field);
	if (r != 0)
	{
		LOGW("EES EnterOrder, error no: " << r);
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
		tradeConnection->add_task(async_task(async_task_send_order_fail, failorder));

		//开始触发重连
		if (r <= NO_CONN_SERVER && tradeConnection->ready())
		{
			tradeConnection->setReady(false);
			on_disconnected();
			//盛立接口不会自动重连，手动重连			
			tradeConnection->add_task(async_task(async_task_login, NULL));
		}
	}
	LOGI("EES EnterOrder: " << _order_ref);
	return _order_ref;
}

void SpiderEesTdSession::cancel_order(OrderCancel * order)
{
	if (strlen(get_account().broker_account_id) <= 0)
	{
		LOGW("EES Withdraw, 未获得真实资金账户");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "未获得真实资金账户", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	if (!tradeConnection->ready())
	{
		LOGW("EES Withdraw, 盛立服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "盛立服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	EES_CancelOrder action;
	memset(&action, 0, sizeof(action));
	strncpy(action.m_Account, get_account().broker_account_id, sizeof(action.m_Account) - 1);
	action.m_MarketOrderToken = atoll(order->OrderSysID);
	LOGD(get_account().account_id << ":EES Withdraw: " << action.m_MarketOrderToken);

	int r = tradeConnection->getUserApi()->CancelOrder(&action);
	if (r != 0)
	{
		LOGW("EES CancelOrder, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "CancelOrder调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);

		//开始触发重连
		if (r <= NO_CONN_SERVER && tradeConnection->ready())
		{
			tradeConnection->setReady(false);
			on_disconnected();
			//盛立接口不会自动重连，手动重连			
			tradeConnection->add_task(async_task(async_task_login, NULL));
		}
	}
}

void SpiderEesTdSession::query_trading_account(int request_id)
{
	if (strlen(get_account().broker_account_id) <= 0)
	{
		LOGW("EES QueryAccountBP, 未获得真实资金账户");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "未获得真实资金账户", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	if (!tradeConnection->ready())
	{
		LOGW("EES QueryAccountBP, 盛立服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "盛立服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_account_info.find(request_id);
		if (it != cache_account_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " EES 上一次查询账户资金尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户资金尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	LOGI(get_account().account_id <<","<< get_account().broker_account_id << ":EES query clients trading_account, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->QueryAccountBP(get_account().broker_account_id, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ": EES failed to query trading_account, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountBP调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);

		//开始触发重连
		if (result <= NO_CONN_SERVER && tradeConnection->ready())
		{
			tradeConnection->setReady(false);
			on_disconnected();
			//盛立接口不会自动重连，手动重连			
			tradeConnection->add_task(async_task(async_task_login, NULL));
		}
	}

}
void SpiderEesTdSession::query_positions(int request_id)
{
	if (strlen(get_account().broker_account_id) <= 0)
	{
		LOGW("EES QueryAccountPosition, 未获得真实资金账户");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "未获得真实资金账户", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	if (!tradeConnection->ready())
	{
		LOGW("EES QueryAccountPosition, 盛立服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "盛立服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_positions_info.find(request_id);
		if (it != cache_positions_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " EES 上一次查询账户持仓尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户持仓尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	LOGI(get_account().account_id << ":EES query clients positions, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->QueryAccountPosition(get_account().broker_account_id, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: EES failed to query position, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountPosition调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);

		//开始触发重连
		if (result <= NO_CONN_SERVER && tradeConnection->ready())
		{
			tradeConnection->setReady(false);
			on_disconnected();
			//盛立接口不会自动重连，手动重连			
			tradeConnection->add_task(async_task(async_task_login, NULL));
		}
	}
}
void SpiderEesTdSession::query_orders(int request_id)
{
	if (strlen(get_account().broker_account_id) <= 0)
	{
		LOGW("EES QueryAccountOrder, 未获得真实资金账户");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "未获得真实资金账户", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	if (!tradeConnection->ready())
	{
		LOGW("EES QueryAccountOrder, 盛立服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "盛立服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	request_id = 88; //因为盛立接口中没有request_id
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_orders_info.find(request_id);
		if (it != cache_orders_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " EES 上一次查询账户委托尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户委托尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	LOGI(get_account().account_id << ":EES query clients orders, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->QueryAccountOrder(get_account().broker_account_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":EES failed to query orders, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountOrder调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);

		//开始触发重连
		if (result <= NO_CONN_SERVER && tradeConnection->ready())
		{
			tradeConnection->setReady(false);
			on_disconnected();
			//盛立接口不会自动重连，手动重连			
			tradeConnection->add_task(async_task(async_task_login, NULL));
		}
	}
}
void SpiderEesTdSession::query_trades(int request_id)
{
	if (strlen(get_account().broker_account_id) <= 0)
	{
		LOGW("EES QueryAccountOrderExecution, 未获得真实资金账户");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "未获得真实资金账户", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	if (!tradeConnection->ready())
	{
		LOGW("EES QueryAccountOrderExecution, 盛立服务器尚未登录");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "盛立服务器尚未登录", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	request_id = 89; //因为盛立接口中没有request_id
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_trades_info.find(request_id);
		if (it != cache_trades_info.end())
		{
			//还有记录时，说明上一次查询没有完成，不允许查询
			LOGW(get_account().account_id << " EES 上一次查询账户成交尚未完成，请稍后再试");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "上一次查询账户成交尚未完成，请稍后再试", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	LOGI(get_account().account_id << ":EES query clients trades, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->QueryAccountOrderExecution(get_account().broker_account_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":EES failed to query trades, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountOrderExecution调用失败", sizeof(_err->ErrMsg) - 1);
		on_error(_err);

		//开始触发重连
		if (result <= NO_CONN_SERVER && tradeConnection->ready())
		{
			tradeConnection->setReady(false);
			on_disconnected();
			//盛立接口不会自动重连，手动重连			
			tradeConnection->add_task(async_task(async_task_login, NULL));
		}
	}
}

void SpiderEesTdSession::on_connected()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnConnect();
		}
	}
}

void SpiderEesTdSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnDisconnect();
		}
	}
}

void SpiderEesTdSession::on_log_in()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderEesTdSession::on_log_out()
{
	if (spider_common_api != NULL)
	{

		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnLogout();
		}
	}
}

void SpiderEesTdSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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


void SpiderEesTdSession::on_order_change(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeRtnOrder(order);
		}
	}
}
void SpiderEesTdSession::on_trade_change(TradeInfo * trade)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeRtnTrade(trade);
		}
	}
}
void SpiderEesTdSession::on_order_cancel(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnOrderCancel(order);
		}
	}
}
void SpiderEesTdSession::on_order_insert(OrderInfo * order)
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->tradeOnOrderInsert(order);
		}
	}
}

void SpiderEesTdSession::on_rsp_query_account(TradingAccount * account, int request_id, bool last)
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
void SpiderEesTdSession::on_rsp_query_position(InvestorPosition * position, int request_id, bool last)
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
void SpiderEesTdSession::on_rsp_query_trade(TradeInfo * trade, int request_id, bool last)
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
void SpiderEesTdSession::on_rsp_query_order(OrderInfo * order, int request_id, bool last)
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

void SpiderEesTdSession::init_exoid(int max_exoid)
{
	if (max_exoid > 0 && max_exoid < INT_MAX)
	{
		std::unique_lock<std::mutex> l(exoid_mutex);
		exoid = max_exoid / 1000;
	}	
}
int SpiderEesTdSession::get_real_order_ref(int order_ref)
{
	if (order_ref >= 1000) //和C#策略那边约定好了，他只负责生成3位的委托编号
	{
		order_ref = order_ref % 1000;
	}
	std::unique_lock<std::mutex> l(exoid_mutex);
	int real_order_ref = (++exoid) * 1000 + order_ref;
	return real_order_ref;
}

void SpiderEesTdSession::insert_market_token(int orderref, long long token)
{
	std::unique_lock<std::mutex> l(exoid_mutex);
	auto it = orderref_order_map.find(orderref);
	if (it != orderref_order_map.end())
	{
		it->second->market_token = token;
	}
}
void SpiderEesTdSession::update_deal_qty(int orderref, int qty)
{
	std::unique_lock<std::mutex> l(exoid_mutex);
	auto it = orderref_order_map.find(orderref);
	if (it != orderref_order_map.end())
	{
		it->second->deal_qty += qty;
	}
}
void SpiderEesTdSession::update_order_accept_tm(int orderref, unsigned long long int ordertm)
{
	std::unique_lock<std::mutex> l(exoid_mutex);
	auto it = orderref_order_map.find(orderref);
	if (it != orderref_order_map.end())
	{
		it->second->order_time = ordertm;
	}
}
std::shared_ptr<ees_order> SpiderEesTdSession::get_ees_order(int orderref)
{
	std::unique_lock<std::mutex> l(exoid_mutex);
	auto it = orderref_order_map.find(orderref);
	if (it != orderref_order_map.end())
	{
		return it->second;
	}
	else {
		return std::shared_ptr<ees_order>();
	}
}

const char * SpiderEesTdSession::get_the_time(unsigned long long int nanosec)
{
	static char nowString1[16] = {0};
	struct tm tmResult;
	unsigned int nanoSsec;
	tradeConnection->getUserApi()->ConvertFromTimestamp(nanosec, tmResult, nanoSsec);
	char nowString[12] = { 0 };
	strftime(nowString, sizeof(nowString), "%H:%M:%S", &tmResult);
	sprintf_s(nowString1,sizeof(nowString1)-1 ,"%s.%d", nowString,(int)((nanosec % 1000000000)/1000000));
	return nowString1;
}