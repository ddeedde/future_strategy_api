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

		if (ips.size() == 5) //tcp://127.0.0.1:31001:127.0.0.1:31002 //��ͨ�棬�ֱ��ǽ��׵�ַ�Ͳ�ѯ��ַ
		{
			replace_all(ips[1], "/", "");

			strncpy(myServer.m_remoteTradeIp, ips[1].c_str(), sizeof(myServer.m_remoteTradeIp) - 1);
			myServer.m_remoteTradeTCPPort = (unsigned short)atoi(ips[2].c_str());
			strncpy(myServer.m_remoteQueryIp, ips[3].c_str(), sizeof(myServer.m_remoteQueryIp) - 1);
			myServer.m_remoteQueryTCPPort = (unsigned short)atoi(ips[4].c_str());

		}
		else if (ips.size() == 8) //fast://127.0.0.1:31001:31001:127.0.0.1:31002:0.0.0.0:65000 //���°棬�ֱ��ǽ��׷�����ip��TCP�˿ںţ�UDP�˿ںţ���ѯ��������ַ�����ص�ַ
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
			LOGE("Ees �����ļ��н��׵�ַ��ʽ����");
			return false;
		}
	}
	else {
		LOGE("�����ļ��з�������ַ����Ϊ��");
		return false;
	}
	
	if (strlen(myServer.m_remoteTradeIp) <= 0)
	{
		LOGE("Ees �����ļ��н��׵�ַΪ��");
		return false;
	}

	userApi = CreateEESTraderApi();
	if (userApi == NULL)
	{
		LOGE("����EES EESTraderApi ʧ��");
		return false;
	}
	userApi->SetLoggerSwitch(true); //����������־

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
		std::this_thread::sleep_for(std:: chrono::seconds(1));	
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
			LOGI(myAccount.account_id << ", EES:��ʼ��������");
			start();
			break;
		}
		case async_task_send_order_fail:
		{
			LOGI(myAccount.account_id << ", EES:����ֱ��ί��ʧ��");
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
	if (isReady) //�Ѿ���¼�ɹ���
	{
		return;
	}
	int ret = 0;
	if (myServer.m_remoteTradeUDPPort != 0) //���°�
	{
		ret = userApi->ConnServer(myServer, this);
	}
	else {
		ret = userApi->ConnServer(myServer.m_remoteTradeIp,myServer.m_remoteTradeTCPPort, this, myServer.m_remoteQueryIp, myServer.m_remoteQueryTCPPort);
	}
	if (ret != 0)
	{
		LOGE("����ʢ�����׺Ͳ�ѯ������ʧ��");
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
	LOGI("EESTradeDataSession ReqUserLogin.UserProductInfo is: " << myAccount.app_id << ", " << myAccount.account_id << ", " << myAccount.auth_code);

	int r = userApi->UserLogon(myAccount.account_id,myAccount.password,myAccount.app_id,myAccount.auth_code);
	if (r != 0)
	{
		LOGW("Warning: EESTradeDataSession encountered an error when try to login, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "UserLogon����ʧ��", sizeof(_err->ErrMsg) - 1);
		smd->on_error(_err);
	}
}

///	\brief	�����������¼�
///	\param  errNo                   ���ӳɹ���������Ϣ
///	\param  pErrStr                 ������Ϣ
///	\return void  
void SpiderEesTdSpi::OnConnection(ERR_NO errNo, const char* pErrStr)
{
	LOGI(myAccount.account_id << ", EES:���ӳɹ�:"<< pErrStr);
	smd->on_connected();
	login();
}

/// ���ӶϿ���Ϣ�Ļص�

/// \brief	�����������Ͽ������յ������Ϣ
/// \param  ERR_NO errNo         ���ӳɹ���������Ϣ
/// \param  const char* pErrStr  ������Ϣ
/// \return void  
void SpiderEesTdSpi::OnDisConnection(ERR_NO errNo, const char* pErrStr)
{
	LOGE(myAccount.account_id << ", EES:�����ж�:"<< pErrStr);
	smd->on_disconnected();
	//ʢ���ӿڲ����Զ��������ֶ�����
	isReady = false;
	add_task(async_task(async_task_login,NULL));
}

/// ��¼��Ϣ�Ļص�

/// \param  pLogon                  ��¼�ɹ�����ʧ�ܵĽṹ
/// \return void 
void SpiderEesTdSpi::OnUserLogon(EES_LogonResponse* pLogon)
{
	if (pLogon)
	{
		if (pLogon->m_Result != 0)
		{
			isReady = false;
			LOGW(myAccount.account_id << ", EES:��¼ʧ��: " << pLogon->m_Result);
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "EES��¼ʧ��", sizeof(_err->ErrMsg) - 1);
			smd->on_error(_err);
		}
		else {
			isReady = true;
			LOGI(myAccount.account_id << ", EES:��½�ɹ���"<< pLogon->m_UserId <<"����󵥺ţ�" << pLogon->m_MaxToken << "�����ز�����" << pLogon->m_OrderFCCount << "," << pLogon->m_OrderFCInterval << "," << pLogon->m_CancelFCCount << "," << pLogon->m_CancelFCInterval);
			if (smd)
			{
				smd->init_exoid((int)pLogon->m_MaxToken);
				smd->on_log_in();
			}
		}
	}
}

/// ��ѯ�û������ʻ��ķ����¼�

/// \param  pAccountInfo	        �ʻ�����Ϣ
/// \param  bFinish	                ���û�д�����ɣ����ֵ�� false ���������ˣ��Ǹ����ֵΪ true 
/// \remark ������� bFinish == true����ô�Ǵ������������ pAccountInfoֵ��Ч��
/// \return void 
void SpiderEesTdSpi::OnQueryUserAccount(EES_AccountInfo * pAccoutnInfo, bool bFinish)
{
	if (pAccoutnInfo)
	{

	}
}

/// ��ѯ�ʻ������ڻ���λ��Ϣ�ķ����¼�	
/// \param  pAccount	                �ʻ�ID 	
/// \param  pAccoutnPosition	        �ʻ��Ĳ�λ��Ϣ					   
/// \param  nReqId		                ����������Ϣʱ���ID�š�
/// \param  bFinish	                    ���û�д�����ɣ����ֵ��false���������ˣ��Ǹ����ֵΪ true 
/// \remark ������� bFinish == true����ô�Ǵ������������ pAccountInfoֵ��Ч��
/// \return void 	
void SpiderEesTdSpi::OnQueryAccountPosition(const char* pAccount, EES_AccountPosition* pAccoutnPosition, int nReqId, bool bFinish)
{
	if (pAccoutnPosition)
	{
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


/// ��ѯ�ʻ������ʽ���Ϣ�ķ����¼�
/// \param  pAccount	                �ʻ�ID 	
/// \param  pAccoutnPosition	        �ʻ��Ĳ�λ��Ϣ					   
/// \param  nReqId		                ����������Ϣʱ���ID��
/// \return void 
void SpiderEesTdSpi::OnQueryAccountBP(const char* pAccount, EES_AccountBP* pAccoutnPosition, int nReqId)
{
	if (pAccoutnPosition)
	{
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

/// �µ�����̨ϵͳ���ܵ��¼�
/// \brief ��ʾ��������Ѿ�����̨ϵͳ��ʽ�Ľ���
/// \param  pAccept	                    �����������Ժ����Ϣ��
/// \return void 
void SpiderEesTdSpi::OnOrderAccept(EES_OrderAcceptField* pAccept)
{
	if (pAccept)
	{
		LOGI(myAccount.account_id << ", EES:��̨ȷ�ϣ�" << pAccept->m_ClientOrderToken << "," << pAccept->m_MarketOrderToken << "," << pAccept->m_OrderState);
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			smd->insert_market_token(pAccept->m_ClientOrderToken, pAccept->m_MarketOrderToken);
			_ees_order = smd->get_ees_order(pAccept->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("�߼�����EES OnOrderAccept �Ҳ���ί����Ϣ��"<< pAccept->m_ClientOrderToken);
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
		strncpy(order->StatusMsg, "��̨ȷ��", sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_change(order);
		delete order;
	}
}


/// �µ����г����ܵ��¼�
/// \brief ��ʾ��������Ѿ�����������ʽ�Ľ���
/// \param  pAccept	                    �����������Ժ����Ϣ�壬����������г�����ID
/// \return void 
void SpiderEesTdSpi::OnOrderMarketAccept(EES_OrderMarketAcceptField* pAccept)
{
	if (pAccept)
	{
		LOGI(myAccount.account_id << ", EES:������ȷ�ϣ�" << pAccept->m_ClientOrderToken << "," << pAccept->m_MarketOrderToken << "," << pAccept->m_MarketOrderId);
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			smd->insert_market_token(pAccept->m_ClientOrderToken, pAccept->m_MarketOrderToken);
			_ees_order = smd->get_ees_order(pAccept->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("�߼�����EES OnOrderMarketAccept �Ҳ���ί����Ϣ��" << pAccept->m_ClientOrderToken);
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
		strncpy(order->StatusMsg, "������ȷ��", sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_change(order);
		delete order;
	}
}


///	�µ�����̨ϵͳ�ܾ����¼�
/// \brief	��������̨ϵͳ�ܾ������Բ鿴�﷨�����Ƿ�ؼ�顣 
/// \param  pReject	                    �����������Ժ����Ϣ��
/// \return void 
void SpiderEesTdSpi::OnOrderReject(EES_OrderRejectField* pReject)
{
	if (pReject)
	{
		LOGI(myAccount.account_id << ", EES:ί�б���̨�ܾ���" << pReject->m_ClientOrderToken << "," << pReject->m_ReasonCode << "," << pReject->m_GrammerResult << "," << pReject->m_RiskResult << "," << pReject->m_GrammerText << "," << pReject->m_RiskText);
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pReject->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("�߼�����EES OnOrderReject �Ҳ���ί����Ϣ��" << pReject->m_ClientOrderToken);
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
		order->OrderStatus = -1; //Լ���õ�ʧ��
		strncpy(order->StatusMsg, "ί�б���̨�ܾ�", sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_insert(order);
		delete order;
	}
}


///	�µ����г��ܾ����¼�
/// \brief	�������г��ܾ������Բ鿴�﷨�����Ƿ�ؼ�顣 
/// \param  pReject	                    �����������Ժ����Ϣ�壬����������г�����ID
/// \return void 
void SpiderEesTdSpi::OnOrderMarketReject(EES_OrderMarketRejectField* pReject)
{
	if (pReject)
	{
		LOGI(myAccount.account_id << ", EES:ί�б��������ܾ���" << pReject->m_ClientOrderToken << "," << pReject->m_MarketOrderToken << "," << pReject->m_ReasonText);

		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pReject->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("�߼�����EES OnOrderMarketReject �Ҳ���ί����Ϣ��" << pReject->m_ClientOrderToken);
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
		order->OrderStatus = -1; //Լ���õ�ʧ��
		strncpy(order->StatusMsg, pReject->m_ReasonText, sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_insert(order);
		delete order;
	}
}


///	�����ɽ�����Ϣ�¼�
/// \brief	�ɽ���������˶����г�ID�����������ID��ѯ��Ӧ�Ķ���
/// \param  pExec	                   �����������Ժ����Ϣ�壬����������г�����ID
/// \return void 
void SpiderEesTdSpi::OnOrderExecution(EES_OrderExecutionField* pExec)
{
	if (pExec)
	{
		LOGI(myAccount.account_id << ", EES:�ɽ��ر���" << pExec->m_ClientOrderToken<< "," << pExec->m_MarketOrderToken << "," << pExec->m_Quantity << "," << pExec->m_Price << "," << pExec->m_ExecutionID << "," << pExec->m_MarketExecID);
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			smd->update_deal_qty(pExec->m_ClientOrderToken, pExec->m_Quantity);
			_ees_order = smd->get_ees_order(pExec->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("�߼�����EES OnOrderExecution �Ҳ���ί����Ϣ��" << pExec->m_ClientOrderToken);
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

		//��ʼα��ί��״̬����
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
		strncpy(order->StatusMsg, "�ɽ���", sizeof(order->StatusMsg) - 1);
		smd->on_order_change(order);
		delete order;
	}
}

///	�����ɹ������¼�
/// \brief	�ɽ���������˶����г�ID�����������ID��ѯ��Ӧ�Ķ���
/// \param  pCxled		               �����������Ժ����Ϣ�壬����������г�����ID
/// \return void 
void SpiderEesTdSpi::OnOrderCxled(EES_OrderCxled* pCxled)
{
	if (pCxled)
	{
		LOGI(myAccount.account_id << ", EES:�����ɹ���"<< pCxled->m_ClientOrderToken<<","<< pCxled->m_MarketOrderToken<<","<< pCxled->m_Reason);
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pCxled->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("�߼�����EES OnOrderCxled �Ҳ���ί����Ϣ��" << pCxled->m_ClientOrderToken);
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

		//��ʼα��ί��״̬����
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
		strncpy(order1->StatusMsg, "�ѳ���", sizeof(order1->StatusMsg) - 1);
		if (smd)
			smd->on_order_change(order1);
		delete order1;
	}
}

///	�������ܾ�����Ϣ�¼�
/// \brief	һ����ڷ��ͳ����Ժ��յ������Ϣ����ʾ�������ܾ�
/// \param  pReject	                   �������ܾ���Ϣ��
/// \return void 
void SpiderEesTdSpi::OnCxlOrderReject(EES_CxlOrderRej* pReject)
{
	if (pReject)
	{
		LOGI(myAccount.account_id << ", EES:����ʧ�ܣ�" << pReject->m_ClientOrderToken << "," << pReject->m_MarketOrderToken << "," << pReject->m_ReasonCode << "," << pReject->m_ReasonText);
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pReject->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("�߼�����EES OnCxlOrderReject �Ҳ���ί����Ϣ��" << pReject->m_ClientOrderToken);
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
		order->OrderStatus = -1; //Լ���õ�ʧ��
		strncpy(order->StatusMsg, pReject->m_ReasonText, sizeof(order->StatusMsg) - 1);
		if (smd)
			smd->on_order_cancel(order);
		delete order;
	}
}

///	��ѯ�����ķ����¼�
/// \brief	��ѯ������Ϣʱ��Ļص���������Ҳ���ܰ������ǵ�ǰ�û��µĶ���
/// \param  pAccount                 �ʻ�ID 
/// \param  pQueryOrder	             ��ѯ�����Ľṹ
/// \param  bFinish	                 ���û�д�����ɣ����ֵ�� false���������ˣ��Ǹ����ֵΪ true    
/// \remark ������� bFinish == true����ô�Ǵ������������ pQueryOrderֵ��Ч��
/// \return void 
void SpiderEesTdSpi::OnQueryTradeOrder(const char* pAccount, EES_QueryAccountOrder* pQueryOrder, bool bFinish)
{
	if (pQueryOrder)
	{

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
		if (smd)
		{
			smd->on_rsp_query_order(order, 88, bFinish);
		}
	}
}

///	��ѯ�����ķ����¼�
/// \brief	��ѯ������Ϣʱ��Ļص���������Ҳ���ܰ������ǵ�ǰ�û��µĶ����ɽ�
/// \param  pAccount                        �ʻ�ID 
/// \param  pQueryOrderExec	                ��ѯ�����ɽ��Ľṹ
/// \param  bFinish	                        ���û�д�����ɣ����ֵ��false���������ˣ��Ǹ����ֵΪ true    
/// \remark ������� bFinish == true����ô�Ǵ������������pQueryOrderExecֵ��Ч��
/// \return void 
void SpiderEesTdSpi::OnQueryTradeOrderExec(const char* pAccount, EES_QueryOrderExecution* pQueryOrderExec, bool bFinish)
{
	if (pQueryOrderExec)
	{
		std::shared_ptr<ees_order> _ees_order;
		if (smd)
		{
			_ees_order = smd->get_ees_order(pQueryOrderExec->m_ClientOrderToken);
		}
		if (_ees_order.get() == NULL)
		{
			LOGE("�߼�����EES OnQueryTradeOrderExec �Ҳ���ί����Ϣ��" << pQueryOrderExec->m_ClientOrderToken);
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

	//����ֵ
	sprintf_s(_order_ref, 13, "%d", field.m_ClientOrderToken);
	//delete[] _order_ref; //Ҫ��ʹ�õĵط�ɾ����,apiʹ�����Լ��ͷ�

	field.m_Tif = EES_OrderTif_Day;
	field.m_HedgeFlag = EES_HedgeFlag_Speculation;

	strncpy(field.m_Account, get_account().account_id,sizeof(field.m_Account) - 1);
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
		LOGE("����Ŀ�ƽ����" << order->Offset);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "����Ŀ�ƽ����", sizeof(_err->ErrMsg) - 1);
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
		failorder->OrderStatus = -1; //Լ���õ�ʧ��
		strncpy(failorder->StatusMsg, "����Ŀ�ƽ����", sizeof(failorder->StatusMsg) - 1);
		tradeConnection->add_task(async_task(async_task_send_order_fail, failorder));

		return _order_ref;
	}
	}

	if (!tradeConnection->ready())
	{
		LOGW("Warning: EES EnterOrder, ʢ����������δ��¼");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "ʢ����������δ��¼", sizeof(_err->ErrMsg) - 1);
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
		failorder->OrderStatus = -1; //Լ���õ�ʧ��
		strncpy(failorder->StatusMsg, "ʢ����������δ��¼", sizeof(failorder->StatusMsg) - 1);
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
		LOGW("Warning: EES EnterOrder, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "EnterOrder����ʧ��", sizeof(_err->ErrMsg) - 1);
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
		failorder->OrderStatus = -1; //Լ���õ�ʧ��
		strncpy(failorder->StatusMsg, "EnterOrder����ʧ��", sizeof(failorder->StatusMsg) - 1);
		tradeConnection->add_task(async_task(async_task_send_order_fail, failorder));
	}

	return _order_ref;
}

void SpiderEesTdSession::cancel_order(OrderCancel * order)
{

	EES_CancelOrder action;
	memset(&action, 0, sizeof(action));
	strncpy(action.m_Account, get_account().account_id, sizeof(action.m_Account) - 1);
	action.m_MarketOrderToken = atoll(order->OrderSysID);
	LOGD(get_account().account_id << ":EES Withdraw: " << action.m_MarketOrderToken);

	if (!tradeConnection->ready())
	{
		LOGW("Warning: EES EnterOrder, ʢ����������δ��¼");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "ʢ����������δ��¼", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	int r = tradeConnection->getUserApi()->CancelOrder(&action);
	if (r != 0)
	{
		LOGW("Warning: EES CancelOrder, error no: " << r);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = r;
		strncpy(_err->ErrMsg, "CancelOrder����ʧ��", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}

void SpiderEesTdSession::query_trading_account(int request_id)
{
	if (!tradeConnection->ready())
	{
		LOGW("Warning: EES EnterOrder, ʢ����������δ��¼");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "ʢ����������δ��¼", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_account_info.find(request_id);
		if (it != cache_account_info.end())
		{
			//���м�¼ʱ��˵����һ�β�ѯû����ɣ��������ѯ
			LOGW(get_account().account_id << " Warning: CTP ��һ�β�ѯ�˻��ʽ���δ��ɣ����Ժ�����");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "��һ�β�ѯ�˻��ʽ���δ��ɣ����Ժ�����", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	LOGI(get_account().account_id << ":EES query clients trading_account, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->QueryAccountBP(get_account().account_id, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: EES failed to query trading_account, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountBP����ʧ��", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}

}
void SpiderEesTdSession::query_positions(int request_id)
{
	if (!tradeConnection->ready())
	{
		LOGW("Warning: EES EnterOrder, ʢ����������δ��¼");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "ʢ����������δ��¼", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_positions_info.find(request_id);
		if (it != cache_positions_info.end())
		{
			//���м�¼ʱ��˵����һ�β�ѯû����ɣ��������ѯ
			LOGW(get_account().account_id << " Warning: CTP ��һ�β�ѯ�˻��ֲ���δ��ɣ����Ժ�����");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "��һ�β�ѯ�˻��ֲ���δ��ɣ����Ժ�����", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	LOGI(get_account().account_id << ":EES query clients positions, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->QueryAccountPosition(get_account().account_id, request_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: EES failed to query position, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountPosition����ʧ��", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}
void SpiderEesTdSession::query_orders(int request_id)
{
	if (!tradeConnection->ready())
	{
		LOGW("Warning: EES EnterOrder, ʢ����������δ��¼");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "ʢ����������δ��¼", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	request_id = 88; //��Ϊʢ���ӿ���û��request_id
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_orders_info.find(request_id);
		if (it != cache_orders_info.end())
		{
			//���м�¼ʱ��˵����һ�β�ѯû����ɣ��������ѯ
			LOGW(get_account().account_id << " Warning: CTP ��һ�β�ѯ�˻�ί����δ��ɣ����Ժ�����");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "��һ�β�ѯ�˻�ί����δ��ɣ����Ժ�����", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	LOGI(get_account().account_id << ":EES query clients orders, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->QueryAccountOrder(get_account().account_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: EES failed to query orders, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountOrder����ʧ��", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
	}
}
void SpiderEesTdSession::query_trades(int request_id)
{
	if (!tradeConnection->ready())
	{
		LOGW("Warning: EES EnterOrder, ʢ����������δ��¼");
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = -1;
		strncpy(_err->ErrMsg, "ʢ����������δ��¼", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
		return;
	}

	request_id = 89; //��Ϊʢ���ӿ���û��request_id
	{
		std::unique_lock<std::mutex> l(cache_mutex);
		auto it = cache_trades_info.find(request_id);
		if (it != cache_trades_info.end())
		{
			//���м�¼ʱ��˵����һ�β�ѯû����ɣ��������ѯ
			LOGW(get_account().account_id << " Warning: CTP ��һ�β�ѯ�˻��ɽ���δ��ɣ����Ժ�����");
			std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
			_err->ErrCode = -1;
			strncpy(_err->ErrMsg, "��һ�β�ѯ�˻��ɽ���δ��ɣ����Ժ�����", sizeof(_err->ErrMsg) - 1);
			on_error(_err);
			return;
		}
	}

	LOGI(get_account().account_id << ":EES query clients trades, requestid:" << request_id);
	int result = tradeConnection->getUserApi()->QueryAccountOrderExecution(get_account().account_id);
	if (result != 0)
	{
		LOGW(get_account().account_id << ":Warning: EES failed to query trades, error:" << result);
		std::shared_ptr<SpiderErrorMsg> _err(new SpiderErrorMsg());
		_err->ErrCode = result;
		strncpy(_err->ErrMsg, "QueryAccountOrderExecution����ʧ��", sizeof(_err->ErrMsg) - 1);
		on_error(_err);
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
			if (position->Position > 0) //�ֲִ�������ٷ���
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
			if (position->Position > 0) //�ֲִ�������ٷ���
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
void SpiderEesTdSession::on_rsp_query_order(OrderInfo * order, int request_id, bool last)
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
	if (order_ref >= 1000) //��C#�����Ǳ�Լ�����ˣ���ֻ��������3λ��ί�б��
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
	static char nowString1[12] = {0};
	struct tm tmResult;
	unsigned int nanoSsec;
	tradeConnection->getUserApi()->ConvertFromTimestamp(nanosec, tmResult, nanoSsec);
	strftime(nowString1, sizeof(nowString1), "%H:%M:%S", &tmResult);
	return nowString1;
}