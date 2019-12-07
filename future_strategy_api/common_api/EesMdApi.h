#if !defined(SPIDER_EESMDAPI_H)
#define SPIDER_EESMDAPI_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*
盛立行情接口
*/

#include "MdApi.h"
#include "SpiderApiStruct.h"
#include "EESQuoteApi.h"


class SpiderEesMdSession;
class SpiderEesMdSpi : public EESQuoteEvent
{
	EESQuoteApi * userApi;

	SpiderEesMdSession * smd;

	AccountInfo myAccount;

	int recv_count;

	std::vector<EqsTcpInfo> tcp_list; //这两者同一时间只会有一个有数据
	std::vector<EqsMulticastInfo> multi_list; //这两者同一时间只会有一个有数据
public:
	EESQuoteApi * getUserApi() { return userApi; }
	bool init(SpiderEesMdSession * sm);
	void start();
	void stop();
	void login();
	bool if_multicast_mode() { return multi_list.size() > 0; }
public:
	SpiderEesMdSpi();
	~SpiderEesMdSpi();

	/// \brief 当服务器连接成功，登录前调用, 如果是组播模式不会发生, 只需判断InitMulticast返回值即可
	virtual void OnEqsConnected();

	/// \brief 当服务器曾经连接成功，被断开时调用，组播模式不会发生该事件
	virtual void OnEqsDisconnected();

	/// \brief 当登录成功或者失败时调用，组播模式不会发生
	/// \param bSuccess 登陆是否成功标志  
	/// \param pReason  登陆失败原因  
	virtual void OnLoginResponse(bool bSuccess, const char* pReason);

	/// \brief 收到行情时调用,具体格式根据instrument_type不同而不同
	/// \param chInstrumentType  EES行情类型
	/// \param pDepthQuoteData   EES统一行情指针  
	virtual void OnQuoteUpdated(EesEqsIntrumentType chInstrumentType, EESMarketDepthQuoteData* pDepthQuoteData);

	/// \brief 日志接口，让使用者帮助写日志。
	/// \param nlevel    日志级别
	/// \param pLogText  日志内容
	/// \param nLogLen   日志长度
	virtual void OnWriteTextLog(EesEqsLogLevel nlevel, const char* pLogText, int nLogLen);

	/// \brief 注册symbol响应消息来时调用，组播模式不支持行情注册
	/// \param chInstrumentType  EES行情类型
	/// \param pSymbol           合约名称
	/// \param bSuccess          注册是否成功标志
	virtual void OnSymbolRegisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess);

	/// \brief  注销symbol响应消息来时调用，组播模式不支持行情注册
	/// \param chInstrumentType  EES行情类型
	/// \param pSymbol           合约名称
	/// \param bSuccess          注册是否成功标志
	virtual void OnSymbolUnregisterResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bSuccess);

	/// \brief 查询symbol列表响应消息来时调用，组播模式不支持合约列表查询
	/// \param chInstrumentType  EES行情类型
	/// \param pSymbol           合约名称
	/// \param bLast             最后一条查询合约列表消息的标识
	/// \remark 查询合约列表响应, last = true时，本条数据是无效数据。
	virtual void OnSymbolListResponse(EesEqsIntrumentType chInstrumentType, const char* pSymbol, bool bLast) {}
};

class SpiderEesMdSession : public BaseMarketSession
{
public:
	SpiderEesMdSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderEesMdSession();

	virtual bool init();
	virtual void start();
	virtual void stop();

	void subscribe(std::vector<std::string> & list);
	void unsubscribe(std::vector<std::string> & list);

	virtual void on_connected();
	virtual void on_disconnected();
	virtual void on_log_in();
	virtual void on_log_out();
	virtual void on_error(std::shared_ptr<SpiderErrorMsg> & err_msg);
	virtual void on_receive_data(QuotaData * md);


private:
	std::shared_ptr<SpiderEesMdSpi> marketConnection;

};


















#endif