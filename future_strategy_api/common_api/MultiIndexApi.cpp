#include "MultiIndexApi.h"
#include <algorithm>
#include "boost/bind.hpp"
#include "LogWrapper.h"
#include "CommonDefine.h"
#include "Utility.h"
#include "FutureApi.h"


SpiderMultiIndexSpi::SpiderMultiIndexSpi()
	: smd(NULL)
	, io_keeper(new boost::asio::io_service::work(io))
	, mul_socket(io)
	, start_timer(io)
	, mul_ip("")
	, mul_port(0)
	, recv_count(0)
{

}

SpiderMultiIndexSpi::~SpiderMultiIndexSpi()
{
	if (io_keeper)
	{
		delete io_keeper;
		io_keeper = NULL;
	}
	if (mul_socket.is_open())
	{
		boost::system::error_code ignored_ec;
		mul_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		mul_socket.close();
	}
	io.stop();
	if (io_thread.get())
	{
		io_thread->join();
	}
}

bool SpiderMultiIndexSpi::init(SpiderMultiIndexSession * sm)
{
	smd = sm;

	if (strlen(sm->get_account().uri_list[0]) <= 0)
	{
		LOGE("配置文件中组播地址不能为空");
		return false;
	}
	std::vector<std::string> ips;
	split_str(sm->get_account().uri_list[0], ips, ":");
	if (ips.size() != 4)
	{
		LOGE("配置文件中组播地址格式错误：mul://230.0.0.1:31001:192.168.79.56");
		return false;
	}
	replace_all(ips[1],"/", "");
	mul_ip = ips[1];
	mul_port = atoi(ips[2].c_str());
	local_ip = ips[3];
	try
	{
		LOGI("join multicast group : " << mul_ip << ":" << mul_port << " on interface " << local_ip);
		mul_socket.open(boost::asio::ip::udp::v4());
		mul_socket.set_option(boost::asio::socket_base::receive_buffer_size(1024 * 8000));
		mul_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
		mul_socket.set_option(boost::asio::ip::multicast::join_group(boost::asio::ip::address_v4(boost::asio::ip::address_v4::from_string(mul_ip)), 
																boost::asio::ip::address_v4(boost::asio::ip::address_v4::from_string(local_ip))));
		mul_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), mul_port));
	
	}
	catch (const std::exception& e)
	{
		LOGE("组播启动异常："<<e.what());
		return false;
	}
	return true;
}

void SpiderMultiIndexSpi::start()
{
	io_thread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io)));

	start_timer.expires_from_now(boost::posix_time::seconds(1));
	start_timer.async_wait(boost::bind(&SpiderMultiIndexSpi::on_start, this, _1));
}

void SpiderMultiIndexSpi::on_start(const boost::system::error_code& error)
{
	if (!error)
	{
		if (smd)
		{
			smd->on_connected();
			smd->on_log_in();
		}
		async_receive();
	}
}

void SpiderMultiIndexSpi::async_receive()
{
	memset(m_data, 0, sizeof(m_data));
	mul_socket.async_receive_from(
		boost::asio::buffer(m_data, sizeof(m_data)), mul_sender_endpoint,
		boost::bind(&SpiderMultiIndexSpi::handle_receive_from, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
		);
}

void SpiderMultiIndexSpi::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
{
	if (error)
	{
		if (error == boost::asio::error::operation_aborted) {
			LOGI("multicast is canceled, ip=" << mul_ip << ", port=" << mul_port << ", errorCode=" << error.value());
			return;
		}
		if (error.value() == 234 || error.value() == 10040) {
			LOGW("receive multicast buff size error:" << error.value() << "--" << error.message());
			//incase one packet is too big, ignore it and continue reading
			
			async_receive();
			return;
		}
		LOGE("receive multicast packet error:" << error.value() << "--" << error.message());

		if (smd)
		{
			smd->on_log_out();
			smd->on_disconnected();
		}		

		return;
	}

	if (bytes_recvd == sizeof(MarketDataIndex))
	{
		if (recv_count++ % 2000 == 0)
		{
			LOGI("SpiderMultiIndexSpi received: "<<recv_count);
		}

		//MarketDataIndex _index;
		//memcpy(&_index, m_data,bytes_recvd);
		MarketDataIndex * _index = (MarketDataIndex*)m_data;

		int _hour = _index->Time / 10000000;
		int _minite = _index->Time / 100000 - _hour * 100;
		int _second = (_index->Time / 1000) % 100;
		int _milisec = _index->Time % 1000;

		QuotaData * md = new QuotaData();
		md->ExchangeID = get_exid_from_index_mul(_index->ExID);
		md->UpdateMillisec = _milisec;
		memcpy(md->TradingDay, getTodayString(), sizeof(md->TradingDay) - 1);
		memcpy(md->Code, _index->SecID, sizeof(md->Code) - 1);
		sprintf_s(md->UpdateTime, sizeof(md->UpdateTime) - 1,"%.2d:%.2d:%.2d", _hour,_minite,_second); //维持 09:30:00 这样的格式
		md->AskPrice1 = 0;
		md->AskVolume1 = 0;
		md->BidPrice1 = 0;
		md->BidVolume1 = 0;
		md->LastPrice = (double)_index->Match / 10000;
		md->HighestPrice = (double)_index->High / 10000;
		md->LowestPrice = (double)_index->Low / 10000;
		md->LowerLimitPrice = (double)_index->LowLimited / 10000;
		md->UpperLimitPrice = (double)_index->HighLimited / 10000;
		md->OpenPrice = (double)_index->Open / 10000;
		md->PreClosePrice = (double)_index->PreClose / 10000;
		md->ClosePrice = 0;
		md->PreSettlementPrice = 0;
		md->SettlementPrice = 0;
		md->PreOpenInterest = 0;
		md->OpenInterest = 0;
		md->Turnover = (double)_index->Turnover;
		md->Volume = _index->Volume;
	
		if (smd)
		{
			bool _need_send = (_index->Type == 400) || smd->ifTest();
			if (_need_send)
			{
				smd->on_receive_data(md);
			}		
		}
		LOGD("my index:"<<md->Code << "," << md->LastPrice << "," << md->Volume << "," << md->UpdateTime << ":" << md->UpdateMillisec); //just for test
	}

	async_receive();
}

long long SpiderMultiIndexSpi::get_not_microsec()
{
	boost::posix_time::ptime tm = boost::posix_time::microsec_clock::local_time();
	return (long long)tm.time_of_day().total_microseconds();
}



//*********************************************************

SpiderMultiIndexSession::SpiderMultiIndexSession(SpiderCommonApi * sci, AccountInfo & ai)
	:BaseMarketSession(sci, ai)
{
	marketConnection.reset(new SpiderMultiIndexSpi());
}

SpiderMultiIndexSession::~SpiderMultiIndexSession()
{

}

bool SpiderMultiIndexSession::init()
{
	return marketConnection->init(this);
}

void SpiderMultiIndexSession::start()
{
	marketConnection->start();
}

void SpiderMultiIndexSession::stop()
{

}

void SpiderMultiIndexSession::subscribe(std::vector<std::string> & list)
{
	for (unsigned int i = 0; i < list.size(); ++i)
	{
		updateSubscribedList(list[i].c_str());
	}
}

void SpiderMultiIndexSession::unsubscribe(std::vector<std::string> & list)
{
	for (unsigned int i = 0; i < list.size(); ++i)
	{
		updateSubscribedList(list[i].c_str(),true);
	}
}

void SpiderMultiIndexSession::on_connected()
{
	if (spider_common_api != NULL)
	{
		//if (spider_common_api->callMarketConnect != NULL)
		//{
		//	spider_common_api->callMarketConnect();
		//}
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnConnect();
		}
	}
}

void SpiderMultiIndexSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{
		//if (spider_common_api->callMarketDisconnect != NULL)
		//{
		//	spider_common_api->callMarketDisconnect();
		//}
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnDisconnect();
		}
	}
}

void SpiderMultiIndexSession::on_log_in()
{
	if (spider_common_api != NULL)
	{
		//if (spider_common_api->callMarketLogin != NULL)
		//{
		//	spider_common_api->callMarketLogin(get_account().account_id);
		//}
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderMultiIndexSession::on_log_out()
{
	if (spider_common_api != NULL)
	{
		//if (spider_common_api->callMarketLogout != NULL)
		//{
		//	spider_common_api->callMarketLogout();
		//}
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogout();
		}
	}
}

void SpiderMultiIndexSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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

void SpiderMultiIndexSession::on_receive_data(QuotaData * md)
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

