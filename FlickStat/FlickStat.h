//FlickStat library
//Copyright (c) by Sergey Zavalishin 2011

//FlickStat.h
//User thread - main header file

#pragma once

#include <stdio.h>
#include <string>
#include "flickcurl.h"

using namespace std;

namespace FlickStat{
	//testing if we're building dll or just linking it
	#ifdef FLICKSTAT_INTERNAL //if building
		#define FLICKSTAT_API _declspec(dllexport)
	#else 
		#ifdef FLICKSTAT_EXTERNAL //if linking
			#define FLICKSTAT_API _declspec(dllimport)
		#else
			#define FLICKSTAT_API //if static link needed
		#endif
	#endif

	//defining helpers
	#define uint unsigned int
	#define ulong unsigned long
	#define uchar unsigned char

	//forward declarations
	typedef void* fsSession;
	typedef flickcurl_photo ImgMeta;
	typedef flickcurl_exif* ImgEXIF;
	#define NotificationFunc void (*Func)(void*)

	//defining search params sort type
	typedef enum{ 
		stDatePostedDesc,
		stDatePostedAsc,
		stDateTakenDesc,
		stDateTakenAsc,
		stInterestDesc,
		stInterestAsc,
		stRelevance
	} SortType;

	//defining available image sizes
	typedef enum{
		isSquare, //75x75px
		isThumbnail, //100px on longest side
		isSmall, //240px on longest side
		isMedium, //500px on longest side
		isBig, //640px on longest side
		isLarge, //1024px on longest side
		isOriginal //original image
	} ImgSize;

	//defining search params
	typedef struct{
		string UserID; //user id
		string Tags; //comma-delimited list of tags
		string MachineTags; //comma-delimited list of machine tags

		struct{ //upload date (or 0)
			int Min; //minimal (in unix timestamp)
			int Max; //maximal (in unix timestamp)
		} UploadDate; 

		SortType Sort; //sort type
		ImgSize Size; //image size 

		bool WantImage; //set to true if image is needed
		bool WantMeta; //set to true if meta is needed
		bool WantEXIF; //set to true if EXIF is needed
		bool WantFavorites; //set to true if favorites count is wanted
	} SearchParams;

	//defining image
	typedef struct{
		string Img; //image file path or empry string
		ImgMeta* Meta; //image meta data or NULL
		ImgEXIF* EXIF; //image exif or NULL
	} Image;

	//defining metrics type
	typedef enum{
		mtString, //string
		mtInteger, //integer
		mtFloat, //float
		mtImage //image
	} MetricsType;

	//defining metrics
	typedef struct{
		string Name; //metrics name
		MetricsType Type; //metrics type

		union{ //metrics value
			long i; //for int
			float f; //for float
		};

		string s; //for string
		FILE* Img; //for image
	} DBMetrics;

	//defining DBEntry
	typedef struct{
		size_t Size; //metrics count
		DBMetrics** Metrics; //metrics
	} DBEntry;

	//defining notification types
	typedef enum{
		ntImageDownloaded
	} NotificationType;

	//defining functions
	FLICKSTAT_API fsSession Init(string Key, string Secret); //opens new FlickStat session
	FLICKSTAT_API fsSession Init(string Key, string Secret, uchar ThreadCount); //opens new FlickStat session
	FLICKSTAT_API bool Free(fsSession Session); //closes FlickStat session
	FLICKSTAT_API bool Start(fsSession Session, SearchParams* Params); //starts search
	FLICKSTAT_API bool Stop(fsSession Session); //stops search
	FLICKSTAT_API bool Resume(fsSession Session); //resumes search after stop

	FLICKSTAT_API bool SetWebCacheDir(fsSession Session, string Dir); //sets web cache work dir
	FLICKSTAT_API bool SetWebCacheSize(fsSession Session, uchar CacheSize); //sets web cache size
	FLICKSTAT_API bool SetResultPageSize(fsSession Session, uint PageSize); //sets result page size (must be greater than 0 and 
																			//lower than 500)
	FLICKSTAT_API bool SetConnectionTimeout(fsSession Session, uint Msecs); //sets connection timeout
	FLICKSTAT_API bool SetDownloadTimeout(fsSession Session, uint Msecs); //sets download timeout (when connected)
	
	FLICKSTAT_API uint GetImagesCount(fsSession Session); //returns avalable images count for current session
	FLICKSTAT_API Image* GetImage(fsSession Session); //returns next image for current session or NULL
	FLICKSTAT_API Image* GetImage(string ID, bool WantImage, bool WantMeta, bool WantEXIF); //returns image with current ID or NULL
	FLICKSTAT_API Image* GetImageAsync(fsSession Session); //returns next image for current session ofr NULL

	FLICKSTAT_API string GetTagValue(FlickStat::Image* Img, string TagName); //gets tag value by it's name

	FLICKSTAT_API string GetLastError(fsSession Session); //returns last error for current session
	FLICKSTAT_API bool NoMoreImagesLeft(fsSession Session); //checks if no more images left to download
	FLICKSTAT_API bool IsRunning(fsSession Session); //checks if session is running
	
	FLICKSTAT_API bool SetNotification(fsSession Session, NotificationType Type, NotificationFunc); //sets function for notification
	
	FLICKSTAT_API bool OpenDB(fsSession Session, string FileName); //opens DB with named FileName
	FLICKSTAT_API bool CloseDB(fsSession Session); //closes DB assosiated with current session
	FLICKSTAT_API bool ClearDB(fsSession Session); //clears DB assosiated with current session
	FLICKSTAT_API bool DeleteDB(string FileName); //deletes DB named FileName
	FLICKSTAT_API uint GetDBSize(fsSession Session); //returns current DB entries count
	FLICKSTAT_API DBEntry* GetDBEntry(fsSession Session, uint ID); //returns DB entry
	FLICKSTAT_API bool UpdateDBEntry(fsSession Session, uint ID, DBEntry* NewEntry); //updates DB entry with current data
	FLICKSTAT_API bool DeleteDBEntry(fsSession Session, uint ID); //deletes DB entry
	FLICKSTAT_API bool AddDBEntry(fsSession Session, DBEntry* NewEntry); //adds new DB entry

	FLICKSTAT_API bool InitSearchParams(SearchParams* Params); //inits search params
	FLICKSTAT_API bool FreeSearchParams(SearchParams* Params); //frees search params
	FLICKSTAT_API bool InitImage(Image* Img);
	FLICKSTAT_API bool FreeImage(Image* Img);
	FLICKSTAT_API bool InitDBEntry(DBEntry* Entry, size_t Size);
	FLICKSTAT_API bool FreeDBEntry(DBEntry* Entry);
	FLICKSTAT_API bool InitMetrics(DBMetrics* Metrics);
	FLICKSTAT_API bool FreeMetrics(DBMetrics* Metrics);

	//statistics processor
	namespace Stats{
		FLICKSTAT_API double GetImgRate(ImgMeta* Meta); //returns image rating or 0
		FLICKSTAT_API double GetImgRate(uint Views, uint Favorites);
		FLICKSTAT_API double GetImgRealRate(ImgMeta* Meta, uint Month); //returns image real rating or 0
		FLICKSTAT_API double GetImgRealRate(uint Views, uint Favorites, uint Month);
	};

	//image processor
	namespace ImgProc{
		#define RGB8_CHANNEL_TYPE uchar
		#define RGB8_TYPE uint

		//RGB24 image structure
		typedef struct{
			RGB8_CHANNEL_TYPE* R; //red channel
			RGB8_CHANNEL_TYPE* G; //green channel
			RGB8_CHANNEL_TYPE* B; //blue channel
			uint Width; //width
			uint Height; //height
		} ImgRGB8;

		//LST192f image structure
		#define LST64f_CHANNEL_TYPE double
		typedef struct{
			LST64f_CHANNEL_TYPE** L; //L channel
			LST64f_CHANNEL_TYPE** S; //S channel
			LST64f_CHANNEL_TYPE** T; //T channel
			uint Width; //width
			uint Height; //height
		} ImgLST64f;

		FLICKSTAT_API bool InitImgRGB8(ImgRGB8* Img, uint Width, uint Height); //inits image in RGB 8-bit format
		FLICKSTAT_API bool FreeImgRGB8(ImgRGB8* Img); //frees image

		FLICKSTAT_API ImgRGB8* LoadJpegToRGB8(string FileName); //loads JPEG file into RGB8 image format
		FLICKSTAT_API bool SaveJpegFromRGB8(string FileName, ImgRGB8* Img); //saves RGB8 image to JPEG file

		FLICKSTAT_API bool ImgRGB8_ARGBToRGB(ImgRGB8* Img); //converts ARGB8 to RGB8 format
	
		//sharpness and exposure fuctions
		//See FlickStat manual or SharpnessProc.h for details
		//Code is provided by Marta Egorova
		FLICKSTAT_API int CalcSharpnessEgorova(ImgRGB8* Img); //calculates sharpness using Egorova's method															
																	 //function returns sharpness level:
																	 //< 0 - Error
																	 //= 0 - Imperceptible
																	 //= 1 - Perceptible but Not Annoying
																	 //= 2 - Slightly Annoying
																	 //= 3 - Annoying
																	 //>= 4 - Very annoying
		
		FLICKSTAT_API int CalcSharpnessSafonov(ImgRGB8* Img, float* Metrics, double* Weights); //calculates sharpness using Safonov's method
																							  //function return sharpness level:
																							  //< 0 - Error
																							  //= 0 - Sharp
																							  //= 1 - Bourred
																							  //Metrics is an array[4] of float, Weights is a pointer to a single double
																							  //See manual for details

		FLICKSTAT_API int CalcExposureSafonov(ImgRGB8* Img, float* Metrics, double* Weights); //calculates exposure using Safonov and Egorova's method
																							//function return exposure level:
																							//< 0 - Error
																							//= 0 - Well-exposed
																							//= 1 - Bad-exposed
																							//Metrics is an array[5] of float, Weights is a pointer to a single double
																							//See manual for details

		FLICKSTAT_API bool InitImgLST64f(ImgLST64f* Img, uint Width, uint Height); //inits image in LST 64f-bit format
		FLICKSTAT_API bool FreeImgLST64f(ImgLST64f* Img); //frees image

		FLICKSTAT_API bool InitLST64fChannel(LST64f_CHANNEL_TYPE*** LSTChannel, uint Width, uint Height); //inits LST64f channel
		FLICKSTAT_API bool FreeLST64fChannel(LST64f_CHANNEL_TYPE** LSTChannel, uint Width, uint Height); //frees LST64f channel

		FLICKSTAT_API ImgLST64f* ImgRGB8_To_ImgLST64f(ImgRGB8* Img); //converts from ImgRGB8 to ImgLST64f format

		//Code is provided by Ilia Safonov
		FLICKSTAT_API void GetWaveletFeatures(double* Feat, LST64f_CHANNEL_TYPE** Matrix, int R1, int R2, int C1, int C2); //calculates image wavelet futures
																										    //Feat is array[8] of float
																											//Matrix is LST L channel
																											//R1, R2, C1, C2 are bounding rectangle sizes (top, bottom, left, right)
	};
};