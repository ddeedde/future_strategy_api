#include "SpiderCommonApi.h"
#include "FutureApi.h"


SpiderApi* SpiderApi::createSpiderApi()
{
	return new SpiderCommonApi();
}

