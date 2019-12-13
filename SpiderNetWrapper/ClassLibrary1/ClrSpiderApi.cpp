// 这是主 DLL 文件。

#include "stdafx.h"
#include <gcroot.h>
#include <msclr/marshal.h>
#include "ClrSpiderApi.h"
#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>



namespace Spider {
	//==========工具类==================

	public struct QuotaTask
	{
		void * dest;
		QuotaData * data;
	};
	///线程安全的队列
	template<typename Data>
	class ConcurrentQueue
	{
	private:
		std::queue<Data> the_queue;							//标准库队列
		mutable boost::mutex the_mutex;							//boost互斥锁
		boost::condition_variable the_condition_variable;			//boost条件变量

	public:

		//存入新的任务
		void push(Data const& data)
		{
			boost::mutex::scoped_lock lock(the_mutex);				//获取互斥锁
			the_queue.push(data);							//向队列中存入数据
			lock.unlock();									//释放锁
			the_condition_variable.notify_one();			//通知正在阻塞等待的线程
		}

		//检查队列是否为空
		bool empty() const
		{
			boost::mutex::scoped_lock lock(the_mutex);
			return the_queue.empty();
		}

		//取出
		Data wait_and_pop()
		{
			boost::mutex::scoped_lock lock(the_mutex);

			while (the_queue.empty())						//当队列为空时
			{
				the_condition_variable.wait(lock);			//等待条件变量通知
			}

			Data popped_value = the_queue.front();			//获取队列中的最后一个任务
			the_queue.pop();								//删除该任务
			return popped_value;							//返回该任务
		}

	};

	class QuoteSingleThreadWrapper
	{
		ConcurrentQueue<QuotaTask> task_queue;
		boost::thread* task_thread;
	public:
		QuoteSingleThreadWrapper();
		~QuoteSingleThreadWrapper();
		void inQueue(QuotaTask & task);
		void processQueue();
	};
	QuoteSingleThreadWrapper::QuoteSingleThreadWrapper()
	{
		boost::function0<void> f = boost::bind(&QuoteSingleThreadWrapper::processQueue, this);
		boost::thread t(f);
		this->task_thread = &t;
	}
	QuoteSingleThreadWrapper::~QuoteSingleThreadWrapper()
	{

	}
	void QuoteSingleThreadWrapper::inQueue(QuotaTask & task)
	{
		this->task_queue.push(task);
	}
	void QuoteSingleThreadWrapper::processQueue()
	{
		while (true)
		{
			QuotaTask task = this->task_queue.wait_and_pop();
			if (task.dest != NULL && task.data != NULL)
			{
				gcroot<BaseQuotaApi^>* mdClr = (gcroot<BaseQuotaApi^>*)task.dest;
				((BaseQuotaApi^)* mdClr)->RaiseOnDataArriveEvent(task.data);
			}
			if (task.data != NULL) //在这里释放行情指针，C++ api中不再释放 
			{
				delete task.data;
				task.data = NULL;
			}
		}
	}
//=======工具类结束=========================================================

	BaseQuotaApi::BaseQuotaApi()
	{
		this->selfCommon = new SelfCommonApi(this);
	}

	BaseQuotaApi::~BaseQuotaApi()
	{
		if (selfCommon)
		{
			delete selfCommon;
			selfCommon = NULL;
		}
	}

	bool BaseQuotaApi::Init(LoginInfo^ Account)
	{
		if (Account->AccountType != EnumAccountType::AccountMarketFuture && 
			Account->AccountType != EnumAccountType::AccountMarketIndex)
		{
			return false;
		}
		return this->selfCommon->Init(Account);
	}

	void BaseQuotaApi::Start()
	{
		this->selfCommon->StartMarket();
	}

	void BaseQuotaApi::Stop()
	{
		this->selfCommon->StopMarket();
	}

	void BaseQuotaApi::Restart()
	{
		this->selfCommon->RestartMarket();
	}

	void BaseQuotaApi::Subscribe(array<String^> ^ list)
	{
		this->selfCommon->SubscribeMarket(list);
	}

	void BaseQuotaApi::Unsubscribe(array<String^> ^ list)
	{
		this->selfCommon->UnsubscribeMarket(list);
	}

	array<LoginInfo^>^ BaseQuotaApi::getAllAccounts()
	{
		return this->selfCommon->getAllAccounts();
	}

	//===========交易接口==============================

	BaseTradeApi::BaseTradeApi()
	{
		this->selfCommon = new SelfCommonApi(this);
	}

	BaseTradeApi::~BaseTradeApi()
	{
		if (selfCommon)
		{
			delete selfCommon;
			selfCommon = NULL;
		}
	}

	bool BaseTradeApi::Init(LoginInfo^ Account)
	{
		if (Account->AccountType != EnumAccountType::AccountTradeFuture)
		{
			return false;
		}
		return this->selfCommon->Init(Account);
	}

	void BaseTradeApi::Start()
	{
		this->selfCommon->StartTrade();
	}

	void BaseTradeApi::Stop()
	{
		this->selfCommon->StopTrade();
	}

	void BaseTradeApi::Restart()
	{
		this->selfCommon->RestartTrade();
	}

	String^ BaseTradeApi::InsertOrder(OrderInsertInfo ^ Order)
	{
		return this->selfCommon->InsertOrder(Order);
	}

	void BaseTradeApi::CancelOrder(OrderCancelInfo ^ Order)
	{
		this->selfCommon->CancelOrder(Order);
	}

	array<LoginInfo^>^ BaseTradeApi::getAllAccounts()
	{
		return this->selfCommon->getAllAccounts(true);
	}

	void BaseTradeApi::QueryTradingAccount(int request_id)
	{
		this->selfCommon->QueryTradingAccount(request_id);
	}
	void BaseTradeApi::QueryPositions(int request_id)
	{
		this->selfCommon->QueryPositions(request_id);
	}
	void BaseTradeApi::QueryOrders(int request_id)
	{
		this->selfCommon->QueryOrders(request_id);
	}
	void BaseTradeApi::QueryTrades(int request_id)
	{
		this->selfCommon->QueryTrades(request_id);
	}

	//=============非托管=======================
	QuoteSingleThreadWrapper SelfCommonApi::quoteWrapper;
	SelfCommonApi::SelfCommonApi(BaseQuotaApi^ clr)
	{
		tdClr = NULL;
		mdClr = new gcroot<BaseQuotaApi^>(clr);
		
		nestSpider = SpiderApi::createSpiderApi();
		this->nestSpider->registerSpi(this);

	}
	SelfCommonApi::SelfCommonApi(BaseTradeApi^ clr)
	{
		mdClr = NULL;
		tdClr = new gcroot<BaseTradeApi^>(clr);

		nestSpider = SpiderApi::createSpiderApi();
		this->nestSpider->registerSpi(this);

	}
	SelfCommonApi::~SelfCommonApi()
	{
		if (nestSpider)
		{
			this->nestSpider->release();
		}	

		if (mdClr)
		{
			//gcroot<BaseQuotaApi^> *pp = static_cast<gcroot<BaseQuotaApi^>*>(this->common);
			delete mdClr;
		}
		if (tdClr)
		{
			//gcroot<BaseTradeApi^> *pp = static_cast<gcroot<BaseTradeApi^>*>(this->common);
			delete tdClr;
		}
	}

	bool SelfCommonApi::Init(LoginInfo^ Account)
	{
		using namespace Runtime::InteropServices;
		const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(Account->AccountID)).ToPointer();
		return this->nestSpider->init(chars, (int)Account->AccountType);
	}
	void SelfCommonApi::Release()
	{
		this->nestSpider->release();
	}

	array<LoginInfo^>^ SelfCommonApi::getAllAccounts(bool ifTrade)
	{
		AccountInfo ** _accounts = NULL;
		int ncount = this->nestSpider->getAllAccounts(_accounts);
		int real_count = 0;
		if (_accounts != NULL)
		{		
			for (int i = 0; i < ncount; i++)
			{
				if (ifTrade && _accounts[i]->account_type == (int)EnumAccountType::AccountTradeFuture)
				{
					++real_count;
				}
				else if (!ifTrade && _accounts[i]->account_type != (int)EnumAccountType::AccountTradeFuture)
				{
					++real_count;
				}
			}
		}
		array<LoginInfo^>^ _array = gcnew array<LoginInfo^>(real_count);
		if (_accounts != NULL)
		{
			int j = 0;
			for (int i = 0; i < ncount; i++)
			{
				if (ifTrade && _accounts[i]->account_type == (int)EnumAccountType::AccountTradeFuture)
				{
					LoginInfo^ _acc = gcnew LoginInfo(_accounts[i]);
					_array[j] = _acc;
					++j;
				}
				else if (!ifTrade && _accounts[i]->account_type != (int)EnumAccountType::AccountTradeFuture)
				{
					LoginInfo^ _acc = gcnew LoginInfo(_accounts[i]);
					_array[j] = _acc;
					++j;
				}
				delete _accounts[i];
			}
			delete _accounts;
		}

		return _array;
	}

	void SelfCommonApi::SubscribeMarket(array<String^> ^ list)
	{
		if (list->Length <= 0)
		{
			return;
		}
		char * *list_contract = new char * [list->Length];
		int i = 0;
		for each (String^ item in list)
		{
			using namespace Runtime::InteropServices;
			char* chars = (char*)(Marshal::StringToHGlobalAnsi(item)).ToPointer();
			list_contract[i] = chars;
			++i;
		}
		this->nestSpider->marketSubscribe(list_contract, list->Length);
		for (int j=0; j< i; ++j)
		{
			delete[] list_contract[j];
		}
		delete[] list_contract;
	}

	void SelfCommonApi::UnsubscribeMarket(array<String^> ^ list)
	{
		if (list->Length <= 0)
		{
			return;
		}
		char * *list_contract = new char *[list->Length];
		int i = 0;
		for each (String^ item in list)
		{
			using namespace Runtime::InteropServices;
			char* chars = (char*)(Marshal::StringToHGlobalAnsi(item)).ToPointer();
			list_contract[i] = chars;
			++i;
		}
		this->nestSpider->marketUnSubscribe(list_contract, list->Length);
		for (int j = 0; j< i; ++j)
		{
			delete[] list_contract[j];
		}
		delete[] list_contract;
	}

	//回调
	void SelfCommonApi::onError(SpiderErrorMsg * errmsg)
	{
		if (mdClr)
		{
			((BaseQuotaApi^)*mdClr)->RaiseRspErrorEvent(errmsg);
		}
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseRspErrorEvent(errmsg);
		}
	}

	void SelfCommonApi::marketOnConnect()
	{
		if (mdClr)
		{
			((BaseQuotaApi^)*mdClr)->RaiseOnConnectMarketEvent();
		}
	}
	void SelfCommonApi::marketOnDisconnect()
	{
		if (mdClr)
		{
			((BaseQuotaApi^)*mdClr)->RaiseOnDisconnectMarketEvent();
		}
	}
	void SelfCommonApi::marketOnLogin(const char * account_id, int account_type)
	{
		if (mdClr)
		{
			String^ acc = gcnew String(account_id);
			((BaseQuotaApi^)*mdClr)->RaiseOnLoginMarketEvent(acc, (EnumAccountType)account_type);
		}
	}
	void SelfCommonApi::marketOnLogout()
	{
		if (mdClr)
		{
			((BaseQuotaApi^)*mdClr)->RaiseOnLogoutMarketEvent();
		}
	}
	void SelfCommonApi::marketOnDataArrive(QuotaData * md)
	{
		if (mdClr)
		{
			//((BaseQuotaApi^)*mdClr)->RaiseOnDataArriveEvent(md);

			QuotaTask _task;
			_task.data = md;
			_task.dest = static_cast<void*>(mdClr);
			quoteWrapper.inQueue(_task);
		}
	}
	//============================================
	String^ SelfCommonApi::InsertOrder(OrderInsertInfo^ order)
	{
		using namespace Runtime::InteropServices;
		const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(order->Code)).ToPointer();
		const char* chars1 = (const char*)(Marshal::StringToHGlobalAnsi(order->OrderRef)).ToPointer();

		OrderInsert * _ord = new OrderInsert();
		strncpy(_ord->Code, chars, sizeof(_ord->Code));
		_ord->ExchangeID = (int)order->ExchangeID;
		_ord->Direction = (int)order->Direction;
		_ord->HedgeFlag = (int)order->HedgeFlag;
		_ord->Offset = (int)order->Offset;
		//_ord->OrderRef = order->OrderRef;
		strncpy(_ord->OrderRef, chars1, sizeof(_ord->OrderRef));
		_ord->LimitPrice = order->LimitPrice;
		_ord->VolumeTotalOriginal = order->VolumeTotalOriginal;
		const char * tmp = this->nestSpider->InsertOrder(_ord);
		String^ result = gcnew String(tmp);
		delete chars;
		delete chars1;
		delete _ord;
		return result;
	}
	void SelfCommonApi::CancelOrder(OrderCancelInfo^ order)
	{
		using namespace Runtime::InteropServices;
		const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(order->OrderSysID)).ToPointer();
		const char* chars1 = (const char*)(Marshal::StringToHGlobalAnsi(order->Code)).ToPointer();
		const char* chars2 = (const char*)(Marshal::StringToHGlobalAnsi(order->OrderRef)).ToPointer();

		OrderCancel * _ord = new OrderCancel();
		//_ord->OrderRef = order->OrderRef;	
		strncpy(_ord->OrderSysID, chars, sizeof(_ord->OrderSysID));
		strncpy(_ord->Code, chars1, sizeof(_ord->Code));
		strncpy(_ord->OrderRef, chars2, sizeof(_ord->OrderRef));
		_ord->ExchangeID = (int)order->ExchangeID;	
		this->nestSpider->CancelOrder(_ord);
		delete chars;
		delete chars1;
		delete chars2;
		delete _ord;
	}
	void SelfCommonApi::QueryTradingAccount(int request_id)
	{
		this->nestSpider->QueryTradingAccount(request_id);
	}
	void SelfCommonApi::QueryPositions(int request_id)
	{
		this->nestSpider->QueryPositions(request_id);
	}
	void SelfCommonApi::QueryOrders(int request_id)
	{
		this->nestSpider->QueryOrders(request_id);
	}
	void SelfCommonApi::QueryTrades(int request_id)
	{
		this->nestSpider->QueryTrades(request_id);

	}

	void SelfCommonApi::tradeOnConnect()
	{
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnConnectTradeEvent();
		}
	}
	void SelfCommonApi::tradeOnDisconnect()
	{
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnDisconnectTradeEvent();
		}
	}
	void SelfCommonApi::tradeOnLogin(const char * account_id, int account_type)
	{
		if (tdClr)
		{
			String^ acc = gcnew String(account_id);
			((BaseTradeApi^)*tdClr)->RaiseOnLoginTradeEvent(acc,(EnumAccountType)account_type);
		}
	}
	void SelfCommonApi::tradeOnLogout()
	{
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnLogoutTradeEvent();
		}
	}

	void SelfCommonApi::tradeRtnOrder(OrderInfo * order)
	{
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnRtnOrderEvent(order);
		}
	}
	void SelfCommonApi::tradeRtnTrade(TradeInfo *trade)
	{
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnRtnTradeEvent(trade);
		}
	}
	void SelfCommonApi::tradeOnOrderInsert(OrderInfo * order)
	{
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnRspOrderInsertEvent(order);
		}
	}
	void SelfCommonApi::tradeOnOrderCancel(OrderInfo * order)
	{
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnRspCancelEvent(order);
		}
	}
	void SelfCommonApi::tradeRspQueryAccount(TradingAccount ** accounts, int count, int request_id)
	{
		array<TradingAccountField^>^ _array = gcnew array<TradingAccountField^>(count);
		if (accounts != NULL)
		{
			int j = 0;
			for (int i = 0; i < count; i++)
			{
				TradingAccountField^ _acc = gcnew TradingAccountField(accounts[i]);
				_array[i] = _acc;
			}
		}
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnRspQryTradingAccountEvent(_array,request_id);
		}
	}
	void SelfCommonApi::tradeRspQueryPosition(InvestorPosition ** positions, int count, int request_id)
	{
		array<InvestorPositionField^>^ _array = gcnew array<InvestorPositionField^>(count);
		if (positions != NULL)
		{
			int j = 0;
			for (int i = 0; i < count; i++)
			{
				InvestorPositionField^ _acc = gcnew InvestorPositionField(positions[i]);
				_array[i] = _acc;
			}
		}
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnRspQryPositionEvent(_array, request_id);
		}
	}
	void SelfCommonApi::tradeRspQueryTrade(TradeInfo ** trades, int count, int request_id)
	{
		array<TradeInforField^>^ _array = gcnew array<TradeInforField^>(count);
		if (trades != NULL)
		{
			int j = 0;
			for (int i = 0; i < count; i++)
			{
				TradeInforField^ _acc = gcnew TradeInforField(trades[i]);
				_array[i] = _acc;
			}
		}
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnRspQryTradeEvent(_array, request_id);
		}
	}
	void SelfCommonApi::tradeRspQueryOrder(OrderInfo ** orders, int count, int request_id)
	{
		array<OrderInforField^>^ _array = gcnew array<OrderInforField^>(count);
		if (orders != NULL)
		{
			int j = 0;
			for (int i = 0; i < count; i++)
			{
				OrderInforField^ _acc = gcnew OrderInforField(orders[i]);
				_array[i] = _acc;
			}
		}
		if (tdClr)
		{
			((BaseTradeApi^)*tdClr)->RaiseOnRspQryOrderEvent(_array, request_id);
		}
	}


//############################################################
	
}

