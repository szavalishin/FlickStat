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
	typedef struct{
		uint ImageNum; //image number to be downloaded
		ulong ImageGlobalNum; //image real number
		uchar ThreadNum; //current thread number
		ThreadInfo* t; //owner thread info
	} DownloadThreadInfo;

	//adds image to cache
	bool AddCache(ThreadInfo* Thread, Image* Img);

	//main thread function (pass ThreadInfo* as args)
	uint _stdcall ThreadFunc(void* args); 

	//download threads function (pass DownloadThreadInfo* as args)
	uint _stdcall DownloadThreadFunc(void* args);

	//downloads image
	string DownloadImage(DownloadThreadInfo* dt, ImgMeta* Img);

	//gets image uri from its meta
	string GetURI(ImgMeta* Img, ImgSize Size);

	//writes data to file
	static size_t WriteFileCallback(void *contents, size_t size, size_t nmemb, void *userp);

	//writes data to string
	static size_t WriteStringCallback(void *contents, size_t size, size_t nmemb, void *userp);

	//gets photo favorites count
	uint GetPhotoFavorites(DownloadThreadInfo* dt, ImgMeta* Img);

	ThreadInfo* Init(string Key, string Secret, uchar ThreadCount){
		//Creating structure
		ThreadInfo* t = new(ThreadInfo);

		if(!t){
			PrintDebug("Error: Can't create new web thread structure (not enough memory?)\n");
			return 0;
		}

		t->MaxDownloadThreads = (ThreadCount > 0)?ThreadCount:DEFAULT_DOWNLOAD_THREADS_COUNT;

		//saving API key and secret
		t->APIKey = Key;
		t->APISecret = Secret;

		//Creating cache
		t->CacheData.First = 0;
		t->CacheData.Last = 0;
		t->CacheData.MaxSize = DEFAULT_CACHE_SIZE;
		t->CacheData.Size = 0;

		//creating events
		t->CacheAvailable = CreateMutex(0, false, 0); //cache is available now
		t->ImageAvailable = CreateEvent(0, true, false, 0); //image is not available now
		t->Running = CreateEvent(0, true, false, 0); //thread is created in paused state
		t->DownloadThreadAvailable = 
			CreateSemaphore(0, t->MaxDownloadThreads, t->MaxDownloadThreads, 0); //all download threads are available
		t->Terminate = false;

		//creating flickcurls
		flickcurl_init();
		t->Flickcurl = (flickcurl**)malloc(t->MaxDownloadThreads*sizeof(flickcurl*));
		
		if(t->Flickcurl)
			for(uchar i = 0; i <t->MaxDownloadThreads; i++){
				t->Flickcurl[i] = flickcurl_new();

				if(t->Flickcurl[i]){
					flickcurl_set_api_key(t->Flickcurl[i], Key.c_str());
					flickcurl_set_shared_secret(t->Flickcurl[i], Secret.c_str());
				}
			}

		//creating search params with default values
		flickcurl_search_params_init(&t->SearchParams);
		t->SearchParams.user_id = 0;
		t->SearchParams.media = "photos";
		t->SearchParams.privacy_filter = "1";
		t->SearchParams.content_type = 1;

		t->WantImage = false;
		t->WantMeta = false;
		t->WantEXIF = false;
		t->WantFavorites = false;
		t->WantSize = isMedium;

		//setting up download params
		t->WorkDir = "";
		t->CurImage = 0;
		t->TotalImages = 0;
		t->ConnectionTimeout = DEFAULT_WAIT_IMAGE_TIME;
		t->DownloadTimeout = DEFAULT_DOWNLOAD_TIMEOUT;
		t->FileBufferSize = DEFAULT_FILE_BUFFER_SIZE;

		//creating list params
		flickcurl_photos_list_params_init(&t->ListParams);
		t->ListParams.per_page = (DEFAULT_LIST_PAGE_SIZE >= t->MaxDownloadThreads)?
								 DEFAULT_LIST_PAGE_SIZE:t->MaxDownloadThreads;
		t->ListParams.page = 1;
		t->ResultList = 0;

		//creating curls
		t->Curl = (CURL**)malloc(t->MaxDownloadThreads*sizeof(CURL*));
		
		if(t->Curl)
			for(uchar i = 0; i < t->MaxDownloadThreads; i++)
				t->Curl[i] = curl_easy_init();
		
		//initializing donwload thread handles
		t->hDownloadThreads = (HANDLE*)malloc(t->MaxDownloadThreads*sizeof(HANDLE));
		//if(t->hDownloadThreads)
		//	for(uchar i = 0; i < t->MaxDownloadThreads; i++)
		//		t->hDownloadThreads[i];

		//initializing download thread flags
		t->DownloadThreads = (bool*)malloc(t->MaxDownloadThreads*sizeof(char));
		if(t->DownloadThreads)
			for(uchar i = 0; i < t->MaxDownloadThreads; i++)
				t->DownloadThreads[i] = true; //all threads are available

		//initializing thread
		t->Thread = _beginthreadex(0, 0, ThreadFunc, (void*)t, 0, 0);

		//checking if everything is ok
		if(t->CacheAvailable && t->Running && t->Thread && t->Flickcurl && t->Curl
		   && t->ImageAvailable && t->DownloadThreads){
			PrintDebug("Web thread created\n");
			return t;
		}else{
			//killing everything
			PrintDebug("Error: Can't create web thread\n");
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
		
			//killing flickcurls
			if(Thread->Flickcurl){
				for(uchar i = 0; i < Thread->MaxDownloadThreads; i++)
					if(Thread->Flickcurl[i])
						flickcurl_free(Thread->Flickcurl[i]);
				
				free(Thread->Flickcurl);
			}
	
			//killing results list
			if(Thread->ResultList)
				flickcurl_free_photos_list(Thread->ResultList);
			
			//killing mutex
			if(Thread->CacheAvailable)
				CloseHandle(Thread->CacheAvailable);
		
			//killing curls
			if(Thread->Curl){
				for(uchar i = 0; i < Thread->MaxDownloadThreads; i++)
					if(Thread->Curl[i])
						curl_easy_cleanup(Thread->Curl[i]);

				free(Thread->Curl);
			}
		
			//killing events
			if(Thread->Running)
				CloseHandle(Thread->Running);
	
			if(Thread->ImageAvailable)
				CloseHandle(Thread->ImageAvailable);

			//killing semaphore
			if(Thread->DownloadThreadAvailable)
				CloseHandle(Thread->DownloadThreadAvailable);

			//killing donwload thread handles
			if(Thread->hDownloadThreads)
				free(Thread->hDownloadThreads);

			//killing donwload thread flags
			if(Thread->DownloadThreads)
				free(Thread->DownloadThreads);
		};

		PrintDebug("Web thread killed\n");
		return true;
	};

	bool ClearCache(ThreadInfo* Thread){
		if(!Thread){
			PrintDebug("Error: Can't clear web cache - wrong web thread handle\n");
			return false;
		}

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

		PrintDebug("Web cache cleared\n");

		return true;
	};

	Image* GetCache(ThreadInfo* Thread){
		if(!Thread){
			PrintDebug("Error: Can't get web cache - wrong web thread handle\n");
			return 0;
		}

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
		if(Thread->CacheData.Size < Thread->CacheData.MaxSize)
			SetEvent(Thread->Running);

		//getting image
		Image* img = 0;
		if(citem)
			img = citem->img;

		//killing item
		delete(citem);

		PrintDebugD("Web cache returned image id%s\n", img->Meta ? img->Meta->id : "(no meta)");

		//result
		return img;
	}

	bool AddCache(ThreadInfo* Thread, Image* Img){
		if(!(Thread && Img)){	
			PrintDebug("Error: Can't add image to web cache - wrong web thread or image handle\n");
			return false;
		}

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
		if(Thread->CacheData.Size >= Thread->CacheData.MaxSize){
			PrintDebug("Warning: Web cache is full - pausing web thread\n");
			ResetEvent(Thread->Running); //pausing thread
		}

		PrintDebugD("Image id%s added to web cache\n", Img->Meta ? Img->Meta->id : "(no meta)");

		//releasing cache
		ReleaseMutex(Thread->CacheAvailable);

		//notifying that there is an image in cache
		SetEvent(Thread->ImageAvailable);

		return true;
	};

	void ResolveDeadlocks(ThreadInfo* t){
		//if any download thead is not available but it's handle is
		//zero => it's an error and we need to resolve it
		for(uint i = 0; i < t->MaxDownloadThreads; i++)
			if(!t->DownloadThreads[i] && !t->hDownloadThreads[i]){
				PrintDebugD("Warning: deadlock in thread %u resolved\n", i);
				t->DownloadThreads[i] = true;
				ReleaseSemaphore(t->DownloadThreadAvailable, 1, 0);
			}
	};

	void ThreadFunc_StartDonwloadThread(uint &CurImage, ThreadInfo* t){
		PrintDebug("Starting new donwload thread...\n");

		//waiting for download thread
		while(WaitForSingleObject(t->DownloadThreadAvailable, 10000) == WAIT_TIMEOUT){
			PrintDebug("Warning: No download threads available - checking for possible deadlocks\n");
			ResolveDeadlocks(t);
		}

		//creating new download thread strucure
		DownloadThreadInfo* dt = new(DownloadThreadInfo);
		dt->ImageNum = CurImage;
		dt->ImageGlobalNum = t->CurImage;
		dt->t = t;
					
		PrintDebug("Looking for available download threads...\n");

		//looking for available thread
		uchar AvailableThread = 0;
		while(!t->DownloadThreads[AvailableThread])
			if(AvailableThread < t->MaxDownloadThreads)
				AvailableThread++;
			else
				AvailableThread = 0;

		dt->ThreadNum = AvailableThread;

		PrintDebugD("Download thread %u started by web thread\n", AvailableThread);

		//starting download thread
		t->DownloadThreads[AvailableThread] = false; //thread is unavailable now
		t->hDownloadThreads[AvailableThread] =  (HANDLE)_beginthreadex(0, 0, DownloadThreadFunc, (void*)dt, 0, 0);
	};

	void ThreadFunc_Download(uint &CurImage, ThreadInfo* t){
		//checking if thread is paused
		WaitForSingleObject(t->Running, INFINITE);

		//checking if thread is not terminated
		if(!t->Terminate){
			//checking if we need list and downloading it
			if(!t->ResultList){
				PrintDebug("No result list. Downloading...\n");

				t->ResultList = flickcurl_photos_search_params(t->Flickcurl[0], 
					&t->SearchParams, &t->ListParams);

				if(t->ResultList){
					t->TotalImages = t->ResultList->total_count;
					PrintDebugD("Result list with %d images downloaded\n", t->ResultList->total_count);
				}
				
				CurImage = 0;
				t->ListParams.page++; //updating page
			}

			//checking if anything were found
			if(t->ResultList && t->ResultList->photos_count > 0){
				t->CurImage++;

				//downloading new image...
				ThreadFunc_StartDonwloadThread(CurImage, t);

				//updating image number
				if(CurImage < t->ResultList->photos_count - 1)
					CurImage++;
				else{ //if we need new result list
					PrintDebug("New result list needed. Killing the old one...\n");

					//waiting for all threads to finish
					WaitForMultipleObjects(t->MaxDownloadThreads, t->hDownloadThreads, true, INFINITE);

					//killing previous list
					CurImage = 0;
					flickcurl_free_photos_list(t->ResultList);
					t->ResultList = 0;
				}
			}else
				if(!t->Terminate){
					PrintDebug("Warning: Can't get new result list - pausing web thread\n");
					ResetEvent(t->Running); //pause thread if nothing were found
				}
		}
	};

	uint _stdcall ThreadFunc(void* args){
		PrintDebug("Web thread started\n");

		//checking if we have right args
		if(args){
			//getting thread info
			ThreadInfo* t = (ThreadInfo*)args;

			uint CurImage = 0;
			t->CurImage = 0;

			//donwloading images
			do{
				ThreadFunc_Download(CurImage, t);
			}while(!t->Terminate);

			PrintDebug("Web thread is waiting for download threads to finish...\n");

			//waiting for download threads to stop
			WaitForMultipleObjects(t->MaxDownloadThreads, t->hDownloadThreads, true, INFINITE);

			PrintDebug("All download threads finished\n");
		}

		PrintDebug("Web thread terminated\n");

		_endthreadex(0);
		return 0;
	};

	static size_t WriteFileCallback(void *contents, size_t size, size_t nmemb, void *userp){
		return fwrite(contents, size, nmemb, (FILE*)userp);
	};

	static size_t WriteStringCallback(void *contents, size_t size, size_t nmemb, void *userp){
		size_t realsize = size * nmemb;
		string* str = (string*)userp;

		(*str) = (*str) + (char*)contents;
 
		return realsize;
	};

	string GetFormatExtension(DownloadThreadInfo* dt, ImgMeta* Img){
		if(dt->t->WantSize == isOriginal && Img->fields[PHOTO_FIELD_originalformat].string)
			return Img->fields[PHOTO_FIELD_originalformat].string;
		else
			return "jpg";
	};

	bool IsNotHTML(FILE* f){
		if(!fseek(f, 0, SEEK_SET)){
			char buffer[100];
			
			if(fread(buffer, sizeof(char), 100, f))
				if(((string)buffer).find("html") != string::npos)
					return false;
		}
		return true;
	};

	string DownloadImage(DownloadThreadInfo* dt, ImgMeta* Img){
		//creating temp file
		string FileName;
		char Num[30];

		itoa(dt->ImageGlobalNum, Num, 10);
		
		if(dt->t->WorkDir != "")
			FileName = dt->t->WorkDir + "\\" + Num + "." + GetFormatExtension(dt, Img);
		else
			FileName = (string)Num +  "." + GetFormatExtension(dt, Img);

		//creating new file
		FILE* f = fopen(FileName.c_str(), "wb+");

		//setting up big file buffer
		if(f)
			setvbuf(f, 0, _IOFBF, dt->t->FileBufferSize);
		else{
			PrintDebugD2("Error: Can't donwload image %u (id%s) - no file opened\n", (uint)dt->ImageGlobalNum, Img ? Img->id : "(no meta)");
			return "";
		}
		
		//retrieving url
		string URI = GetURI(Img, dt->t->WantSize);

		//prepating to download
		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_URL, URI.c_str());
		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_WRITEFUNCTION, WriteFileCallback);
		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_WRITEDATA, f);
		
		if(dt->t->DownloadTimeout == 0)
			curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_TIMEOUT_MS, INFINITE);
		else
			curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_TIMEOUT_MS, dt->t->DownloadTimeout);

		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_CONNECTTIMEOUT_MS, dt->t->ConnectionTimeout);
		
		//downloading file
		CURLcode ErrCode = curl_easy_perform(dt->t->Curl[dt->ThreadNum]);
		if(ErrCode == 0){
			PrintDebugD2("Image %u (id%s) downloaded\n", (uint)dt->ImageGlobalNum, Img ? Img->id : "(no meta)");

			//if it's an image
			if(IsNotHTML(f)){	
				fclose(f); 
				return FileName;
			}else{ //if it's a HTML file
				fclose(f); 
				remove(FileName.c_str());
				return "";
			};
		}else{
			PrintDebugD3("Warning: Download thread %u can't donwload image id%s - pausing web thread. CURLcode = %d\n", (uint)dt->ThreadNum, Img ? Img->id : "(no meta)", ErrCode);

			//on error
			fclose(f); 
			remove(FileName.c_str());
			ResetEvent(dt->t->Running); //pausing web thread if connection timeout occured
			return "";
		};
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
			case isOriginal: return flickcurl_photo_as_source_uri(Img, 'o'); //original image
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

	uint GetPhotoFavorites(DownloadThreadInfo* dt, ImgMeta* Img){
		uint FavCount = 0;
		string Response = "";

		//retrieving url
		string URI = (string)"http://api.flickr.com/services/rest/?" +
			"method=flickr.photos.getFavorites&api_key=" + 
			dt->t->APIKey + "&photo_id=" +
			Img->id + "&page=1&per_page=1&format=rest";

		//preparing to download
		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_URL, URI.c_str());
		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_WRITEFUNCTION, WriteStringCallback);
		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_TIMEOUT_MS, dt->t->ConnectionTimeout);
		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_CONNECTTIMEOUT_MS, dt->t->ConnectionTimeout);
		curl_easy_setopt(dt->t->Curl[dt->ThreadNum], CURLOPT_WRITEDATA, (void*)&Response);

		//downloading response xml
		if(curl_easy_perform(dt->t->Curl[dt->ThreadNum]) == 0){
			//checking if we have right response
			if(Response.find("<rsp stat=\"ok\">") != string::npos){
				//looking for total=" tag
				uint first = Response.find("total=\"") + sizeof("total=\"") - 1;
				uint last = first;

				//looking for next quote sybol after total="
				while(Response[++last] != '\"');

				//getting favorites count from response
				Response = Response.substr(first, last - first);
				FavCount = atoi(Response.c_str());
			}

			return FavCount;
		}else{ //on error
			ResetEvent(dt->t->Running);
			return 0;
		};
	};

	uint _stdcall DownloadThreadFunc(void* args){
		//checking params
		if(!args){
			PrintDebug("Error: Can't start download thread - wrong args. Possible deadlock!\n");
			_endthreadex(0);
			return 0;
		}

		DownloadThreadInfo* dt = (DownloadThreadInfo*)args;

		PrintDebugD("Download thread %u started\n", dt->ThreadNum);

		//checking if we can download anything
		if(dt->t && dt->t->ResultList && dt->t->ResultList->photos && dt->ImageNum < dt->t->ResultList->per_page && !dt->t->Terminate){
			//creating new image
			Image* Img = new(Image);

			if(FlickStat::InitImage(Img)){
				//downloading photo meta if needed
				if(dt->t->WantMeta && !dt->t->Terminate)
					Img->Meta = flickcurl_photos_getInfo(dt->t->Flickcurl[dt->ThreadNum], 
								dt->t->ResultList->photos[dt->ImageNum]->id);
				
				//downloading photo favorites count if needed
				if(dt->t->WantFavorites && Img->Meta && !dt->t->Terminate){
					Img->Meta->fields[PHOTO_FIELD_favorites].integer = 
						(flickcurl_photo_field_type)GetPhotoFavorites(dt, Img->Meta);
					Img->Meta->fields[PHOTO_FIELD_favorites].type = VALUE_TYPE_INTEGER;
				}

				//downloading photo exif if needed
				if(dt->t->WantEXIF && !dt->t->Terminate)
					Img->EXIF = flickcurl_photos_getExif(dt->t->Flickcurl[dt->ThreadNum], 
								dt->t->ResultList->photos[dt->ImageNum]->id,
								dt->t->ResultList->photos[dt->ImageNum]->fields[PHOTO_FIELD_secret].string);
				
				//downloading photo itself if needed
				if(dt->t->WantImage && !dt->t->Terminate)
					if(Img->Meta)
						Img->Img = DownloadImage(dt, Img->Meta);
					else
						Img->Img = DownloadImage(dt, dt->t->ResultList->photos[dt->ImageNum]);

				//checking if we've got any data
				if(Img->EXIF || Img->Img != "" || Img->Meta && !dt->t->Terminate){
					#ifdef FLICKSTAT_DEBUG
						fprintf(stderr, 
							"Download thread %u downloaded image %u (id%s): Meta:%s, EXIF: %s, Image: %s\n",
							dt->ThreadNum,
							(uint)dt->ImageGlobalNum,
							Img->Meta ? Img->Meta->id : "(no meta)",
							Img->Meta?"yes":"no",
							Img->EXIF?"yes":"no",
							Img->Img != ""?"yes":"no");
					#endif

					AddCache(dt->t, Img); //adding to cache
				}else{
					FreeImage(Img); //or killing intance
					PrintDebugD("Warning: Download thread %u downloaded nothing\n", dt->ThreadNum);
				}
			}
		}

		PrintDebugD("Download thread %u finished\n", dt->ThreadNum);

		//thread is available now
		//dt->t->DownloadThreads[dt->ThreadNum] = 0;
		dt->t->DownloadThreads[dt->ThreadNum] = true;
		ReleaseSemaphore(dt->t->DownloadThreadAvailable, 1, 0);

		//killing params
		free(dt);

		_endthreadex(0);
		return 0;
	};
};