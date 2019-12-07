#include "MdApi.h"


void BaseMarketSession::updateSubscribedList(const char * ins, bool del)
{
	if (ins != NULL)
	{
		std::unique_lock<std::mutex> l(subscribed_symbol_mutex);
		if (del)
		{
			subscribed_symbol_list.erase(std::string(ins));
		}
		else {
			subscribed_symbol_list.insert(std::string(ins));
		}
	}
}

bool BaseMarketSession::ifSubscribed(const char * ins)
{
	std::unique_lock<std::mutex> l(subscribed_symbol_mutex);
	if (subscribed_symbol_list.find(ins) == subscribed_symbol_list.end()) //不在订阅列表中的直接丢弃掉
	{
		return false;
	}
	else {
		return true;
	}
}

std::vector<std::string> BaseMarketSession::get_subscribed_list()
{
	std::vector<std::string> _list;
	std::unique_lock<std::mutex> l(subscribed_symbol_mutex);
	for (auto it = subscribed_symbol_list.begin(); it != subscribed_symbol_list.end(); ++it)
	{
		_list.push_back(*it);
	}
	return _list;
}