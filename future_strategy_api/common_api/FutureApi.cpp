#include "FutureApi.h"
#include "LogWrapper.h"
#include "CtpMdApi.h"
#include "MultiIndexApi.h"
#include "CtpTdApi.h"
#include "EesMdApi.h"
#include "EesTdApi.h"

SpiderCommonApi::SpiderCommonApi()
	: market_running(false)
	, trade_running(false)
	, notifySpi(NULL)
{
	market_session.reset();
	trade_session.reset();

	InitGlobalLog();
	if (!my_config.init())
	{
		LOGE("账户配置文件读取失败");
	}
}

SpiderCommonApi::~SpiderCommonApi()
{
	
}

bool SpiderCommonApi::init(const char * account_id, int account_type )
{
	SpiderErrorMsg _err;

	if (account_id == NULL)
	{
		LOGE("资金账户ID无效");
		if (notifySpi)
		{
			_err.ErrCode = -1;
			strncpy(_err.ErrMsg, "资金账户ID无效", sizeof(_err.ErrMsg) - 1);
			notifySpi->onError(&_err);
		}
		return false;
	}
	//一次只能登陆一个账户
	switch (account_type)
	{
	case AccountTradeFuture:
	{
		auto it = my_config.future_trade_account_list.find(account_id);
		if (it == my_config.future_trade_account_list.end())
		{
			LOGE("找不到该期货交易账户资金ID的配置信息：" << account_id);
			if (notifySpi)
			{
				_err.ErrCode = -1;
				strncpy(_err.ErrMsg, "找不到该期货交易账户资金ID的配置信息", sizeof(_err.ErrMsg) - 1);
				notifySpi->onError(&_err);
			}
			return false;
		}
		else {
			AccountInfo& _ci = it->second;
			switch (_ci.account_type)
			{
			case EnumAccountDetailType::AccountTradeFutureCTP:
			{
				trade_session.reset(new SpiderCtpTdSession(this, _ci));
				return trade_session->init();
			}
			case EnumAccountDetailType::AccountTradeFutureEES:
			{
				trade_session.reset(new SpiderEesTdSession(this, _ci));
				return trade_session->init();
			}
			default:
				LOGE("该期货交易账户配置文件中的类型未知");
				if (notifySpi)
				{
					_err.ErrCode = -1;
					strncpy(_err.ErrMsg, "该期货交易账户配置文件中的类型未知", sizeof(_err.ErrMsg) - 1);
					notifySpi->onError(&_err);
				}
				return false;
			}
		}
		break;
	}
	case AccountMarketFuture:
	{
		auto it = my_config.future_market_account_list.find(account_id);
		if (it == my_config.future_market_account_list.end())
		{
			LOGE("找不到该期货行情账户资金ID的配置信息："<< account_id);
			if (notifySpi)
			{
				_err.ErrCode = -1;
				strncpy(_err.ErrMsg, "找不到该期货行情账户资金ID的配置信息", sizeof(_err.ErrMsg) - 1);
				notifySpi->onError(&_err);
			}
			return false;
		}
		else {
			AccountInfo& _ci = it->second;
			switch (_ci.account_type)
			{
			case EnumAccountDetailType::AccountMarketFutureCTP:
			{
				market_session.reset(new SpiderCtpMdSession(this, _ci));
				return market_session->init();
			}
			case EnumAccountDetailType::AccountMarketFutureEES:
			{
				market_session.reset(new SpiderEesMdSession(this, _ci));
				return market_session->init();
			}
			default:
				LOGE("该期货行情账户配置文件中的类型未知");
				if (notifySpi)
				{
					_err.ErrCode = -1;
					strncpy(_err.ErrMsg, "该期货行情账户配置文件中的类型未知", sizeof(_err.ErrMsg) - 1);
					notifySpi->onError(&_err);
				}
				return false;
			}
		}
		break;
	}
	case AccountMarketIndex:
	{
		auto it = my_config.index_market_account_list.find(account_id);
		if (it == my_config.index_market_account_list.end())
		{
			LOGE("找不到该指数行情账户资金ID的配置信息：" << account_id);
			if (notifySpi)
			{
				_err.ErrCode = -1;
				strncpy(_err.ErrMsg, "找不到该指数行情账户资金ID的配置信息", sizeof(_err.ErrMsg) - 1);
				notifySpi->onError(&_err);
			}
			return false;
		}
		else {
			AccountInfo& _ci = it->second;
			switch (_ci.account_type)
			{
			case EnumAccountDetailType::AccountMarketIndexMulticast:
			{
				market_session.reset(new SpiderMultiIndexSession(this, _ci));
				market_session->setTest(my_config.is_test);
				return market_session->init();
			}
			default:
				LOGE("该指数行情账户配置文件中的类型未知");
				if (notifySpi)
				{
					_err.ErrCode = -1;
					strncpy(_err.ErrMsg, "该指数行情账户配置文件中的类型未知", sizeof(_err.ErrMsg) - 1);
					notifySpi->onError(&_err);
				}
				return false;
			}
		}
		break;
	}
	default:
		LOGE("暂不支持的账户类型："<< account_type);
		if (notifySpi)
		{
			_err.ErrCode = -1;
			strncpy(_err.ErrMsg, "暂不支持的账户类型", sizeof(_err.ErrMsg) - 1);
			notifySpi->onError(&_err);
		}
		return false;
	}

	return true;
}

void SpiderCommonApi::release()
{
	marketStop();
	tradeStop();
}

void SpiderCommonApi::marketStart()
{
	//一次性开关全部行情账户
	if (market_running)
	{
		return;
	}
	market_running = true;
	run_thread.reset(new std::thread(std::bind(&SpiderCommonApi::quoteWorker,this)));
	//for (auto it = market_connections.begin(); it != market_connections.end(); ++it)
	//{
	//	it->second->start();
	//}
	if (market_session.get() != NULL)
	{
		market_session->start();
	}
}

void SpiderCommonApi::marketStop()
{
	if (!market_running)
	{
		return;
	}
	//for (auto it = market_connections.begin(); it != market_connections.end(); ++it)
	//{
	//	it->second->stop();
	//}
	if (market_session.get() != NULL)
	{
		market_session->stop();
	}
	{
		std::unique_lock<std::mutex> l(run_mutex);
		quote_queue.clear();
	}
	market_running = false;
	run_cond.notify_one();
	if (run_thread.get())
	{
		run_thread->join();
		run_thread.reset();
	}
}

void SpiderCommonApi::marketRestart()
{
	marketStop();
	marketStart();
}

void SpiderCommonApi::onQuote(QuotaData * md)
{
	std::unique_lock<std::mutex> l(run_mutex);
	if (quote_queue.size() > MaxMarketQueueSize)
	{
		quote_queue.clear(); //为了消息的及时性
	}
	quote_queue.push_back(md);
	run_cond.notify_one();
}

void SpiderCommonApi::quoteWorker()
{
	while (market_running)
	{
		std::unique_lock<std::mutex> l(run_mutex);
		while (quote_queue.empty())
		{
			run_cond.wait(l);
		}
		if (!quote_queue.empty())
		{
			QuotaData * md = quote_queue.front();
			quote_queue.pop_front();
			l.unlock();

			if (notifySpi != NULL)
			{
				notifySpi->marketOnDataArrive(md);
			}
			//if (md) //api使用者自己释放
			//{
			//	delete md;
			//	md = NULL;
			//}
		}
	}
}

void SpiderCommonApi::marketSubscribe(char *codes[], int count)
{
	std::vector<std::string> contracts;
	for (int i = 0; i < count; i++)
	{
		contracts.push_back(codes[i]);
	}
	//for (auto it = market_connections.begin(); it != market_connections.end(); ++it)
	//{
	//	it->second->subscribe(contracts);
	//}
	if (market_session.get() != NULL)
	{
		market_session->subscribe(contracts);
	}
}

void SpiderCommonApi::marketUnSubscribe(char *codes[], int count)
{
	std::vector<std::string> contracts;
	for (int i = 0; i < count; i++)
	{
		contracts.push_back(codes[i]);
	}
	//for (auto it = market_connections.begin(); it != market_connections.end(); ++it)
	//{
	//	it->second->unsubscribe(contracts);
	//}
	if (market_session.get() != NULL)
	{
		market_session->unsubscribe(contracts);
	}
}

void SpiderCommonApi::registerSpi(SpiderSpi *pSpi)
{
	notifySpi = pSpi;
}


void SpiderCommonApi::tradeStart()
{
	if (trade_running)
	{
		return;
	}
	trade_running = true;
	//for (auto it = trade_connections.begin(); it != trade_connections.end(); ++it)
	//{
	//	it->second->start();
	//}
	if (trade_session.get() != NULL)
	{
		trade_session->start();
	}
}
void SpiderCommonApi::tradeStop()
{
	if (!trade_running)
	{
		return;
	}
	//for (auto it = trade_connections.begin(); it != trade_connections.end(); ++it)
	//{
	//	it->second->stop();
	//}
	if (trade_session.get() != NULL)
	{
		trade_session->stop();
	}
	trade_running = false;
}
void SpiderCommonApi::tradeRestart()
{
	tradeStop();
	tradeStart();
}

const char * SpiderCommonApi::InsertOrder(OrderInsert * order)
{
	if (trade_session.get() != NULL)
	{
		return trade_session->insert_order(order);
	}
	else {
		return "-1";
	}
}
void SpiderCommonApi::CancelOrder(OrderCancel * order)
{
	if (trade_session.get() != NULL)
	{
		trade_session->cancel_order(order);
	}
}

int SpiderCommonApi::getAllAccounts(AccountInfo ** &out_account_list)
{
	int account_size = my_config.future_trade_account_list.size() + my_config.future_market_account_list.size() + my_config.index_market_account_list.size();
	AccountInfo * * account_list = new AccountInfo *[account_size];
	int _ai = 0;
	for (auto it = my_config.future_trade_account_list.begin(); it != my_config.future_trade_account_list.end(); ++it)
	{
		AccountInfo * account = new AccountInfo(it->second);
		account->account_type = account->account_type / 100; //把AccountDetailType转换为AccountType
		account_list[_ai] = account;
		++_ai;
	}
	for (auto it = my_config.future_market_account_list.begin(); it != my_config.future_market_account_list.end(); ++it)
	{
		AccountInfo * account = new AccountInfo(it->second);
		account->account_type = account->account_type / 100;
		account_list[_ai] = account;
		++_ai;
	}
	for (auto it = my_config.index_market_account_list.begin(); it != my_config.index_market_account_list.end(); ++it)
	{
		AccountInfo * account = new AccountInfo(it->second);
		account->account_type = account->account_type / 100;
		account_list[_ai] = account;
		++_ai;
	}
	out_account_list = account_list;
	//LOGD(out_account_list<<", "<< account_size);
	//这里不释放，使用的地方释放指针
	return _ai;
}

void SpiderCommonApi::QueryTradingAccount(int requestID)
{
	if (trade_session.get() != NULL)
	{
		trade_session->query_trading_account(requestID);
	}
}
void SpiderCommonApi::QueryPositions(int requestID)
{
	if (trade_session.get() != NULL)
	{
		trade_session->query_positions(requestID);
	}
}
void SpiderCommonApi::QueryOrders(int requestID)
{
	if (trade_session.get() != NULL)
	{
		trade_session->query_orders(requestID);
	}
}
void SpiderCommonApi::QueryTrades(int requestID)
{
	if (trade_session.get() != NULL)
	{
		trade_session->query_trades(requestID);
	}
}