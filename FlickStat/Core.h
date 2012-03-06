//FlickStat library
//Copyright (c) by Sergey Zavalishin 2011

//Core.h
//Core - main header file

#pragma once

#include "FlickStat.h"
#include "DataThread.h"
#include "WebThread.h"

//#define FLICKSTAT_DEBUG

namespace FlickStat{
	#define DEFAULT_WAIT_IMAGE_TIME 10000

	namespace Core{

		#ifdef FLICKSTAT_DEBUG
			#define PrintDebug(Context) fprintf(stderr, Context)
			#define PrintDebugD(Context, Data) fprintf(stderr, Context, Data)
			#define PrintDebugD2(Context, Data1, Data2) fprintf(stderr, Context, Data1, Data2)
			#define PrintDebugD3(Context, Data1, Data2, Data3) fprintf(stderr, Context, Data1, Data2, Data3)
		#else
			#define PrintDebug(Context) ("")
			#define PrintDebugD(Context, Data) ("")
			#define PrintDebugD2(Context, Data1, Data2) ("")
			#define PrintDebugD3(Context, Data1, Data2, Data3) ("")
		#endif

		//FlickStat Session struct
		typedef struct{
			WebThread::ThreadInfo* WebThread;
			DataThread::ThreadInfo* DataThread;

			string LastError;
			DWORD WaitForImageTime;
		} Session;
	
		bool AddError(Session* ses, string Context); //adds error info to session
	};
};