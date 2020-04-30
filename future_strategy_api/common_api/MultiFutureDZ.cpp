#include "MultiFutureDZ.h"
#include <algorithm>
#include "boost/bind.hpp"
#include "LogWrapper.h"
#include "CommonDefine.h"
#include "Utility.h"
#include "FutureApi.h"


SpiderMultiFutureDZSpi::SpiderMultiFutureDZSpi()
	: smd(NULL)
	, io_keeper(new boost::asio::io_service::work(io))
	, mul_socket(io)
	, start_timer(io)
	, mul_ip("")
	, mul_port(0)
	, local_ip("0.0.0.0")
	, recv_count(0)
{

}

SpiderMultiFutureDZSpi::~SpiderMultiFutureDZSpi()
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

bool SpiderMultiFutureDZSpi::init(SpiderMultiFutureDZSession * sm)
{
	smd = sm;
	//LOGI("AA:"<<sizeof(Future_DZ::level2_future_zj));

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
	replace_all(ips[1], "/", "");
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
		LOGE("组播启动异常：" << e.what());
		return false;
	}
	return true;
}

void SpiderMultiFutureDZSpi::start()
{
	io_thread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io)));

	start_timer.expires_from_now(boost::posix_time::seconds(1));
	start_timer.async_wait(boost::bind(&SpiderMultiFutureDZSpi::on_start, this, _1));
}

void SpiderMultiFutureDZSpi::on_start(const boost::system::error_code& error)
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

void SpiderMultiFutureDZSpi::async_receive()
{
	memset(m_data, 0, sizeof(m_data));
	mul_socket.async_receive_from(
		boost::asio::buffer(m_data, sizeof(m_data)), mul_sender_endpoint,
		boost::bind(&SpiderMultiFutureDZSpi::handle_receive_from, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
	);
}

void SpiderMultiFutureDZSpi::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
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
	//LOGD(bytes_recvd<<", "<< sizeof(Future_DZ::level2_future_zj));
	if (bytes_recvd == sizeof(Future_DZ::level2_future_zj))
	{
		if (recv_count++ % 2000 == 0)
		{
			LOGD("SpiderMultiFutureDZSpi received: " << recv_count);
		}

		Future_DZ::level2_future_zj _index;
		memcpy(&_index, m_data, bytes_recvd);
		QuotaData * md = new QuotaData();
		md->ExchangeID = (int)EnumExchangeIDType::CFFEX;
		md->UpdateMillisec = spider_swap32(_index.modify_milisec);
		memcpy(md->TradingDay, getTodayString(), sizeof(md->TradingDay) - 1);
		memcpy(md->Code, _index.insid, sizeof(md->Code) - 1);
		memcpy(md->UpdateTime, _index.modify_time, sizeof(md->UpdateTime) - 1);

		md->AskPrice1 = spider_swapdouble(decrypt_strategy_double_1(_index.pankou[0].ask_price));
		md->AskVolume1 = spider_swap32(decrypt_strategy_int_1(_index.pankou[0].ask_vol));
		md->BidPrice1 = spider_swapdouble(decrypt_strategy_double_1(_index.pankou[0].bid_price));
		md->BidVolume1 = spider_swap32(decrypt_strategy_int_1(_index.pankou[0].bid_vol));

		md->LastPrice = spider_swapdouble(decrypt_strategy_double_1(_index.latest_price));

		md->HighestPrice = spider_swapdouble(_index.high_price);
		md->LowestPrice = spider_swapdouble(_index.low_price);
		md->LowerLimitPrice = spider_swapdouble(_index.low_limit_price);
		md->UpperLimitPrice = spider_swapdouble(_index.high_limit_price);
		md->OpenPrice = spider_swapdouble(_index.open_price);
		md->PreClosePrice = 0;
		md->ClosePrice = spider_swapdouble(_index.close_price);
		md->PreSettlementPrice = 0;
		md->SettlementPrice = spider_swapdouble(_index.settle_price);
		md->PreOpenInterest = 0;
		md->OpenInterest = spider_swapdouble(decrypt_strategy_double_1(_index.open_interest));
		md->Turnover = spider_swapdouble(decrypt_strategy_double_1(_index.turnover));
		md->Volume = spider_swap32(decrypt_strategy_int_1(_index.volume));

		if (smd)
		{
			smd->on_receive_data(md);
		}
		LOGD("ees future:" << md->Code << "," << md->LastPrice << "," << md->AskPrice1 << "," << md->AskVolume1 << "," << md->BidVolume1 << "," << md->BidPrice1 <<  "," << md->UpperLimitPrice << "," << md->Volume << "," << md->Turnover << "," << _index.modify_time << "," << md->UpdateMillisec); //just for test

	}

	async_receive();
}


long long SpiderMultiFutureDZSpi::get_not_microsec()
{
	boost::posix_time::ptime tm = boost::posix_time::microsec_clock::local_time();
	return (long long)tm.time_of_day().total_microseconds();
}


//*********************************************************

SpiderMultiFutureDZSession::SpiderMultiFutureDZSession(SpiderCommonApi * sci, AccountInfo & ai)
	:BaseMarketSession(sci, ai)
{
	marketConnection.reset(new SpiderMultiFutureDZSpi());
}

SpiderMultiFutureDZSession::~SpiderMultiFutureDZSession()
{

}

bool SpiderMultiFutureDZSession::init()
{
	return marketConnection->init(this);
}

void SpiderMultiFutureDZSession::start()
{
	marketConnection->start();
}

void SpiderMultiFutureDZSession::stop()
{

}

void SpiderMultiFutureDZSession::subscribe(std::vector<std::string> & list)
{
	for (unsigned int i = 0; i < list.size(); ++i)
	{
		updateSubscribedList(list[i].c_str());
	}
}

void SpiderMultiFutureDZSession::unsubscribe(std::vector<std::string> & list)
{
	for (unsigned int i = 0; i < list.size(); ++i)
	{
		updateSubscribedList(list[i].c_str(), true);
	}
}

void SpiderMultiFutureDZSession::on_connected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnConnect();
		}
	}
}

void SpiderMultiFutureDZSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnDisconnect();
		}
	}
}

void SpiderMultiFutureDZSession::on_log_in()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderMultiFutureDZSession::on_log_out()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogout();
		}
	}
}

void SpiderMultiFutureDZSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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

void SpiderMultiFutureDZSession::on_receive_data(QuotaData * md)
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

