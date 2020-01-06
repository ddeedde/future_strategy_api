#include "MultiIndexDZ.h"
#include <algorithm>
#include "boost/bind.hpp"
#include "LogWrapper.h"
#include "CommonDefine.h"
#include "Utility.h"
#include "FutureApi.h"


SpiderMultiIndexDZSpi::SpiderMultiIndexDZSpi()
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

SpiderMultiIndexDZSpi::~SpiderMultiIndexDZSpi()
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

bool SpiderMultiIndexDZSpi::init(SpiderMultiIndexDZSession * sm)
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

void SpiderMultiIndexDZSpi::start()
{
	io_thread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io)));

	start_timer.expires_from_now(boost::posix_time::seconds(1));
	start_timer.async_wait(boost::bind(&SpiderMultiIndexDZSpi::on_start, this, _1));
}

void SpiderMultiIndexDZSpi::on_start(const boost::system::error_code& error)
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

void SpiderMultiIndexDZSpi::async_receive()
{
	memset(m_data, 0, sizeof(m_data));
	mul_socket.async_receive_from(
		boost::asio::buffer(m_data, sizeof(m_data)), mul_sender_endpoint,
		boost::bind(&SpiderMultiIndexDZSpi::handle_receive_from, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
	);
}

void SpiderMultiIndexDZSpi::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
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
	//LOGD(bytes_recvd<<", "<< sizeof(Index_DZ::MarketDataField));
	if (bytes_recvd == sizeof(Index_DZ::MarketDataField))
	{
		if (recv_count++ % 2000 == 0)
		{
			LOGD("SpiderMultiIndexDZSpi received: " << recv_count);
		}
		if (!strncmp(m_data,"I",8)) //MDStreamID为'E'股票 'O'期权 'W'权证 'B'债券 'I'指数 'F' 基金
		{
			Index_DZ::MarketDataField _index;
			memcpy(&_index, m_data, bytes_recvd);
			QuotaData * md = new QuotaData();
			md->ExchangeID = get_exid_from_index_mul(_index.data.snapshot.SecurityExchange);
			md->UpdateMillisec = 0;
			memcpy(md->TradingDay, getTodayString(), sizeof(md->TradingDay) - 1);
			memcpy(md->Code, _index.data.snapshot.SecurityID, sizeof(md->Code) - 1);
			memcpy(md->UpdateTime, getNowString(), sizeof(md->UpdateTime) - 1);
			md->AskPrice1 = 0;
			md->AskVolume1 = 0;
			md->BidPrice1 = 0;
			md->BidVolume1 = 0;
			md->LastPrice = _index.data.snapshot.LastPx;
			md->HighestPrice = _index.data.snapshot.HighPx;
			md->LowestPrice = _index.data.snapshot.LowPx;
			md->LowerLimitPrice = _index.data.snapshot.StaticInfo.LowLimitPx;
			md->UpperLimitPrice = _index.data.snapshot.StaticInfo.HighLimitPx;
			md->OpenPrice = _index.data.snapshot.StaticInfo.OpenPx;
			md->PreClosePrice = _index.data.snapshot.StaticInfo.PrevClosePx;
			md->ClosePrice = _index.data.snapshot.StaticInfo.ClosePx;
			md->PreSettlementPrice = 0;
			md->SettlementPrice = 0;
			md->PreOpenInterest = 0;
			md->OpenInterest = 0;
			md->Turnover = _index.data.snapshot.TotalValueTraded;
			md->Volume = _index.data.snapshot.TotalVolumeTraded;

			if (smd)
			{
				smd->on_receive_data(md);
			}
			LOGD("ees index:" << md->Code<<","<< md->LastPrice <<","<< md->Volume <<","<< get_not_microsec()<<":"<< _index.data.snapshot.LastUpdateTime<<","<< _index.data.snapshot.StaticInfo.PrevClosePx); //just for test
		}
		//else {
		//	LOGD(std::string(m_data,8));
		//}
	}

	async_receive();
}


long long SpiderMultiIndexDZSpi::get_not_microsec()
{
	boost::posix_time::ptime tm = boost::posix_time::microsec_clock::local_time();
	return (long long)tm.time_of_day().total_microseconds();
}


//*********************************************************

SpiderMultiIndexDZSession::SpiderMultiIndexDZSession(SpiderCommonApi * sci, AccountInfo & ai)
	:BaseMarketSession(sci, ai)
{
	marketConnection.reset(new SpiderMultiIndexDZSpi());
}

SpiderMultiIndexDZSession::~SpiderMultiIndexDZSession()
{

}

bool SpiderMultiIndexDZSession::init()
{
	return marketConnection->init(this);
}

void SpiderMultiIndexDZSession::start()
{
	marketConnection->start();
}

void SpiderMultiIndexDZSession::stop()
{

}

void SpiderMultiIndexDZSession::subscribe(std::vector<std::string> & list)
{
	for (unsigned int i = 0; i < list.size(); ++i)
	{
		updateSubscribedList(list[i].c_str());
	}
}

void SpiderMultiIndexDZSession::unsubscribe(std::vector<std::string> & list)
{
	for (unsigned int i = 0; i < list.size(); ++i)
	{
		updateSubscribedList(list[i].c_str(), true);
	}
}

void SpiderMultiIndexDZSession::on_connected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnConnect();
		}
	}
}

void SpiderMultiIndexDZSession::on_disconnected()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnDisconnect();
		}
	}
}

void SpiderMultiIndexDZSession::on_log_in()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogin(get_account().account_id, get_account().account_type / 100);
		}
	}
}

void SpiderMultiIndexDZSession::on_log_out()
{
	if (spider_common_api != NULL)
	{
		if (spider_common_api->notifySpi != NULL)
		{
			spider_common_api->notifySpi->marketOnLogout();
		}
	}
}

void SpiderMultiIndexDZSession::on_error(std::shared_ptr<SpiderErrorMsg> & err_msg)
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

void SpiderMultiIndexDZSession::on_receive_data(QuotaData * md)
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

