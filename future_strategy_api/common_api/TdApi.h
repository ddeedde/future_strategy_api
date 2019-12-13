#if !defined(SPIDER_TDAPI_H)
#define SPIDER_TDAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SpiderApiStruct.h"
#include <mutex>
#include <set>
#include <vector>

enum async_task_id
{
	async_task_unset = 0,
	async_task_login = 1,
	async_task_send_order_fail = 2,
};

struct async_task
{
	async_task_id task_id;
	void * task_data;
	async_task() :task_id(async_task_unset), task_data(NULL) {}
	async_task(async_task_id _id, void * _p) :task_id(_id), task_data(_p) {}
};

class SpiderCommonApi;
class BaseTradeSession
{
public:
	BaseTradeSession(SpiderCommonApi * sci, AccountInfo & ai)
		:spider_common_api(sci)
		, account_info(ai)
	{}

	virtual ~BaseTradeSession() {}

	virtual bool init() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;

	virtual const char * insert_order(OrderInsert *) = 0;
	virtual void cancel_order(OrderCancel *) = 0;

	virtual void on_connected() = 0;
	virtual void on_disconnected() = 0;
	virtual void on_log_in() = 0;
	virtual void on_log_out() = 0;
	virtual void on_error(std::shared_ptr<SpiderErrorMsg> & err_msg) = 0;

	virtual void on_order_change(OrderInfo *) = 0;
	virtual void on_trade_change(TradeInfo *) = 0;
	virtual void on_order_cancel(OrderInfo *) = 0;
	virtual void on_order_insert(OrderInfo *) = 0;

	virtual void query_trading_account(int) = 0;
	virtual void query_positions(int) = 0;
	virtual void query_orders(int) = 0;
	virtual void query_trades(int) = 0;
	virtual void on_rsp_query_account(TradingAccount * account, int request_id, bool last = false) = 0;
	virtual void on_rsp_query_position(InvestorPosition * position, int request_id, bool last = false) = 0;
	virtual void on_rsp_query_trade(TradeInfo * trade, int request_id, bool last = false) = 0;
	virtual void on_rsp_query_order(OrderInfo * order, int request_id, bool last = false) = 0;

public:
	AccountInfo & get_account() { return account_info; }

protected:
	AccountInfo account_info;
	SpiderCommonApi * spider_common_api;
};




#endif