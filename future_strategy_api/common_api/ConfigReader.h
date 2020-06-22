#pragma once
#include "Utility.h"
#include "LogWrapper.h"
#include "cJsonWrapper.h"
#include "SpiderApiStruct.h"

using namespace raptor;


enum EnumAccountDetailType
{
	AccountTradeFutureCTP = 101,
	AccountTradeFutureEES = 102,
	AccountMarketFutureCTP = 201,
	AccountMarketFutureEES = 202,
	AccountMarketFutureMulticastDZ = 203, //东证期货提供的中金Level2期货组播行情
	AccountMarketFutureCFFEXTCP = 204, //中信期货提供的中金Level2期货tcp行情接口
	AccountMarketFutureCFFEXUDP = 205, //我们自己的udp期货行情格式
	AccountMarketIndexMulticast = 301,
	AccountMarketIndexMulticastDZ = 302, //东证期货提供的指数组播行情
};

static const int URILIST_MAX_SIZE = 5;

class ConfigReader 
{

public:

	bool is_test;

	std::map<std::string, AccountInfo > future_trade_account_list;
	std::map<std::string, AccountInfo> future_market_account_list;
	std::map<std::string, AccountInfo> index_market_account_list;

	/*********************************************************************************/
	ConfigReader()
		:is_test(false)
	{

	}

	bool init(){return read("./config","config.json");}

	bool read(const std::string& dir, const std::string& filename){
		std::shared_ptr<char> buffer = readFile(dir.c_str(), filename.c_str());
		if (buffer.get()==NULL)
		{
			LOGE("can not read config dir:"<<dir<<" filename:"<<filename);
			return false;
		}

		raptor::CJsonParser parser;
		cJSON* poi = parser.parse(buffer.get());

		if (poi == NULL || poi->type!=cJSON_Object)
		{
			LOGE("错误的配置文件格式");
			return false;
		}
		cJSON *accountArray1 = NULL;
		cJSON *accountArray2 = NULL;
		cJSON *accountArray3 = NULL;
		for (cJSON *exp = poi->child; exp!=NULL; exp=exp->next)
		{
			if (strcmp(exp->string, "is_test") == 0 && (exp->type == cJSON_False || exp->type == cJSON_True)) {
				is_test = exp->type == cJSON_True;
			}
			else if (strcmp(exp->string, "future_trade_accounts") == 0 && exp->type == cJSON_Array) {
				accountArray1 = exp;
			} 
			else if (strcmp(exp->string, "future_market_accounts") == 0 && exp->type == cJSON_Array) {
				accountArray2 = exp;
			}
			else if (strcmp(exp->string, "index_market_accounts") == 0 && exp->type == cJSON_Array) {
				accountArray3 = exp;
			}
			else {
				LOGW("Unexpected element in file:"<<exp->string);
				continue;
			}
		}
		//读取期货交易账户
		if (accountArray1 != NULL)
		{
			for (cJSON *aka = accountArray1->child; aka != NULL; aka = aka->next)
			{
				AccountInfo _ai;
				memset(&_ai, 0, sizeof(AccountInfo));
				cJSON *subArray1 = NULL;
				for (cJSON *ak = aka->child; ak != NULL; ak = ak->next)
				{
					if (strcmp(ak->string, "account_type") == 0 && ak->type == cJSON_Number)
						_ai.account_type = ak->valueint;
					else if (strcmp(ak->string, "broker_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.broker_id, ak->valuestring,sizeof(_ai.broker_id) -1 );
					else if (strcmp(ak->string, "front_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.front_id, ak->valuestring, sizeof(_ai.front_id) - 1);
					else if (strcmp(ak->string, "session_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.session_id, ak->valuestring, sizeof(_ai.session_id) - 1);
					else if (strcmp(ak->string, "account_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.account_id, ak->valuestring, sizeof(_ai.account_id) - 1);
					else if (strcmp(ak->string, "password") == 0 && ak->type == cJSON_String)
						memcpy(_ai.password, ak->valuestring, sizeof(_ai.password) - 1);
					else if (strcmp(ak->string, "product_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.product_id, ak->valuestring, sizeof(_ai.product_id) - 1);
					else if (strcmp(ak->string, "app_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.app_id, ak->valuestring, sizeof(_ai.app_id) - 1);
					else if (strcmp(ak->string, "auth_code") == 0 && ak->type == cJSON_String)
						memcpy(_ai.auth_code, ak->valuestring, sizeof(_ai.auth_code) - 1);
					//else if (strcmp(ak->string, "local_mac") == 0 && ak->type == cJSON_String)
					//	memcpy(_ai.local_mac, ak->valuestring, sizeof(_ai.local_mac) - 1);
					else if (strcmp(ak->string, "uris") == 0 && ak->type == cJSON_Array)
						subArray1 = ak;
					else
						continue;
				}
				if (subArray1 != NULL)
				{
					int ncount = 0;
					for (cJSON *aka2 = subArray1->child; aka2 != NULL; aka2 = aka2->next)
					{
						memcpy(_ai.uri_list[ncount], aka2->valuestring, sizeof(_ai.uri_list[ncount]) - 1);
						if (++ncount >= URILIST_MAX_SIZE) //目前只支持5个url
							break;
					}
				}
				future_trade_account_list.insert(std::make_pair(_ai.account_id,_ai));
			}
		}
		//读取期货行情账户
		if (accountArray2!= NULL)
		{
			for (cJSON *aka = accountArray2->child; aka != NULL; aka = aka->next)
			{
				AccountInfo _ai;
				memset(&_ai, 0, sizeof(AccountInfo));
				cJSON *subArray1 = NULL;
				for (cJSON *ak = aka->child; ak != NULL; ak = ak->next)
				{
					if (strcmp(ak->string, "account_type") == 0 && ak->type == cJSON_Number)
						_ai.account_type = ak->valueint;
					else if (strcmp(ak->string, "broker_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.broker_id, ak->valuestring, sizeof(_ai.broker_id) - 1);
					else if (strcmp(ak->string, "front_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.front_id, ak->valuestring, sizeof(_ai.front_id) - 1);
					else if (strcmp(ak->string, "session_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.session_id, ak->valuestring, sizeof(_ai.session_id) - 1);
					else if (strcmp(ak->string, "account_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.account_id, ak->valuestring, sizeof(_ai.account_id) - 1);
					else if (strcmp(ak->string, "password") == 0 && ak->type == cJSON_String)
						memcpy(_ai.password, ak->valuestring, sizeof(_ai.password) - 1);
					else if (strcmp(ak->string, "product_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.product_id, ak->valuestring, sizeof(_ai.product_id) - 1);
					else if (strcmp(ak->string, "app_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.app_id, ak->valuestring, sizeof(_ai.app_id) - 1);
					else if (strcmp(ak->string, "auth_code") == 0 && ak->type == cJSON_String)
						memcpy(_ai.auth_code, ak->valuestring, sizeof(_ai.auth_code) - 1);
					//else if (strcmp(ak->string, "local_mac") == 0 && ak->type == cJSON_String)
					//	memcpy(_ai.local_mac, ak->valuestring, sizeof(_ai.local_mac) - 1);
					else if (strcmp(ak->string, "uris") == 0 && ak->type == cJSON_Array)
						subArray1 = ak;
					else
						continue;
				}
				if (subArray1 != NULL)
				{
					int ncount = 0;
					for (cJSON *aka2 = subArray1->child; aka2 != NULL; aka2 = aka2->next)
					{
						memcpy(_ai.uri_list[ncount], aka2->valuestring, sizeof(_ai.uri_list[ncount]) - 1);
						if (++ncount >= URILIST_MAX_SIZE) //目前只支持5个url
							break;
					}
				}
				future_market_account_list.insert(std::make_pair(_ai.account_id, _ai));
			}
		}
		//读取指数行情账户
		if (accountArray3 != NULL)
		{
			for (cJSON *aka = accountArray3->child; aka != NULL; aka = aka->next)
			{
				AccountInfo _ai;
				memset(&_ai,0,sizeof(AccountInfo));
				cJSON *subArray1 = NULL;
				for (cJSON *ak = aka->child; ak != NULL; ak = ak->next)
				{
					if (strcmp(ak->string, "account_type") == 0 && ak->type == cJSON_Number)
						_ai.account_type = ak->valueint;
					else if (strcmp(ak->string, "broker_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.broker_id, ak->valuestring, sizeof(_ai.broker_id) - 1);
					else if (strcmp(ak->string, "front_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.front_id, ak->valuestring, sizeof(_ai.front_id) - 1);
					else if (strcmp(ak->string, "session_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.session_id, ak->valuestring, sizeof(_ai.session_id) - 1);
					else if (strcmp(ak->string, "account_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.account_id, ak->valuestring, sizeof(_ai.account_id) - 1);
					else if (strcmp(ak->string, "password") == 0 && ak->type == cJSON_String)
						memcpy(_ai.password, ak->valuestring, sizeof(_ai.password) - 1);
					else if (strcmp(ak->string, "product_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.product_id, ak->valuestring, sizeof(_ai.product_id) - 1);
					else if (strcmp(ak->string, "app_id") == 0 && ak->type == cJSON_String)
						memcpy(_ai.app_id, ak->valuestring, sizeof(_ai.app_id) - 1);
					else if (strcmp(ak->string, "auth_code") == 0 && ak->type == cJSON_String)
						memcpy(_ai.auth_code, ak->valuestring, sizeof(_ai.auth_code) - 1);
					//else if (strcmp(ak->string, "local_mac") == 0 && ak->type == cJSON_String)
					//	memcpy(_ai.local_mac, ak->valuestring, sizeof(_ai.local_mac) - 1);
					else if (strcmp(ak->string, "uris") == 0 && ak->type == cJSON_Array)
						subArray1 = ak;
					else
						continue;
				}
				if (subArray1 != NULL)
				{
					int ncount = 0;
					for (cJSON *aka2 = subArray1->child; aka2 != NULL; aka2 = aka2->next)
					{
						memcpy(_ai.uri_list[ncount], aka2->valuestring, sizeof(_ai.uri_list[ncount]) - 1);
						if (++ncount >= URILIST_MAX_SIZE) //目前只支持5个url
							break;
					}
				}
				index_market_account_list.insert(std::make_pair(_ai.account_id, _ai));
			}
		}
	

		LOGI("读取配置文件成功，共读取"<<(future_trade_account_list.size() + future_market_account_list.size() + index_market_account_list.size())<<"个账户。");
		return true;
	}
};


