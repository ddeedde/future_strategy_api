
#include "LogWrapper.h"
	
LoggerId g_main_log;

void InitGlobalLog()
{
	ILog4zManager::GetInstance()->Config("./config/log4z.cfg");
	g_main_log=ILog4zManager::GetInstance()->FindLogger("Main");

	ILog4zManager::GetInstance()->Start();
}


