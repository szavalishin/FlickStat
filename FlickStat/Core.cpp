//FlickStat library
//Copyright (c) by Sergey Zavalishin 2011

//Core.cpp
//Core - main file

#include "stdafx.h"

#include "Core.h"
#include <io.h>

namespace FlickStat{
	fsSession Init(string Key, string Secret){
		return FlickStat::Init(Key, Secret, DEFAULT_DOWNLOAD_THREADS_COUNT);	
	};

	fsSession Init(string Key, string Secret, uchar ThreadCount){
		//creating session
		Core::Session* ses = new(Core::Session);

		//setting up default params
		if(ses){
			ses->DataThread = 0; //DataThread::Init();
			ses->WebThread = WebThread::Init(Key, Secret, ThreadCount);

			ses->LastError = "";
			ses->WaitForImageTime = DEFAULT_WAIT_IMAGE_TIME;

			//returning session handle
			if(/*ses->DataThread &&*/ ses->WebThread){
				PrintDebug("New session created\n");
				return (fsSession)ses;
			};
		};
		
		//killing everything on error
		PrintDebug("Error: Can't create new session (not enough memory?)\n");
		Free(ses);
		return 0;
	};

	bool Free(fsSession Session){
		//checking params
		if(!Session){
			PrintDebug("Error: Can't kill session - wrong session handle\n");
			return true;
		}

		Core::Session* ses = (Core::Session*)Session;

		//killing web thread
		WebThread::Free(ses->WebThread);

		//killing data thread
		//DataThread::Free(ses->DataThread);

		PrintDebug("Session killed\n");

		return true;
	};

	bool Start(fsSession Session, SearchParams* Params){	
		//checking if we can run
		if(!Session){
			PrintDebug("Error: Can't start session - wrong session handle\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		FlickStat::Stop(Session);
		
		//killing cache
		WebThread::ClearCache(ses->WebThread);

		//updating search params
		ses->WebThread->SearchParams.user_id = (char*)Params->UserID.c_str();
		ses->WebThread->SearchParams.tags = (char*)Params->Tags.c_str();
		ses->WebThread->SearchParams.machine_tags = (char*)Params->MachineTags.c_str();
		ses->WebThread->SearchParams.min_upload_date = Params->UploadDate.Min;
		ses->WebThread->SearchParams.max_upload_date = Params->UploadDate.Max;
		
		switch(Params->Sort){
			case stDatePostedDesc: ses->WebThread->SearchParams.sort = "date-posted-desc"; break;
			case stDatePostedAsc: ses->WebThread->SearchParams.sort = "date-posted-asc"; break;
			case stDateTakenDesc: ses->WebThread->SearchParams.sort = "date-taken-desc"; break;
			case stDateTakenAsc: ses->WebThread->SearchParams.sort = "date-taken-asc"; break;
			case stInterestDesc: ses->WebThread->SearchParams.sort = "interestingness-desc"; break;
			case stInterestAsc: ses->WebThread->SearchParams.sort = "interestingness-asc"; break;
			case stRelevance: ses->WebThread->SearchParams.sort = "relevance"; break;
			default: ses->WebThread->SearchParams.sort = 0;
		};

		ses->WebThread->WantSize = Params->Size;
		if(Params->Size == isOriginal) //we need meta do get original image
			Params->WantMeta = true;

		ses->WebThread->WantFavorites = Params->WantFavorites;
		if(Params->WantFavorites) //we need meta to get favorites
			Params->WantMeta = true;

		ses->WebThread->WantImage = Params->WantImage;
		ses->WebThread->WantMeta = Params->WantMeta;
		ses->WebThread->WantEXIF = Params->WantEXIF;
		ses->WebThread->CurImage = 0;

		//killing old result list
		//flickcurl_free_photos_list(ses->WebThread->ResultList);
		//ses->WebThread->ResultList = 0;

		//starting threads
		SetEvent(ses->WebThread->Running);
		//DataThread.start...

		PrintDebug("Session started\n");

		return true;
	};

	bool Stop(fsSession Session){
		//checking if we can stop
		if(!Session){
			PrintDebug("Error: Can't stop session - wrong session handle\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		//if threads are running, let's pause them
		ResetEvent(ses->WebThread->Running);

		//waiting for donwload threads to finish
		//WaitForMultipleObjects(ses->WebThread->MaxDownloadThreads, 
			//ses->WebThread->hDownloadThreads, true, INFINITE);

		PrintDebug("Session stopped\n");

		return true;
	};

	bool Resume(fsSession Session){
		//checking if we can stop
		if(!Session){
			PrintDebug("Error: Can't resume session - wrong session handle\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		//if threads are running, let's pause them
		SetEvent(ses->WebThread->Running);

		PrintDebug("Session resumed\n");

		return true;
	};

	bool SetWebCacheDir(fsSession Session, string Dir){
		//checking if we can stop
		if(!Session)
			return false;

		Core::Session* ses = (Core::Session*)Session;

		ses->WebThread->WorkDir = Dir;

		PrintDebugD("Web cache dir changed to \"%s\"\n", Dir.c_str());

		return true;
	};

	bool SetWebCacheSize(fsSession Session, uchar CacheSize){
		//checking data
		if(!Session){
			PrintDebug("Error: Can't change web cache size (wrong session handle)\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		//changing web cache size
		if(CacheSize > 0){
			ses->WebThread->CacheData.MaxSize = CacheSize;
			PrintDebugD("Web cache size changed to %u\n", ses->WebThread->CacheData.MaxSize);
			return true;
		}

		PrintDebugD("Error: Can't change web cache size to %u\n", CacheSize);
		return false;
	};

	bool SetResultPageSize(fsSession Session, uint PageSize){
		//checking data
		if(!Session){
			PrintDebug("Error: Can't set result page size - wrong session handle\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		//checking data
		if(PageSize > 0 && PageSize <= 500){
			//page size MUST be greater than download threads count
			if(PageSize >= ses->WebThread->MaxDownloadThreads)
				ses->WebThread->ListParams.per_page = PageSize;
			else
				ses->WebThread->ListParams.per_page = 
					ses->WebThread->MaxDownloadThreads;

			PrintDebugD("Result page size changed to %d\n", ses->WebThread->ListParams.per_page);

			return true;
		}

		PrintDebugD("Error: Can't change result page size to %u\n", PageSize);

		return false;
	};

	bool SetConnectionTimeout(fsSession Session, uint Msecs){
		//checking data
		if(!Session){
			PrintDebug("Error: Can't set connetion timeout - wrong session handle\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		if(Msecs > 0){
			ses->WaitForImageTime = Msecs;
			ses->WebThread->ConnectionTimeout = Msecs;
			PrintDebugD("Connection timeout changed to %u\n", Msecs);
		}else{
			PrintDebugD("Warning: Can't set connetion timeout to %u (use value greater than 0)\n", Msecs);
			return false;
		}

		return true;
	};

	bool SetDownloadTimeout(fsSession Session, uint Msecs){
		//checking data
		if(!Session){
			PrintDebug("Error: Can't set download timeout - wrong session handle\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		if(Msecs != 0)
			ses->WebThread->DownloadTimeout = Msecs;
		else
			ses->WebThread->DownloadTimeout = INFINITE;

		PrintDebugD("Download timeout changed to %u\n", Msecs);

		return true;
	};

	uint GetImagesCount(fsSession Session){
		if(Session)
			return ((Core::Session*)Session)->WebThread->TotalImages;
		
		PrintDebug("Error: Can't get images count - wrong session handle\n");

		return 0;
	};

	string GetLastError(fsSession Session){
		if(Session)
			return ((Core::Session*)Session)->LastError;
		
		return "Can't get last error - wrong session handle\n";
	};

	bool NoMoreImagesLeft(fsSession Session){
		//checking data
		if(!Session){
			PrintDebug("Error: Can't check if no more images left - wrong session handle\n");
			return true;
		}

		Core::Session* ses = (Core::Session*)Session;

		if(ses->WebThread->CurImage >= ses->WebThread->TotalImages)
			return true;

		return false;
	};

	bool IsRunning(fsSession Session){
		//checking data
		if(!Session){
			PrintDebug("Error: Can't check if session is running - wrong session handle\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		if(WaitForSingleObject(ses->WebThread->Running, 1) == WAIT_TIMEOUT)
			return false;

		return true;
	};

	Image* GetImage(fsSession Session){
		//checking...
		if(!Session){
			PrintDebug("Error: Can't get new image - wrong session handle\n");
			return 0;
		}

		Core::Session* ses = (Core::Session*)Session;

		//waiting for image
		DWORD WaitRes = WaitForSingleObject(ses->WebThread->ImageAvailable, ses->WaitForImageTime);

		//checking if any error occured...
		if(WaitRes == WAIT_TIMEOUT){
			//checking if web thread is running
			if(ses->WebThread->Terminate || !ses->WebThread->Thread){
				Core::AddError(ses, "Can't get an image - download thread is disabled (internal error)");
				PrintDebugD("Error: User app %s\n", ses->LastError.c_str());
			}else
				if(IsRunning(ses)){
					if(NoMoreImagesLeft(ses))
						Core::AddError(ses, "Can't get an image - no more images left to download");
					else
						Core::AddError(ses, "Can't get an image - session is paused");

					PrintDebugD("Error: User app %s\n", ses->LastError.c_str());
				}else{
					Core::AddError(ses, "Can't get an image - connection timeout");
					PrintDebugD("Error: User app %s\n", ses->LastError.c_str());
				}

			return 0;
		};

		//getting an image
		return WebThread::GetCache(ses->WebThread);
	};

	Image* GetImage(string ID, bool WantImage, bool WantMeta, bool WantEXIF){
		return 0;
	};

	Image* GetImageAsync(fsSession Session){
		//checking data
		if(!Session){
			PrintDebug("Error: Can't get new image in async mode - wrong session handle\n");
			return false;
		}

		Core::Session* ses = (Core::Session*)Session;

		PrintDebug("User app got new image in async mode\n");

		return WebThread::GetCache(ses->WebThread);
	};
	
	bool SetNotification(fsSession Session, NotificationType Type, NotificationFunc){
		return false;
	};
	
	//dummies
	bool OpenDB(fsSession Session, string FileName){return false;}; //opens DB with named FileName
	bool CloseDB(fsSession Session){return false;}; //closes DB assosiated with current session
	bool ClearDB(fsSession Session){return false;}; //clears DB assosiated with current session
	bool DeleteDB(string FileName){return false;}; //deletes DB named FileName
	uint GetDBSize(fsSession Session){return false;}; //returns current DB entries count
	DBEntry* GetDBEntry(fsSession Session, uint ID){return false;}; //returns DB entry
	bool UpdateDBEntry(fsSession Session, uint ID, DBEntry* NewEntry){return false;}; //updates DB entry with current data
	bool DeleteDBEntry(fsSession Session, uint ID){return false;}; //deletes DB entry
	bool AddDBEntry(fsSession Session, DBEntry* NewEntry){return false;}; //adds new DB entry

	bool InitSearchParams(SearchParams* Params){
		//creating params if needed
		if(!Params){
			Params = new(SearchParams);

			if(!Params)
				return false;
		}

		Params->Sort = FlickStat::SortType::stDatePostedDesc;
		Params->Size = isLarge;
		Params->Tags = "";
		Params->MachineTags = "";
		Params->UploadDate.Max = 0;
		Params->UploadDate.Min = 0;
		Params->UserID = "";
		Params->WantEXIF = false;
		Params->WantImage = false;
		Params->WantFavorites = false;
		Params->WantMeta = true;
		
		return true;
	};

	bool FreeSearchParams(SearchParams* Params){
		if(!Params)
			return false;

		free(Params);

		return true;
	};
	
	FLICKSTAT_API bool InitImage(Image* Img){
		if(!Img){
			Img = new(Image);

			if(!Img)
				return false;
		}

		Img->EXIF = 0;
		Img->Img = "";
		Img->Meta = 0;

		return true;
	};

	FLICKSTAT_API bool FreeImage(Image* Img){
		if(!Img)
			return false;

		if(Img->EXIF)
			flickcurl_free_exifs(Img->EXIF);

		if(Img->Img != ""){
			remove(Img->Img.c_str());
			Img->Img = "";
		}

		if(Img->Meta)
			flickcurl_free_photo(Img->Meta);

		delete(Img);

		return true;
	};
	
	bool InitDBEntry(DBEntry* Entry, size_t Size){return false;};
	bool FreeDBEntry(DBEntry* Entry){return false;};
	bool InitMetrics(DBMetrics* Metrics){return false;};
	bool FreeMetrics(DBMetrics* Metrics){return false;};

	bool Core::AddError(Session* ses, string Context){
		if(ses){
			ses->LastError = Context;
		}else
			return false;

		return true;
	};

	string GetTagValue(FlickStat::Image* Img, string TagName){
		if(!Img)
			return 0;

		if(!Img->EXIF)
			return 0;

		for(uint i = 0; Img->EXIF[i] != 0; i++)
			if(TagName == Img->EXIF[i]->label)
				return Img->EXIF[i]->raw; 

		return "";
	};
};