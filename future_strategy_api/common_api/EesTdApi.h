#if !defined(SPIDER_EESTDAPI_H)
#define SPIDER_EESTDAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TdApi.h"
#include "SpiderApiStruct.h"
#include "EesTraderApi.h"
#include <map>
#include <deque>


struct ees_order
{
	unsigned int client_token;
	long long market_token;
	int ex;
	int direct;
	int hedge;
	int offset;
	int order_qty;
	double order_px;
	int deal_qty;
	std::string symbol;

	ees_order()
		: client_token(0), market_token(0), ex(0), direct(0), hedge(0), offset(0), order_qty(0), order_px(0), deal_qty(0), symbol("")
	{}
	ees_order(OrderInsert * _ord)
	{
		ex = _ord->ExchangeID;
		direct = _ord->Direction;
		hedge = _ord->HedgeFlag;
		offset = _ord->Offset;
		order_qty = _ord->VolumeTotalOriginal;
		order_px = _ord->LimitPrice;
		deal_qty = 0;
		symbol = _ord->Code;
	}
};

class SpiderEesTdSession;
class SpiderEesTdSpi : public EESTraderEvent
{
	EESTraderApi * userApi;

	SpiderEesTdSession * smd;

	AccountInfo myAccount;

	EES_TradeSvrInfo myServer;

	bool isReady;

	std::shared_ptr<std::thread> independent_thread; //���ڶ�ʱ���������첽��Щ�������

	std::deque<async_task> task_queue;
	std::mutex task_mutex;
public:
	EESTraderApi * getUserApi() { return userApi; }
	bool init(SpiderEesTdSession * sm);
	void start();
	void stop();
	void login();
	//void authenticate(); //����ʽ��֤
	bool ready() { return isReady; }

	void add_task(async_task _t);
	void process_task();

public:
	SpiderEesTdSpi();
	~SpiderEesTdSpi();

	///	\brief	�����������¼�
	///	\param  errNo                   ���ӳɹ���������Ϣ
	///	\param  pErrStr                 ������Ϣ
	///	\return void  

	virtual void OnConnection(ERR_NO errNo, const char* pErrStr);

	/// ���ӶϿ���Ϣ�Ļص�

	/// \brief	�����������Ͽ������յ������Ϣ
	/// \param  ERR_NO errNo         ���ӳɹ���������Ϣ
	/// \param  const char* pErrStr  ������Ϣ
	/// \return void  

	virtual void OnDisConnection(ERR_NO errNo, const char* pErrStr);

	/// ��¼��Ϣ�Ļص�

	/// \param  pLogon                  ��¼�ɹ�����ʧ�ܵĽṹ
	/// \return void 

	virtual void OnUserLogon(EES_LogonResponse* pLogon);

	/// �޸�������Ӧ�ص�

	/// \param  nResult                  ��������Ӧ�ĳɹ���񷵻���
	/// \return void 

	virtual void OnRspChangePassword(EES_ChangePasswordResult nResult) {}

	/// ��ѯ�û������ʻ��ķ����¼�

	/// \param  pAccountInfo	        �ʻ�����Ϣ
	/// \param  bFinish	                ���û�д�����ɣ����ֵ�� false ���������ˣ��Ǹ����ֵΪ true 
	/// \remark ������� bFinish == true����ô�Ǵ������������ pAccountInfoֵ��Ч��
	/// \return void 

	virtual void OnQueryUserAccount(EES_AccountInfo * pAccoutnInfo, bool bFinish);

	/// ��ѯ�ʻ������ڻ���λ��Ϣ�ķ����¼�	
	/// \param  pAccount	                �ʻ�ID 	
	/// \param  pAccoutnPosition	        �ʻ��Ĳ�λ��Ϣ					   
	/// \param  nReqId		                ����������Ϣʱ���ID�š�
	/// \param  bFinish	                    ���û�д�����ɣ����ֵ��false���������ˣ��Ǹ����ֵΪ true 
	/// \remark ������� bFinish == true����ô�Ǵ������������ pAccountInfoֵ��Ч��
	/// \return void 	
	virtual void OnQueryAccountPosition(const char* pAccount, EES_AccountPosition* pAccoutnPosition, int nReqId, bool bFinish);

	/// ��ѯ�ʻ�������Ȩ��λ��Ϣ�ķ����¼�, ע������ص�, ����һ��OnQueryAccountPosition, ����һ��QueryAccountPosition�����, �ֱ𷵻�, �ȷ����ڻ�, �ٷ�����Ȩ, ��ʹû����Ȩ��λ, Ҳ�᷵��һ��bFinish=true�ļ�¼
	/// \param  pAccount	                �ʻ�ID 	
	/// \param  pAccoutnPosition	        �ʻ��Ĳ�λ��Ϣ					   
	/// \param  nReqId		                ����������Ϣʱ���ID�š�
	/// \param  bFinish	                    ���û�д�����ɣ����ֵ��false���������ˣ��Ǹ����ֵΪ true 
	/// \remark ������� bFinish == true����ô�Ǵ������������ pAccountInfoֵ��Ч��
	/// \return void 	
	virtual void OnQueryAccountOptionPosition(const char* pAccount, EES_AccountOptionPosition* pAccoutnOptionPosition, int nReqId, bool bFinish) {}


	/// ��ѯ�ʻ������ʽ���Ϣ�ķ����¼�

	/// \param  pAccount	                �ʻ�ID 	
	/// \param  pAccoutnPosition	        �ʻ��Ĳ�λ��Ϣ					   
	/// \param  nReqId		                ����������Ϣʱ���ID��
	/// \return void 

	virtual void OnQueryAccountBP(const char* pAccount, EES_AccountBP* pAccoutnPosition, int nReqId);

	/// ��ѯ��Լ�б�ķ����¼�

	/// \param  pSymbol	                    ��Լ��Ϣ   
	/// \param  bFinish	                    ���û�д�����ɣ����ֵ�� false���������ˣ��Ǹ����ֵΪ true   
	/// \remark ������� bFinish == true����ô�Ǵ������������ pSymbol ֵ��Ч��
	/// \return void 

	virtual void OnQuerySymbol(EES_SymbolField* pSymbol, bool bFinish) {}

	/// ��ѯ�ʻ����ױ�֤��ķ����¼�

	/// \param  pAccount                    �ʻ�ID 
	/// \param  pSymbolMargin               �ʻ��ı�֤����Ϣ 
	/// \param  bFinish	                    ���û�д�����ɣ����ֵ�� false�������ɣ��Ǹ����ֵΪ true 
	/// \remark ������� bFinish == true����ô�Ǵ������������ pSymbolMargin ֵ��Ч��
	/// \return void 

	virtual void OnQueryAccountTradeMargin(const char* pAccount, EES_AccountMargin* pSymbolMargin, bool bFinish) {}

	/// ��ѯ�ʻ����׷��õķ����¼�

	/// \param  pAccount                    �ʻ�ID 
	/// \param  pSymbolFee	                �ʻ��ķ�����Ϣ	 
	/// \param  bFinish	                    ���û�д�����ɣ����ֵ�� false���������ˣ��Ǹ����ֵΪ true    
	/// \remark ������� bFinish == true ����ô�Ǵ������������ pSymbolFee ֵ��Ч��
	/// \return void 

	virtual void OnQueryAccountTradeFee(const char* pAccount, EES_AccountFee* pSymbolFee, bool bFinish) {}

	/// �µ�����̨ϵͳ���ܵ��¼�

	/// \brief ��ʾ��������Ѿ�����̨ϵͳ��ʽ�Ľ���
	/// \param  pAccept	                    �����������Ժ����Ϣ��
	/// \return void 

	virtual void OnOrderAccept(EES_OrderAcceptField* pAccept);


	/// �µ����г����ܵ��¼�

	/// \brief ��ʾ��������Ѿ�����������ʽ�Ľ���
	/// \param  pAccept	                    �����������Ժ����Ϣ�壬����������г�����ID
	/// \return void 
	virtual void OnOrderMarketAccept(EES_OrderMarketAcceptField* pAccept);


	///	�µ�����̨ϵͳ�ܾ����¼�

	/// \brief	��������̨ϵͳ�ܾ������Բ鿴�﷨�����Ƿ�ؼ�顣 
	/// \param  pReject	                    �����������Ժ����Ϣ��
	/// \return void 

	virtual void OnOrderReject(EES_OrderRejectField* pReject);


	///	�µ����г��ܾ����¼�

	/// \brief	�������г��ܾ������Բ鿴�﷨�����Ƿ�ؼ�顣 
	/// \param  pReject	                    �����������Ժ����Ϣ�壬����������г�����ID
	/// \return void 

	virtual void OnOrderMarketReject(EES_OrderMarketRejectField* pReject);


	///	�����ɽ�����Ϣ�¼�

	/// \brief	�ɽ���������˶����г�ID�����������ID��ѯ��Ӧ�Ķ���
	/// \param  pExec	                   �����������Ժ����Ϣ�壬����������г�����ID
	/// \return void 

	virtual void OnOrderExecution(EES_OrderExecutionField* pExec);

	///	�����ɹ������¼�

	/// \brief	�ɽ���������˶����г�ID�����������ID��ѯ��Ӧ�Ķ���
	/// \param  pCxled		               �����������Ժ����Ϣ�壬����������г�����ID
	/// \return void 

	virtual void OnOrderCxled(EES_OrderCxled* pCxled);

	///	�������ܾ�����Ϣ�¼�

	/// \brief	һ����ڷ��ͳ����Ժ��յ������Ϣ����ʾ�������ܾ�
	/// \param  pReject	                   �������ܾ���Ϣ��
	/// \return void 

	virtual void OnCxlOrderReject(EES_CxlOrderRej* pReject);

	///	��ѯ�����ķ����¼�

	/// \brief	��ѯ������Ϣʱ��Ļص���������Ҳ���ܰ������ǵ�ǰ�û��µĶ���
	/// \param  pAccount                 �ʻ�ID 
	/// \param  pQueryOrder	             ��ѯ�����Ľṹ
	/// \param  bFinish	                 ���û�д�����ɣ����ֵ�� false���������ˣ��Ǹ����ֵΪ true    
	/// \remark ������� bFinish == true����ô�Ǵ������������ pQueryOrderֵ��Ч��
	/// \return void 

	virtual void OnQueryTradeOrder(const char* pAccount, EES_QueryAccountOrder* pQueryOrder, bool bFinish);

	///	��ѯ�����ķ����¼�

	/// \brief	��ѯ������Ϣʱ��Ļص���������Ҳ���ܰ������ǵ�ǰ�û��µĶ����ɽ�
	/// \param  pAccount                        �ʻ�ID 
	/// \param  pQueryOrderExec	                ��ѯ�����ɽ��Ľṹ
	/// \param  bFinish	                        ���û�д�����ɣ����ֵ��false���������ˣ��Ǹ����ֵΪ true    
	/// \remark ������� bFinish == true����ô�Ǵ������������pQueryOrderExecֵ��Ч��
	/// \return void 

	virtual void OnQueryTradeOrderExec(const char* pAccount, EES_QueryOrderExecution* pQueryOrderExec, bool bFinish);

	///	�����ⲿ��������Ϣ

	/// \brief	һ�����ϵͳ�������������˹�������ʱ���õ���
	/// \param  pPostOrder	                    ��ѯ�����ɽ��Ľṹ
	/// \return void 

	virtual void OnPostOrder(EES_PostOrder* pPostOrder) {}

	///	�����ⲿ�����ɽ�����Ϣ

	/// \brief	һ�����ϵͳ�������������˹�������ʱ���õ���
	/// \param  pPostOrderExecution	             ��ѯ�����ɽ��Ľṹ
	/// \return void 

	virtual void OnPostOrderExecution(EES_PostOrderExecution* pPostOrderExecution) {}

	///	��ѯ�������������ӵ���Ӧ

	/// \brief	ÿ����ǰϵͳ֧�ֵĻ㱨һ�Σ���bFinish= trueʱ����ʾ���н���������Ӧ���ѵ����������Ϣ�����������õ���Ϣ��
	/// \param  pPostOrderExecution	             ��ѯ�����ɽ��Ľṹ
	/// \return void 
	virtual void OnQueryMarketSession(EES_ExchangeMarketSession* pMarketSession, bool bFinish) {}

	///	����������״̬�仯���棬

	/// \brief	�����������ӷ�������/�Ͽ�ʱ�����״̬
	/// \param  MarketSessionId: ���������Ӵ���
	/// \param  ConnectionGood: true��ʾ����������������false��ʾ���������ӶϿ��ˡ�
	/// \return void 
	virtual void OnMarketSessionStatReport(EES_MarketSessionId MarketSessionId, bool ConnectionGood) {}

	///	��Լ״̬�仯����

	/// \brief	����Լ״̬�����仯ʱ����
	/// \param  pSymbolStatus: �μ�EES_SymbolStatus��Լ״̬�ṹ�嶨��
	/// \return void 
	virtual void OnSymbolStatusReport(EES_SymbolStatus* pSymbolStatus) {}


	///	��Լ״̬��ѯ��Ӧ

	/// \brief  ��Ӧ��Լ״̬��ѯ����
	/// \param  pSymbolStatus: �μ�EES_SymbolStatus��Լ״̬�ṹ�嶨��
	/// \param	bFinish: ��Ϊtrueʱ����ʾ��ѯ���н�����ء���ʱpSymbolStatusΪ��ָ��NULL
	/// \return void 
	virtual void OnQuerySymbolStatus(EES_SymbolStatus* pSymbolStatus, bool bFinish) {}

	/// ��������ѯ��Ӧ
	/// \param	pMarketMBLData: �μ�EES_MarketMBLData�������ṹ�嶨��
	/// \param	bFinish: ��Ϊtrueʱ����ʾ��ѯ���н�����ء���ʱpMarketMBLData������,��m_RequestId��Ч
	/// \return void 
	virtual void OnQueryMarketMBLData(EES_MarketMBLData* pMarketMBLData, bool bFinish) {}

};

class SpiderEesTdSession : public BaseTradeSession
{
public:
	SpiderEesTdSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderEesTdSession();

	virtual bool init();
	virtual void start();
	virtual void stop();

	virtual const char * insert_order(OrderInsert *);
	virtual void cancel_order(OrderCancel *);

	virtual void on_connected();
	virtual void on_disconnected();
	virtual void on_log_in();
	virtual void on_log_out();
	virtual void on_error(std::shared_ptr<SpiderErrorMsg> & err_msg);

	virtual void on_order_change(OrderInfo *);
	virtual void on_trade_change(TradeInfo *);
	virtual void on_order_cancel(OrderInfo *);
	virtual void on_order_insert(OrderInfo *);

	virtual void query_trading_account(int);
	virtual void query_positions(int);
	virtual void query_orders(int);
	virtual void query_trades(int);
	virtual void on_rsp_query_account(TradingAccount * account, int request_id, bool last = false);
	virtual void on_rsp_query_position(InvestorPosition * position, int request_id, bool last = false);
	virtual void on_rsp_query_trade(TradeInfo * trade, int request_id, bool last = false);
	virtual void on_rsp_query_order(OrderInfo * order, int request_id, bool last = false);
public:
	void init_exoid(int max_exoid);
	//����C#�����е�3λorderref��Ȼ��ƴ����ʵ��orderref�������׷�����
	int get_real_order_ref(int order_ref);

	void insert_market_token(int orderref, long long token);
	void update_deal_qty(int orderref, int qty);
	std::shared_ptr<ees_order> get_ees_order(int orderref);

	const char * get_the_time(unsigned long long int nanosec);

private:
	std::shared_ptr<SpiderEesTdSpi> tradeConnection;

	int exoid;
	std::mutex exoid_mutex;

	std::map<int, std::shared_ptr<ees_order> > orderref_order_map;
	std::mutex order_mutex;

	//���浥����ѯ�����Ȼ��һ�����������
	std::mutex cache_mutex;
	std::map<int, std::vector<TradingAccount* > > cache_account_info; //key request_id
	std::map<int, std::map<std::string, InvestorPosition * > > cache_positions_info; //2nd key contract#direct
	std::map<int, std::vector<OrderInfo * > > cache_orders_info;
	std::map<int, std::vector<TradeInfo * > > cache_trades_info;
};






#endif