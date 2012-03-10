// TestApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FlickStat.h"
#include <conio.h>
#include <iostream>

using namespace std;

using namespace FlickStat;
#define FLICKSTAT_EXTERNAL

void CalcHist(ulong* Hist, uchar HistSize, double HistMin, double HistMax, LST64f_CHANNEL_TYPE** Channel, uint Height, uint Width){
	//clearing hist
	for(uchar i = 0; i < HistSize; i++)
		Hist[i] = 0;

	//filling hist
	for(uint i = 0; i < Height; i++)
		for(uint j = 0; j < Width; j++)
			Hist[(uchar)((double)(HistSize - 1)*(Channel[i][j] - HistMin)/(HistMax - HistMin) + 0.5)]++;
};

int _tmain(int argc, _TCHAR* argv[])
{
	fsSession fs = FlickStat::Init("76fc6e8497f2654dacab29f9d9e18c18", "1369e07cf7fb3ea8", 15);
	FlickStat::SetResultPageSize(fs, 200);
	FlickStat::SetWebCacheSize(fs, 20);
	FlickStat::SetWebCacheDir(fs, "C:\\Etti\\Uni\\UIR\\3 - Flickr\\Apps\\Test");
	FlickStat::SetConnectionTimeout(fs, 60000);
	FlickStat::SearchParams Params;

	FlickStat::InitSearchParams(&Params);
	Params.Sort = FlickStat::SortType::stDatePostedAsc;
	Params.Size = isMedium;
	Params.Tags =  "nature,landscape,outdoor"; //"architecture,interior,indoor"; //"nature,landscape,outdoor";
	const int ClassNum = 1; //image class number: 1 - outdoor, -1 - indoor
	Params.WantImage = true;

	FILE* f = fopen("C:\\Etti\\Uni\\UIR\\3 - Flickr\\Apps\\Test\\Stats.txt", "w");

	printf("Init done\n");

	FlickStat::Start(fs, &Params);

	//creating metrics
	const uchar HistSize = 8;
	double Wavelets[8];
	ulong HistL[HistSize], HistS[HistSize], HistT[HistSize];

	//downloading images...
	uint maxi = 350;
	for(uint i = 0; i < maxi; i++){
		//getting photo
		FlickStat::Image* img = FlickStat::GetImage(fs);
		cout<<"Downloading photo "<<i+1<<'/'<<maxi<<"\n";

		if(!img){
			printf("%s\n", FlickStat::GetLastError(fs).c_str());
		}else{
			//encounting image metrics
			if(img->Img != ""){
				//loading jpeg image
				ImgProc::ImgRGB8* ImgRGB = ImgProc::LoadJpegToRGB8(img->Img);

				//converting RGB to LST
				ImgProc::ImgLST64f* ImgLST = ImgProc::ImgRGB8_To_ImgLST64f(ImgRGB);
				ImgProc::FreeImgRGB8(ImgRGB);

				//getting metrics
				if(ImgLST){
					//calculating histogramms
					CalcHist(HistL, HistSize, 0.0, 1.75, ImgLST->L, ImgLST->Height, ImgLST->Width);
					CalcHist(HistS, HistSize, -0.75, 0.75, ImgLST->S, ImgLST->Height, ImgLST->Width);
					CalcHist(HistT, HistSize, -0.85, 0.85, ImgLST->T, ImgLST->Height, ImgLST->Width);

					//calculating wavelets
					ImgProc::GetWaveletFeatures(Wavelets, ImgLST->L, 0, ImgLST->Height - 1, 0, ImgLST->Width - 1);
				}

				ImgProc::FreeImgLST64f(ImgLST);
			}

			img->Img = "";
			FreeImage(img);

			//printing info
			string format = (string)"%d 1:%f 2:%f 3:%f 4:%f 5:%d 6:%d 7:%d 8:%d 9:%d 10:%d 11:%d 12:%d 13:%d 14:%d 15:%d 16:%d " +
				"17:%d 18:%d 19:%d 20:%d 21:%d 22:%d 23:%d 24:%d 25:%d 26:%d 27:%d 28:%d\n";

			fprintf(f, format.c_str(), ClassNum, Wavelets[0], Wavelets[1], Wavelets[2], Wavelets[3], HistL[0], HistL[1],
				HistL[2], HistL[3], HistL[4], HistL[5], HistL[6], HistL[7], HistS[0], HistS[1], HistS[2], 
				HistS[3], HistS[4], HistS[5], HistS[6], HistS[7], HistT[0], HistT[1], HistT[2], HistT[3], 
				HistT[4], HistT[5], HistT[6], HistT[7]);
		}
	}

	FlickStat::Stop(fs);

	fclose(f);

	FlickStat::Free(fs);
	printf("Free done\n");

	_getch();
	return 0;
}

