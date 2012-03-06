// TestApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FlickStat.h"
#include <conio.h>
#include <iostream>
#include <agents.h>

using namespace std;

using namespace FlickStat;
#define FLICKSTAT_EXTERNAL

void SaveHist(ulong* hist, uchar size){
	FILE* f = fopen("C:\\Etti\\Uni\\UIR\\3 - Flickr\\Apps\\Test\\stats.txt", "w");

	for(uchar i = 0; i < size; i++)
		fprintf(f, "%d: %d\n", (uchar)((float)(i+1)/size*100), hist[i]);

	fclose(f);
};

string GetTag(FlickStat::Image* Img, string TagName){
	if(!Img)
		return 0;

	if(!Img->EXIF)
		return 0;

	for(uint i = 0; Img->EXIF[i] != 0; i++)
		if(TagName == Img->EXIF[i]->label)
			return Img->EXIF[i]->raw; 

	return "";
};

float GetRealRate(double Rate, uint Month){
	return (Month <= 2)?Rate:0.08*Rate/(0.1/(pow(Month, 0.5)));
};

int _tmain(int argc, _TCHAR* argv[])
{
	fsSession fs = FlickStat::Init("76fc6e8497f2654dacab29f9d9e18c18", "1369e07cf7fb3ea8", 15);
	FlickStat::SetResultPageSize(fs, 500);
	FlickStat::SetWebCacheSize(fs, 20);
	FlickStat::SetWebCacheDir(fs, "C:\\Etti\\Uni\\UIR\\3 - Flickr\\Apps\\Test");
	FlickStat::SetConnectionTimeout(fs, 60000);
	FlickStat::SearchParams Params;

	FlickStat::InitSearchParams(&Params);
	Params.Sort = FlickStat::SortType::stInterestDesc;
	Params.UploadDate.Max = 1323216000;
	Params.UploadDate.Min = 1323216000 - 3600*24*30*24;
	Params.Tags = "nature";
	Params.WantEXIF = true;
	Params.WantImage = true;
	Params.WantMeta = true;
	Params.WantFavorites = true;

	FILE* f = fopen("C:\\Etti\\Uni\\UIR\\3 - Flickr\\Apps\\Test\\Stats.txt", "w");

	printf("Init done\n");

	FlickStat::Start(fs, &Params);

	//creating metrics
	string WhiteBalance="", Make="", Model="", Exposure="", Aperture="", ISOSpeed="", 
		Flash="", FocalLength="", CreatorTool="", id="";
	uint Views=0, Favorites=0;
	int SharpnessSafonov=0, SharpnessEgorova=0, ExposureMetr=0;
	double ShSaf_Weights=0, Exp_Weights=0;
	float ShSaf_Features[4] = {0, 0, 0, 0}, Exp_Features[5] = {0, 0, 0, 0, 0};

	//downloading images...
	uint maxi = 50000;
	for(uint i = 0; i < maxi; i++){
		//getting photo
		FlickStat::Image* img = FlickStat::GetImage(fs);
		cout<<"Downloading photo "<<i+1<<'/'<<maxi<<"\n";

		if(!img){
			printf("%s\n", FlickStat::GetLastError(fs).c_str());
		}else{
			//getting meta
			if(img->Meta){
				id = img->Meta->id;
				Views = img->Meta->fields[PHOTO_FIELD_views].integer;
				Favorites = img->Meta->fields[PHOTO_FIELD_favorites].integer;
			}

			//getting exifs
			if(img->EXIF){
				WhiteBalance = GetTag(img, "White Balance");
				Make = GetTag(img, "Make");
				Model = GetTag(img, "Model");
				Exposure = GetTag(img, "Exposure");
				Aperture = GetTag(img, "Aperture");
				ISOSpeed = GetTag(img, "ISO Speed");
				Flash = GetTag(img, "Flash");
				FocalLength = GetTag(img, "Focal Length");
				CreatorTool = GetTag(img, "Creator Tool");
			}

			//encounting image metrics
			if(img->Img != ""){
				//loading jpeg image
				ImgProc::ImgRGB8* ImgRGB = ImgProc::LoadJpegToRGB8(img->Img);

				//getting metrics
				if(ImgRGB){
					//ImgProc::ImgRGB8_ARGBToRGB(ImgRGB);

					SharpnessSafonov = ImgProc::CalcSharpnessSafonov(ImgRGB, ShSaf_Features, &ShSaf_Weights); 
					SharpnessEgorova = ImgProc::CalcSharpnessEgorova(ImgRGB); 
					ExposureMetr = ImgProc::CalcExposureSafonov(ImgRGB, Exp_Features, &Exp_Weights);
				}

				ImgProc::FreeImgRGB8(ImgRGB);
			}

			FreeImage(img);

			//printing info
			string format = 
				(string)"id=\"%s\" Views=%d Favorites=%d WhiteBalance=\"%s\" Make=\"%s\" Model=\"%s\" Exposure=\"%s\" Aperture=\"%s\" " + 
				"ISOSpeed=\"%s\" Flash=\"%s\" FocalLength=\"%s\" CreatorTool=\"%s\" " +
				"ShSaf=%d [0]=%f [1]=%f [2]=%f [3]=%f Weights=%f ShEgr=%d Exp=%d [0]=%f [1]=%f [2]=%f [3]=%f [4]=%f Weights=%f\n";

			//printf(format.c_str(), Views, Favorites, WhiteBalance.c_str(), Make.c_str(), Model.c_str(), Exposure.c_str(), Aperture.c_str(), 
			//	ISOSpeed.c_str(), Flash.c_str(), FocalLength.c_str(), CreatorTool.c_str(), SharpnessSafonov, ShSaf_Features[0], ShSaf_Features[1],
			//	ShSaf_Features[2], ShSaf_Features[3], ShSaf_Weights, SharpnessEgorova, ExposureMetr, Exp_Features[0], Exp_Features[1], 
			//	Exp_Features[2], Exp_Features[3], Exp_Features[4], Exp_Weights);

			fprintf(f, format.c_str(), id.c_str(), Views, Favorites, WhiteBalance.c_str(), Make.c_str(), Model.c_str(), Exposure.c_str(), Aperture.c_str(), 
				ISOSpeed.c_str(), Flash.c_str(), FocalLength.c_str(), CreatorTool.c_str(), SharpnessSafonov, ShSaf_Features[0], ShSaf_Features[1],
				ShSaf_Features[2], ShSaf_Features[3], ShSaf_Weights, SharpnessEgorova, ExposureMetr, Exp_Features[0], Exp_Features[1], 
				Exp_Features[2], Exp_Features[3], Exp_Features[4], Exp_Weights);
		}

		//updating file every 100 images
		if(i%100 == 0)
			fflush(f);
	}

	FlickStat::Stop(fs);

	fclose(f);

	FlickStat::Free(fs);
	printf("Free done\n");

	_getch();
	return 0;
}

