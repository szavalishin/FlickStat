//FlickStat library
//Copyright (c) by Sergey Zavalishin 2011

//WebThread.cpp
//Web Thread - main file

#pragma once

#include "stdafx.h"

#include "Core.h"
#include "WebThread.h"

using namespace FlickStat;

namespace WebThread{
	//adds image to cache
	bool AddCache(ThreadInfo* Thread, Image* Img);

	//main thread function (pass ThreadInfo* as args)
	void ThreadFunc(void* args); 

	//downloads image
	string DownloadImage(ThreadInfo* t, ImgMeta* Img);

	//gets image uri from its meta
	string GetURI(ImgMeta* Img, ImgSize Size);

	//writes data to file
	static size_t WriteFileCallback(void *contents, size_t size, size_t nmemb, void *userp);

	ThreadInfo* Init(string Key, string Secret){
		//Creating structure
		ThreadInfo* t = new(ThreadInfo);

		if(!t)
			return 0;

		//Creating cache
		t->CacheData.First = 0;
		t->CacheData.Last = 0;
		t->CacheData.MaxSize = DEFAULT_CACHE_SIZE;
		t->CacheData.Size = 0;

		//creating events
		t->CacheAvailable = CreateMutex(0, false, 0); //cache is available now
		t->ImageAvailable = CreateEvent(0, true, false, 0); //image is not available now
		t->Running = CreateEvent(0, true, false, 0); //thread is created in paused state
		t->Terminate = false;

		//creating flickcurl
		flickcurl_init();
		t->Flickcurl = flickcurl_new();

		if(t->Flickcurl){
			flickcurl_set_api_key(t->Flickcurl, Key.c_str());
			flickcurl_set_shared_secret(t->Flickcurl, Secret.c_str());
		}

		//creating search params with default values
		flickcurl_search_params_init(&t->SearchParams);
		t->SearchParams.user_id = 0;
		t->SearchParams.media = "photos";
		t->SearchParams.license = "1";
		t->WantImage = false;
		t->WantMeta = false;
		t->WantEXIF = false;
		t->WantSize = isMedium;
		t->WorkDir = "";
		t->CurImage = 0;
		t->TotalImages = 0;

		//creating list params
		flickcurl_photos_list_params_init(&t->ListParams);
		t->ListParams.per_page = DEFAULT_LIST_PAGE_SIZE;
		t->ListParams.page = 1;
		t->ResultList = 0;

		//creating curl
		t->Curl = curl_easy_init();
		curl_easy_setopt(t->Curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
		
		//initializing thread
		t->Thread = _beginthread(ThreadFunc, 0, (void*)t);

		//checking if everything is ok
		if(t->CacheAvailable && t->Running && t->Thread && t->Flickcurl && t->Curl
			&& t->ImageAvailable)
			return t;
		else{
			//killing everything
			Free(t);
			return 0;
		}
	};

	bool Free(ThreadInfo* Thread){
		//checking if everything is already freed
		if(Thread){
			//killing thread
			Thread->Terminate = true;
			SetEvent(Thread->Running);
			WaitForSingleObject((HANDLE)Thread->Thread, INFINITE);

			//killing cache
			ClearCache(Thread);
		
			//killing flickcurl
			if(Thread->Flickcurl){
				flickcurl_free(Thread->Flickcurl);
				//flickcurl_finish();
			}
	
			//killing results list
			if(Thread->ResultList)
				flickcurl_free_photos_list(Thread->ResultList);
			
			//killing mutex
			if(Thread->CacheAvailable)
				CloseHandle(Thread->CacheAvailable);
		
			//killing curl
			if(Thread->Curl)
				curl_easy_cleanup(Thread->Curl);
		
			//killing event
			if(Thread->Running)
				CloseHandle(Thread->Running);
	
			if(Thread->ImageAvailable)
				CloseHandle(Thread->ImageAvailable);
		};

		return true;
	};

	bool ClearCache(ThreadInfo* Thread){
		if(!Thread)
			return false;

		//notifying that image is not available
		ResetEvent(Thread->ImageAvailable);

		//waiting for cache
		WaitForSingleObject(Thread->CacheAvailable, INFINITE);

		//freeing cache
		CacheItem* Item = Thread->CacheData.First;
		Thread->CacheData.Size = 0;
		Thread->CacheData.First = 0;
		Thread->CacheData.Last = 0;
		
		//releasing cache
		ReleaseMutex(Thread->CacheAvailable);

		//restarting thread
		SetEvent(Thread->Running);

		//killing cache
		while(Item){
			FreeImage(Item->img);
			CacheItem* next = Item->next;
			delete(Item);
			Item = next;
		}

		return true;
	};

	Image* GetCache(ThreadInfo* Thread){
		if(!Thread)
			return 0;

		CacheItem* citem = 0;

		//waiting for cache
		WaitForSingleObject(Thread->CacheAvailable, INFINITE);

		//checking if cache is not empty
		if(Thread->CacheData.Size > 0){
			//getting cache
			Thread->CacheData.Size--;
			citem = Thread->CacheData.First;
			Thread->CacheData.First = citem->next;
		};

		//checking if image is available and updating notification
		if(Thread->CacheData.Size == 0)
			ResetEvent(Thread->ImageAvailable);

		//releasing cache
		ReleaseMutex(Thread->CacheAvailable);

		//restarting thread
		SetEvent(Thread->Running);

		//getting image
		Image* img = 0;
		if(citem)
			img = citem->img;

		//killing item
		delete(citem);

		//result
		return img;
	}

	bool AddCache(ThreadInfo* Thread, Image* Img){
		if(!(Thread && Img))	
			return false;

		//creating new item
		CacheItem* citem = new(CacheItem);

		if(!citem)
			return false;

		citem->img = Img;
		citem->next = 0;

		//waiting for cache
		WaitForSingleObject(Thread->CacheAvailable, INFINITE);

		//checking if cache is not full
		//if(Thread->CacheData.Size < Thread->CacheData.MaxSize){
			//adding to cache
			if(Thread->CacheData.Size == 0){
				Thread->CacheData.First = citem;
				Thread->CacheData.Last = citem;
			}else{
				Thread->CacheData.Last->next = citem;
				Thread->CacheData.Last = citem;
			}

			Thread->CacheData.Size++;
		//}

		//checking if cache is full
		if(Thread->CacheData.Size >= Thread->CacheData.MaxSize)
			ResetEvent(Thread->Running); //pausing thread

		//releasing cache
		ReleaseMutex(Thread->CacheAvailable);

		//notifying that there is an image in cache
		SetEvent(Thread->ImageAvailable);

		return true;
	};

	void ThreadFunc(void* args){
		//checking if we have right args
		if(!args)
			return;
		
		//getting thread info
		ThreadInfo* t = (ThreadInfo*)args;

		uint CurImage = 0;
		t->CurImage = 0;

		//main loop
		do{
			//checking if thread is paused
			WaitForSingleObject(t->Running, INFINITE);

			//checking if thread is not terminated
			if(!t->Terminate){
				//checking if we need list and downloading it
				if(!t->ResultList){
					t->ResultList = flickcurl_photos_search_params(t->Flickcurl, 
						&t->SearchParams, &t->ListParams);

					if(t->ResultList)
						t->TotalImages = t->ResultList->total_count;
					
					CurImage = 0;
					t->ListParams.page++; //updating page
				}

				//checking if anything were found
				if(t->ResultList){
					t->CurImage++;

					//creating new image
					Image* img = new(Image);
					InitImage(img);

					//downloading photo meta if needed
					if(t->WantMeta)
						img->Meta = flickcurl_photos_getInfo(t->Flickcurl, t->ResultList->photos[CurImage]->id);
				
					//downloading photo exif if needed
					if(t->WantEXIF)
						img->EXIF = flickcurl_photos_getExif(t->Flickcurl, t->ResultList->photos[CurImage]->id,
									t->ResultList->photos[CurImage]->fields[PHOTO_FIELD_secret].string);
				
					//downloading photo itself if needed
					if(t->WantImage)
						img->Img = DownloadImage(t, t->ResultList->photos[CurImage]);

					//checking if we've got any data
					if(img->EXIF || img->Img != "" || img->Meta)
						AddCache(t, img); //adding to cache
					else
						FreeImage(img); //or killing intance

					//updating image number
					if(CurImage < t->ResultList->photos_count - 1)
						CurImage++;
					else{
						CurImage = 0;
						flickcurl_free_photos_list(t->ResultList);
						t->ResultList = 0;
					}
				}else
					if(!t->Terminate)
						ResetEvent(t->Running); //pause thread if nothing were found
			}
		}while(!t->Terminate);
	};

	static size_t WriteFileCallback(void *contents, size_t size, size_t nmemb, void *userp){
		return fwrite(contents, size, nmemb, (FILE*)userp);
	};

	string DownloadImage(ThreadInfo* t, ImgMeta* Img){
		//creating temp file
		string FileName; 
		
		if(t->WorkDir != "")
			FileName = t->WorkDir + "\\" + Img->id + ".jpg";
		else
			FileName = (string)Img->id + ".jpg";

		FILE* f = fopen(FileName.c_str(), "wb");

		//setting up big file buffer
		if(f)
			setvbuf(f, 0, _IOFBF, FILE_BUFFER_SIZE);
		else
			return "";

		//retrieving url
		string URI = GetURI(Img, t->WantSize);

		//downloading file
		curl_easy_setopt(t->Curl, CURLOPT_URL, URI.c_str());
		curl_easy_setopt(t->Curl, CURLOPT_WRITEDATA, f);
		curl_easy_perform(t->Curl);

		fclose(f);

		return FileName;
	};

	string GetURI(ImgMeta* Img, ImgSize Size){
		if(!Img)
			return "";

		string SizeStr;

		switch(Size){
			case isSquare: SizeStr = "_s"; break; //75x75px
			case isThumbnail: SizeStr = "_t"; break; //100px on longest side
			case isSmall: SizeStr = "_m"; break; //240px on longest side
			case isMedium: SizeStr = ""; break; //500px on longest side
			case isBig: SizeStr = "_z"; break; //640px on longest side
			case isLarge: SizeStr = "_b"; break; //1024px on longest side
			default: SizeStr = "";
		};

		return "http://farm" + 
			(string)Img->fields[PHOTO_FIELD_farm].string + 
			".static.flickr.com/" +
			Img->fields[PHOTO_FIELD_server].string + 
			"/" + Img->id + "_" + 
			Img->fields[PHOTO_FIELD_secret].string +
			SizeStr + ".jpg";
	};
};