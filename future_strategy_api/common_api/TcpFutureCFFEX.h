#if !defined(SPIDER_TCPFUTURECFFEX_H)
#define SPIDER_TCPFUTURECFFEX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MdApi.h"
#include "SpiderApiStruct.h"
#include "USTPFtdcMduserApi.h"

/*
2020-04-02
采用中金Lv2组播行情的api来收取行情，而不是镜像的方式;
中金的udp的api目前只有linux版本，但是tcp的有windows版本
*/



class SpiderTcpFutureCFFEXSession;
class SpiderTcpFutureCFFEXSpi : public CUstpFtdcMduserSpi
{
	// 指向CUstpFtdcMduserApi实例的指针
	CUstpFtdcMduserApi *m_pUserApi;

	SpiderTcpFutureCFFEXSession * smd;

	AccountInfo myAccount;

	int recv_count;
public:
	SpiderTcpFutureCFFEXSpi();
	~SpiderTcpFutureCFFEXSpi();

	bool init(SpiderTcpFutureCFFEXSession * sm);
	void start();
	void stop();
	void login();
	CUstpFtdcMduserApi * getUserApi() { return m_pUserApi; }

public:
	// 当客户端与行情发布服务器建立起通信连接，客户端需要进行登录
	virtual void OnFrontConnected();

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///错误应答
	virtual void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
	virtual void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///用户退出应答
	virtual void OnRspUserLogout(CUstpFtdcRspUserLogoutField *pRspUserLogout, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///订阅主题应答
	virtual void OnRspSubscribeTopic(CUstpFtdcDisseminationField *pDissemination, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	// 深度行情通知，行情服务器会主动通知客户端
	virtual void OnRtnDepthMarketData(CUstpFtdcDepthMarketDataField *pMarketData);

	///订阅合约的相关信息
	virtual void OnRspSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///退订合约的相关信息
	virtual void OnRspUnSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);


};




class SpiderTcpFutureCFFEXSession : public BaseMarketSession
{
public:
	SpiderTcpFutureCFFEXSession(SpiderCommonApi * sci, AccountInfo & ai);
	~SpiderTcpFutureCFFEXSession();

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
	std::shared_ptr<SpiderTcpFutureCFFEXSpi> marketConnection;
};












#endif