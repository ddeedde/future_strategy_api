#pragma once

#include "SpiderApiStruct.h"


int get_seq();
const char * get_order_ref(const char * prefix = 0);

//================================

const char * get_exid_from_ctp(EnumExchangeIDType enumex);
int get_exid_from_ctp(const char * strex);
int get_direction_from_ctp(char direct);
int get_offset_from_ctp(char offset);
int get_hedgeflag_from_ctp(char hedge);
int get_orderstatus_from_ctp(char status);
int get_position_direct_from_ctp(char direct);

unsigned char get_exid_from_ees(EnumExchangeIDType enumex);
int get_exid_from_ees(unsigned char  strex);
int get_direction_from_ees(unsigned char direct);
int get_offset_from_ees(unsigned char offset);
int get_hedgeflag_from_ees(char hedge);
int get_orderstatus_from_ees(unsigned char status);
int get_position_direct_from_ees(int direct);