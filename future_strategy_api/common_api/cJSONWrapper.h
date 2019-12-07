#pragma once
#ifndef CJSON_WRAPPER_H
#define CJSON_WRAPPER_H

#include <cstdlib>
#include <string>
#include "cJSON.h"

namespace raptor 
{
	class CJsonPrinter
	{
	private: 
		cJSON *root;
		char *out;

	public:
		CJsonPrinter(bool createRootAsArray = false): out(NULL)
		{
			if ( createRootAsArray )
				root = cJSON_CreateArray();
			else
				root=cJSON_CreateObject();
		}

		~CJsonPrinter()
		{
			if (out!=NULL)
				free(out);

			cJSON_Delete(root);
		}

		cJSON *getRoot()
		{
			return root;
		}

		char * print()
		{
			if (out!=NULL)
				free(out);

			out=cJSON_PrintUnformatted(root);
			return out;
		}

		char * printFormatted()
		{
			if (out!=NULL)
				free(out);

			out=cJSON_Print(root);
			return out;
		}
	};


	class CJsonParser
	{
	private:
		cJSON *root;
	
	public:
		CJsonParser() : root(NULL)
		{
		}

		~CJsonParser()
		{
			if (root!=NULL)
				cJSON_Delete(root);
		}

		cJSON *parse(const char* text)
		{
			root = cJSON_Parse(text);
			return root;
		}
	};


} //namespace raptor
#endif

