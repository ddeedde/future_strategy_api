#include "CommonDefine.h"
#include "Utility.h"
#include <mutex>


int g_seq = 0;
std::mutex g_seq_mutex;
long long g_last_get_ref_time = 0;
int g_local_ref = 0;
std::mutex g_ref_mutex;

int get_seq()
{
	std::unique_lock<std::mutex> l(g_seq_mutex);
	return ++g_seq;
}

const char * get_order_ref(const char * prefix)
{
	time_t rawtime;
	struct tm timeinfo;
	long long now = time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	//char order_ref[13] = {};
	static const int _char_size = 13;
	char * order_ref = new char[_char_size];
	memset(order_ref,0,sizeof(order_ref));
	g_ref_mutex.lock();
	if (now > g_last_get_ref_time)
	{
		g_last_get_ref_time = now;
		g_local_ref = 0;
	}
	++g_local_ref;
	g_ref_mutex.unlock();
	if (prefix)
	{
		int nlen = (int)strlen(prefix);
		switch (nlen)
		{
		case 0:
			sprintf_s(order_ref, _char_size, "%02d%02d%02d%04d00", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, g_local_ref & 0x1FFF); //ÿ��8191�ʵ���
			break;
		case 1:
			sprintf_s(order_ref, _char_size, "%02d%02d%02d%04d0%s", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, g_local_ref & 0x1FFF, prefix); //ÿ��8191�ʵ���
			break;
		case 2:
			sprintf_s(order_ref, _char_size, "%02d%02d%02d%04d%s", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, g_local_ref & 0x1FFF, prefix); //ÿ��8191�ʵ���
			break;
		default:
			sprintf_s(order_ref, _char_size, "%02d%02d%02d%04d%s", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, g_local_ref & 0x1FFF, prefix + (nlen -2)); //ÿ��8191�ʵ���
			break;
		}		
	}
	else {
		sprintf_s(order_ref, _char_size, "%02d%02d%02d%06d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, g_local_ref & 0x7FFFF);
	}
	return order_ref; //����Ҫ�ͷ��ڴ�
}

//=============================================================

int get_exid_from_index_mul(const char * strex)
{
	if (strcmp(strex,"SZ") == 0)
	{
		return (int)EnumExchangeIDType::SZSE;
	}
	else if (strcmp(strex, "SH") == 0)
	{
		return (int)EnumExchangeIDType::SSE;
	}
	else {
		return (int)EnumExchangeIDType::SSE;
	}
}

int get_exid_from_ctp(const char * strex) 
{
	return (int)EnumExchangeIDType::CFFEX;
}

const char * get_exid_to_ctp(EnumExchangeIDType enumex)
{
	return "CFFEX";
}

int get_direction_from_ctp(char direct)
{
	if (direct == '0')
	{
		return (int)Buy;
	}
	else {
		return (int)Sell;
	}
}
/*
///����
#define THOST_FTDC_OF_Open '0'
///ƽ��
#define THOST_FTDC_OF_Close '1'
///ǿƽ
#define THOST_FTDC_OF_ForceClose '2'
///ƽ��
#define THOST_FTDC_OF_CloseToday '3'
///ƽ��
#define THOST_FTDC_OF_CloseYesterday '4'
///ǿ��
#define THOST_FTDC_OF_ForceOff '5'
///����ǿƽ
#define THOST_FTDC_OF_LocalForceClose '6'
*/
int get_offset_from_ctp(char offset)
{
	switch (offset)
	{
	case '0':
		return (int)EnumOffsetFlagType::Open;
	case '1':
		return (int)EnumOffsetFlagType::Close;
	case '3':
		return (int)EnumOffsetFlagType::CloseToday;
	case '4':
		return (int)EnumOffsetFlagType::CloseYesterday;
	default:
		return (int)EnumOffsetFlagType::Close;
	}
}
int get_hedgeflag_from_ctp(char hedge)
{

}
/*
///ȫ���ɽ�
#define THOST_FTDC_OST_AllTraded '0'
///���ֳɽ����ڶ�����
#define THOST_FTDC_OST_PartTradedQueueing '1'
///���ֳɽ����ڶ�����
#define THOST_FTDC_OST_PartTradedNotQueueing '2'
///δ�ɽ����ڶ�����
#define THOST_FTDC_OST_NoTradeQueueing '3'
///δ�ɽ����ڶ�����
#define THOST_FTDC_OST_NoTradeNotQueueing '4'
///����
#define THOST_FTDC_OST_Canceled '5'
///δ֪
#define THOST_FTDC_OST_Unknown 'a'
///��δ����
#define THOST_FTDC_OST_NotTouched 'b'
///�Ѵ���
#define THOST_FTDC_OST_Touched 'c'
*/
int get_orderstatus_from_ctp(char status)
{
	switch (status)
	{
	case '0':
		return (int)EnumOrderStatusType::AllTraded;
	case '1':
	case '3':
		return (int)EnumOrderStatusType::NoTradeQueueing;
	case '5':
		return (int)EnumOrderStatusType::Canceled;
	case 'a':
		return (int)EnumOrderStatusType::StatusUnknown;
	default:
		return (int)EnumOrderStatusType::StatusUnknown;
	}
}
/*
///��
#define THOST_FTDC_PD_Net '1'
///��ͷ
#define THOST_FTDC_PD_Long '2'
///��ͷ
#define THOST_FTDC_PD_Short '3'
*/
int get_position_direct_from_ctp(char direct)
{
	switch (direct)
	{
	case '1':
		return (int)EnumPosiDirectionType::Net;
	case '2':
		return (int)EnumPosiDirectionType::Long;
	case '3':
		return (int)EnumPosiDirectionType::Short;
	default:
		return -1;
	}
}

/*
typedef unsigned char EES_ExchangeID;					///< ������ID
#define EES_ExchangeID_sh_cs                    100		///< =�Ͻ���
#define EES_ExchangeID_sz_cs                    101		///< =���
#define EES_ExchangeID_cffex                    102		///< =�н���
#define EES_ExchangeID_shfe                     103		///< =������
#define EES_ExchangeID_dce                      104		///< =������
#define EES_ExchangeID_zcze                     105		///< =֣����
#define EES_ExchangeID_ine						106		///< =��Դ����
#define EES_ExchangeID_sge						107		///< =�Ϻ�����
#define EES_ExchangeID_done_away                255		///< =Done-away
*/
int get_exid_from_ees(unsigned char strex)
{
	switch (strex)
	{
	case 100:
		return (int)EnumExchangeIDType::SSE;
	case 101:
		return (int)EnumExchangeIDType::SZSE;
	case 102:
		return (int)EnumExchangeIDType::CFFEX;
	case 103:
		return (int)EnumExchangeIDType::SHFE;
	case 104:
		return (int)EnumExchangeIDType::DCE;
	case 105:
		return (int)EnumExchangeIDType::CZCE;
	case 106:
		return (int)EnumExchangeIDType::INE;
	case 107:
		return (int)EnumExchangeIDType::SGE;
	default:
		return -1;
	}
}
unsigned char get_exid_to_ees(EnumExchangeIDType enumex)
{
	switch (enumex)
	{
	case INE:
		return 106;
	case SSE:
		return 100;
	case DCE:
		return 104;
	case SZSE:
		return 101;
	case SGE:
		return 107;
	case CFFEX:
		return 102;
	case SHFE:
		return 103;
	case CZCE:
		return 105;
	default:
		return 0;
	}
}
/*
typedef unsigned char EES_SideType;						///< ��������
#define EES_SideType_open_long                  1		///< =�򵥣�����
#define EES_SideType_close_today_long           2		///< =������ƽ��
#define EES_SideType_close_today_short          3		///< =�򵥣�ƽ��
#define EES_SideType_open_short                 4		///< =����������
#define EES_SideType_close_ovn_short            5		///< =�򵥣�ƽ��
#define EES_SideType_close_ovn_long             6		///< =������ƽ��
#define EES_SideType_opt_exec					11		///< =��Ȩ��Ȩ
#define EES_SideType_close_short				21		///< =�򵥣�ƽ�֣�
#define EES_SideType_close_long					22		///< =������ƽ�֣�
*/
int get_direction_from_ees(unsigned char side)
{
	switch (side)
	{
	case 1:
	case 3:
	case 5:
	case 21:
		return (int)EnumDirectionType::Buy;
	case 2:
	case 4:
	case 6:
	case 22:
		return (int)EnumDirectionType::Sell;
	default:
		return (int)EnumDirectionType::Buy;
	}
}
int get_offset_from_ees(unsigned char side)
{
	switch (side)
	{
	case 1:
	case 4:
		return (int)EnumOffsetFlagType::Open;
	case 2:
	case 3:
		return (int)EnumOffsetFlagType::CloseToday;
	case 5:
	case 6:
		return (int)EnumOffsetFlagType::CloseYesterday;
	default:
		return (int)EnumOffsetFlagType::Close;
	}
}
int get_hedgeflag_from_ees(char hedge)
{

}
/*
typedef unsigned char EES_OrderStatus;					///< ���ն��������Ŷ������״̬
#define EES_OrderStatus_shengli_accept			0x80	///< bit7=1��EESϵͳ�ѽ���
#define EES_OrderStatus_mkt_accept				0x40	///< bit6=1���г��ѽ��ܻ����ֹ���Ԥ����
#define EES_OrderStatus_executed				0x20	///< bit5=1���ѳɽ��򲿷ֳɽ�
#define EES_OrderStatus_cancelled				0x10 	///< bit4=1���ѳ���, �����ǲ��ֳɽ�����
#define EES_OrderStatus_cxl_requested			0x08	///< bit3=1�������ͻ���������
#define EES_OrderStatus_reserved1				0x04	///< bit2������, Ŀǰ����
#define EES_OrderStatus_reserved2				0x02	///< bit1������, Ŀǰ����
#define EES_OrderStatus_closed					0x01	///< bit0=1���ѹر�, (�ܾ�/ȫ���ɽ�/�ѳ���)

*/
int get_orderstatus_from_ees(unsigned char status)
{
	if ((status & 0x10) && (status & 0x01))
	{
		return (int)EnumOrderStatusType::Canceled;
	}
	else if ((status & 0x20) && (status & 0x01))
	{
		return (int)EnumOrderStatusType::AllTraded;
	}
	else if (!(status & 0x01) && (status & 0x40))
	{
		return (int)EnumOrderStatusType::NoTradeQueueing;
	}
	else
	{
		return (int)EnumOrderStatusType::StatusUnknown;
	}
}
/*
typedef int     EES_PosiDirection;						///< ��շ��� 1����ͷ 5����ͷ
#define EES_PosiDirection_long					1		///< =��ͷ
#define EES_PosiDirection_short					5		///< =��ͷ
*/
int get_position_direct_from_ees(int direct)
{
	switch (direct)
	{
	case 1:
		return (int)EnumPosiDirectionType::Long;
	case 5:
		return (int)EnumPosiDirectionType::Short;
	default:
		return -1;
	}
}


