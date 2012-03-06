//FlickStat library
//Copyright (c) by Sergey Zavalishin 2011

//StatisticsProcessor.cpp
//Statistics Processor - main file

#include "stdafx.h"
#include "FlickStat.h"

using namespace FlickStat;

double FlickStat::Stats::GetImgRate(ImgMeta* Meta){
	//checking args
	if(Meta)
		return GetImgRate(Meta->fields[PHOTO_FIELD_favorites].integer, 
							Meta->fields[PHOTO_FIELD_views].integer);

	return 0;
};

double FlickStat::Stats::GetImgRate(uint Views, uint Favorites){
	if(Views)
		return (double)Favorites/Views;

	return 0;
};

double FlickStat::Stats::GetImgRealRate(ImgMeta* Meta, uint Month){
	//checking args
	if(Meta)
		//calculating real rate
		return (Month <= 2) ? GetImgRate(Meta) : 0.08*GetImgRate(Meta)/(0.1/(pow(Month, 0.5)));

	return 0; //return 0 on error
};

double FlickStat::Stats::GetImgRealRate(uint Views, uint Favorites, uint Month){
	if(Views)
		return (Month <= 2) ? GetImgRate(Views, Favorites) : 0.08*GetImgRate(Views, Favorites)/(0.1/(pow(Month, 0.5)));

	return 0;
};