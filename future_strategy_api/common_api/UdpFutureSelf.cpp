#include "UdpFutureSelf.h"
#include <algorithm>
#include "LogWrapper.h"
#include "CommonDefine.h"
#include "Utility.h"
#include "FutureApi.h"




SpiderUdpFutureSpi::SpiderUdpFutureSpi()
	: smd(NULL)
	, io_keeper(new boost::asio::io_service::work(io))
	, udp_socket(io)
	, ping_timer(io)
	, udp_ip("")
	, udp_port(0)
{

}

SpiderUdpFutureSpi::~SpiderUdpFutureSpi()
{
	if (io_keeper)
	{
		delete io_keeper;
		io_keeper = NULL;
	}
	ping_timer.cancel();
	if (udp_socket.is_open())
	{
		boost::system::error_code ignored_ec;
		udp_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		udp_socket.close();
	}
	io.stop();
	if (io_thread.get())
	{
		io_thread->join();
	}
}

bool SpiderUdpFutureSpi::init(SpiderUdpFutureSession * sm)
{
	smd = sm;

	if (strlen(sm->get_account().uri_list[0]) <= 0)
	{
		LOGE("配置文件中UDP地址不能为空");
		return false;
	}
	std::vector<std::string> ips;
	split_str(sm->get_account().uri_list[0], ips, ":");
	if (ips.size() != 3)
	{
		LOGE("配置文件中UDP地址格式错误：udp://127.0.0.1:31001");
		return false;
	}
	replace_all(ips[1], "/", "");
	udp_ip = ips[1];
	udp_port = atoi(ips[2].c_str());
	try
	{
		LOGI("connect to udp server : " << udp_ip << ":" << udp_port);
		udp_socket.open(boost::asio::ip::udp::v4());
		udp_socket.set_option(boost::asio::socket_base::receive_buffer_size(1024 * 8000));
		udp_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
		udp_sender_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string(udp_ip), udp_port);
	}
	catch (const std::exception& e)
	{
		LOGE("UDP启动异常：" << e.what());
		return false;
	}
	return true;
}

void SpiderUdpFutureSpi::start()
{
	io_thread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io)));

	io.post(boost::bind(&SpiderUdpFutureSpi::on_start, this));

	//开始收数据
	async_receive();

	//开始心跳，如果长时间没有心跳，服务端会断开连接
	ping();

}

void SpiderUdpFutureSpi::ping()
{
	send("ping");

	ping_timer.expires_from_now(boost::posix_time::seconds(5));
	ping_timer.async_wait(
		[this](const boost::system::error_code &ec) 
		{
			if (ec)
				return;

			ping();
		}
	);
}

void SpiderUdpFutureSpi::on_start()
{
	if (smd)
	{
		smd->on_connected();
		smd->on_log_in();
	}
	async_receive();
}

void SpiderUdpFutureSpi::send(const std::string & msg)
{
	boost::shared_ptr<std::string> message(new std::string(msg));
	udp_socket.async_send_to(boost::asio::buffer(message->data(),message->size()),udp_sender_endpoint,
		[=](const boost::system::error_code &ec, std::size_t bytes){});
		//boost::bind(&SpiderUdpFutureSpi::on_send,this,_1,_2));
}

void SpiderUdpFutureSpi::on_send(const boost::system::error_code &ec, std::size_t bytes)
{
	//todo
}

void SpiderUdpFutureSpi::async_receive()
{
	memset(m_data, 0, sizeof(m_data));
	udp_socket.async_receive_from(
		boost::asio::buffer(m_data, sizeof(m_data)), udp_recv_endpoint,
		boost::bind(&SpiderUdpFutureSpi::handle_receive_from, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
	);
}

void SpiderUdpFutureSpi::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
{
	if (! error)
	{
		if (bytes_recvd == sizeof(MarketDataFuture))
		{
			if (recv_count++ % 2000 == 0)
			{
				LOGD("SpiderUdpFutureSpi received: " << recv_count);
			}
			MarketDataFuture * _index = (MarketDataFuture*)m_data;

			QuotaData * md = new QuotaData();
			md->ExchangeID = (int)EnumExchangeIDType::CFFEX;
			md->UpdateMillisec = get_not_microsec();
			memcpy(md->TradingDay, getTodayString(), sizeof(md->TradingDay) - 1);
			memcpy(md->Code, _index->SecID, sizeof(md->Code) - 1);
			int _hour = _index->Time / 10000000;
			int _minite = _index->Time / 100000 - _hour * 100;
			int _second = (_index->Time / 1000) % 100;
			sprintf_s(md->UpdateTime, sizeof(md->UpdateTime) - 1, "%.2d:%.2d:%.2d", _hour, _minite, _second); //维持 09:30:00 这样的格式

			md->AskPrice1 = (double)_index->AskPrice / 10000;
			md->AskVolume1 = _index->AskVol;
			md->BidPrice1 = (double)_index->BidPrice / 10000;
			md->BidVolume1 = _index->BidVol;
			md->LastPrice = (double)_index->Match / 10000;
			md->HighestPrice = (double)_index->High / 10000;
			md->LowestPrice = (double)_index->Low / 10000;
			md->LowerLimitPrice = (double)_index->LowLimited / 10000;
			md->UpperLimitPrice = (double)_index->HighLimited / 10000;
			md->OpenPrice = (double)_index->Open / 10000;
			md->PreClosePrice = (double)_index->PreClose / 10000;
			md->ClosePrice = (double)_index->Close / 10000;
			md->PreSettlementPrice = (double)_index->PreSettlement / 10000;
			md->SettlementPrice = (double)_index->Settlement / 10000;
			md->PreOpenInterest = (double)_index->PreOpenInterest;
			md->OpenInterest = (double)_index->OpenInterest;
			md->Turnover = (double)_index->Turnover;
			md->Volume = _index->Volume;

			if (smd)
			{
				smd->on_receive_data(md);
			}
			LOGD("my udp future:" << md->Code << "," << md->AskPrice1 << "," << md->AskVolume1 << "," << md->BidPrice1 << "," << md->BidVolume1 << "," << md->LastPrice << "," << md->Turnover << "," << md->Volume); //just for test
		}
	}
	else {
		if (error == boost::asio::error::operation_aborted) 
		{
			LOGI("udp is canceled, ip=" << udp_ip << ", port=" << udp_port << ", errorCode=" << error.value());
			return;
		}
		LOGE("receive udp packet error:" << error.value() << "--" << error.message());

		//可能会很频繁，所以干脆不发送
		//if (smd)
		//{
		//	smd->on_log_out();
		//	smd->on_disconnected();
		//}

	}
	
	async_receive();
}

long long SpiderUdpFutureSpi::get_not_microsec()
{
	boost::posix_time::ptime tm = boost::posix_time::microsec_clock::local_time();
	return (long long)tm.time_of_day().total_microseconds();
}



//*********************************************************

SpiderUdpFutureSession::SpiderUdpFutureSession(SpiderCommonApi * sci, AccountInfo & ai)
	:BaseMarketSession(sci, ai)
{
	marketConnection.reset(new SpiderUdpFutureSpi());
}

SpiderUdpFutureSession::~SpiderUdpFutureSession()
{

}

bool SpiderUdpFutureSession::init()
{
	return marketConnection->init(this);
}

void SpiderUdpFutureSession::start()
{
	marketConnection->start();
}

void SpiderUdpFutureSession::stop()
{

}

void SpiderUdpFutureSession::subscribe(std::vector<std::string> & list)
{
	for (unsigned int i = 0; i < list.size(); ++i)
	{
		updateSubscribedList(list[i].c_str());
	}
}

void SpiderUdpFutureSession::unsubscribe(std::vector<std::string> & list)
{
	for (unsigned int i = 0; i < list.size(); ++i)
	{
		updateSubscribedList(list[i].c_str(), true);
	}
}

void SpiderUdpFutureSession::on_connected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnConnect();
		}
	}
}

void SpiderUdpFutureSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnDisconnect();
		}
	}
}

void SpiderUdpFutureSession::on_log_in()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderUdpFutureSession::on_log_out()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogout();
		}
	}
}

void SpiderUdpFutureSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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

void SpiderUdpFutureSession::on_receive_data(QuotaData * md)
{
	if (!ifSubscribed(md->Code))
	{
		return;
	}
	if (spider_common_api != NULL)
	{
		spider_common_api->onQuote(md);
	}
}

