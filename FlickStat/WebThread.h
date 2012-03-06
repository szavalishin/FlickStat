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
	#define DEFAULT_CACHE_SIZE 20 //items
	#define DEFAULT_LIST_PAGE_SIZE 100 //items
	#define DEFAULT_FILE_BUFFER_SIZE 512000 //bytes
	#define DEFAULT_DOWNLOAD_THREADS_COUNT 10 
	#define DEFAULT_DOWNLOAD_TIMEOUT 600000 //msecs

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
		string APIKey;
		string APISecret;

		uintptr_t Thread; //thread pointer
		HANDLE* hDownloadThreads; //download threads array
		bool* DownloadThreads; //available thread flags (true if available)

		flickcurl** Flickcurl; //flickcurl pointers array
		CURL** Curl; //curl pointers array

		flickcurl_search_params SearchParams; //search params
		flickcurl_photos_list* ResultList; //result list pointer
		flickcurl_photos_list_params ListParams; //list params

		bool WantImage; //set to true if image is needed
		bool WantMeta; //set to true if meta is needed
		bool WantEXIF; //set to true if EXIF is needed
		bool WantFavorites; //set to true if favorites count is wanted
		ImgSize WantSize; //image size

		string WorkDir; //temp dir (for cache)
		ulong CurImage; //current image number
		ulong TotalImages; //total images count
		uchar MaxDownloadThreads; //max download threads count
		long DownloadTimeout; //max time for image downloading (when connected)
		long ConnectionTimeout; //connection waiting timeout
		uint FileBufferSize; //file buffer size

		HANDLE CacheAvailable; //cache access mutex (set mutex when accessing cache)
		HANDLE ImageAvailable; //image available event (wait to this event when getting image)
		HANDLE Running; //running event (reset event to pause thread)
		HANDLE DownloadThreadAvailable; //download thread available semaphore (wait to this semaphore when when staring new thread)
		bool Terminate; //signals thread to terminate

		Cache CacheData; //cache
	} ThreadInfo;

	ThreadInfo* Init(string Key, string Secret, uchar ThreadCount); //creates ThreadInfo with current API key, secret and download threads
	bool Free(ThreadInfo* Thread); //frees ThreadInfo

	Image* GetCache(ThreadInfo* Thread); //gets image from cache
	bool ClearCache(ThreadInfo* Thread); //clears cache
};