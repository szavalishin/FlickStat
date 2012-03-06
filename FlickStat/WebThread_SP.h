//FlickStat library
//Copyright (c) by Sergey Zavalishin 2011

//WebThread.h
//Web Thread - main header file

#pragma once

#include <windows.h>
#include <process.h>
#include "curl\curl.h"

using namespace FlickStat;

namespace WebThread{
	#define DEFAULT_CACHE_SIZE 5
	#define DEFAULT_LIST_PAGE_SIZE 100
	#define FILE_BUFFER_SIZE 512 //bytes

	//cache item
	typedef struct CacheItem{
		Image* img; //current item
		WebThread::CacheItem* next; //next item
	};
	
	//cache
	typedef struct{
		CacheItem* First; //first item
		CacheItem* Last; //last item

		uint Size; //current cache size
		uint MaxSize; //max cache size
	} Cache;

	//web thread structure
	typedef struct{
		uintptr_t Thread; //thread pointer
		flickcurl* Flickcurl; //flickcurl pointer
		CURL* Curl; //curl pointer
		flickcurl_search_params SearchParams; //search params
		flickcurl_photos_list* ResultList; //result list pointer
		flickcurl_photos_list_params ListParams; //list params

		bool WantImage; //set to true if image is needed
		bool WantMeta; //set to true if meta is needed
		bool WantEXIF; //set to true if EXIF is needed
		ImgSize WantSize; //image size
		string WorkDir; //temp dir
		ulong CurImage;
		ulong TotalImages;

		HANDLE CacheAvailable; //cache access mutex (set mutex when accessing cache)
		HANDLE ImageAvailable; //image available event (wait to this event when getting image)
		HANDLE Running; //running event (reset event to pause thread)
		bool Terminate; //signals thread to terminate

		Cache CacheData; //cache
	} ThreadInfo;

	ThreadInfo* Init(string Key, string Secret); //creates ThreadInfo with current API key and secret
	bool Free(ThreadInfo* Thread); //frees ThreadInfo

	Image* GetCache(ThreadInfo* Thread); //gets image from cache
	bool ClearCache(ThreadInfo* Thread); //clears cache
};