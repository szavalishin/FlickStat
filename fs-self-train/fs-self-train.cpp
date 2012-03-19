// fs-self-train.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <conio.h>
#include "flickstat.h"
#include <direct.h>

#define TrainFileName "train_fsst.txt"
#define DownloadFileName "download_fsst.txt"
#define PredFileName "pred.txt"
#define ModelFileName "model.txt"

#define FLICKSTAT_EXTERNAL

using namespace std;
using namespace FlickStat;

fsSession PrepareSession(int ClassNum){
	//creating fs session
	fsSession fs = FlickStat::Init("76fc6e8497f2654dacab29f9d9e18c18", "1369e07cf7fb3ea8", 15);
	FlickStat::SetResultPageSize(fs, 100);
	FlickStat::SetWebCacheSize(fs, 20);

	//creating web cache dir
	char *wd = getcwd(0, 0);
	mkdir(((string)wd + ((ClassNum == 1) ? "\\cache_out" : "\\cache_in")).c_str());
	FlickStat::SetWebCacheDir(fs, (string)wd + ((ClassNum == 1) ? "\\cache_out" : "\\cache_in"));
	
	FlickStat::SetConnectionTimeout(fs, 60000);
	FlickStat::SearchParams* Params = new(FlickStat::SearchParams);

	FlickStat::InitSearchParams(Params);
	Params->Sort = FlickStat::SortType::stRelevance;
	Params->Size = isMedium;

	Params->Tags = (ClassNum == 1) ? "landscape" : "interior,architecture";
	Params->WantImage = true;

	FlickStat::Start(fs, Params);
	FlickStat::Stop(fs);

	return fs;
};

void CalcHist(ulong* Hist, uchar HistSize, double HistMin, double HistMax, LST64f_CHANNEL_TYPE** Channel, uint Height, uint Width){
	//clearing hist
	for(uchar i = 0; i < HistSize; i++)
		Hist[i] = 0;

	//filling hist
	for(uint i = 0; i < Height; i++)
		for(uint j = 0; j < Width; j++)
			Hist[(uchar)((double)(HistSize - 1)*(Channel[i][j] - HistMin)/(HistMax - HistMin) + 0.5)]++;
};

void GetImages(fsSession fs, char* FileName, unsigned int PortionSize, int ClassNum){
	//resuming session
	FlickStat::Resume(fs);

	//creating metrics
	const uchar HistSize = 8;
	double Wavelets[8];
	ulong HistL[HistSize], HistS[HistSize], HistT[HistSize];

	FILE* fout = fopen(FileName, "w");

	//downloading images...
	for(uint i = 0; i < PortionSize; i++){
		//getting photo
		FlickStat::Image* img = FlickStat::GetImage(fs);
		printf("Downloading photo %d/%d\n", i+1, PortionSize);

		if(!img){
			printf("%s\n", FlickStat::GetLastError(fs).c_str());
			FlickStat::Resume(fs);
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

			FreeImage(img);

			//printing info
			string format = (string)"%d 1:%f 2:%f 3:%f 4:%f 5:%d 6:%d 7:%d 8:%d 9:%d 10:%d 11:%d 12:%d 13:%d 14:%d 15:%d 16:%d " +
				"17:%d 18:%d 19:%d 20:%d 21:%d 22:%d 23:%d 24:%d 25:%d 26:%d 27:%d 28:%d\n";

			std::fprintf(fout, format.c_str(), ClassNum, Wavelets[0], Wavelets[1], Wavelets[2], Wavelets[3], HistL[0], HistL[1],
				HistL[2], HistL[3], HistL[4], HistL[5], HistL[6], HistL[7], HistS[0], HistS[1], HistS[2], 
				HistS[3], HistS[4], HistS[5], HistS[6], HistS[7], HistT[0], HistT[1], HistT[2], HistT[3], 
				HistT[4], HistT[5], HistT[6], HistT[7]);
		}
	}

	FlickStat::Stop(fs);

	std::fclose(fout);
};

void FreeSession(fsSession fs){
	FlickStat::Stop(fs);
	FlickStat::Free(fs);
};

void DownloadData(const char* DownloadFile, unsigned int PortionSize, bool RandLabels, fsSession fs_in, fsSession fs_out){
	//downloading first class
	char PortionSizeS[5];
	_itoa(PortionSize/2, PortionSizeS, 10);
	GetImages(fs_in, "tmp1.txt", PortionSize/2, -1);

	//downloading second class
	GetImages(fs_out, "tmp2.txt", PortionSize/2, 1);

	//adding data to download file and randomizing labels
	FILE *fout = fopen("tmp3.txt", "w");
	FILE *fin = fopen("tmp1.txt", "r");

	int class_in = 0, class_out = 1;
	char str[1000];

	//writing first class
	while(fscanf(fin, "%d %[^\n]s", &class_in, str) != EOF){
		class_out = RandLabels ? class_out*-1 : class_in;
		std::fprintf(fout, "%d %s\n", class_out, str);
	}
	std::fclose(fin);

	//writing second class
	fin = fopen("tmp2.txt", "r");
	while(fscanf(fin, "%d %[^\n]s", &class_in, str) != EOF){
		class_out = RandLabels ? class_out*-1 : class_in;
		std::fprintf(fout, "%d %s\n", class_out, str);
	}
	std::fclose(fin);
	std::fclose(fout);

	//scaling data
	system(((string)"svm-scale -l 0 tmp3.txt>" + DownloadFile).c_str());

	//closing files
	unlink("tmp1.txt");
	unlink("tmp2.txt");
	unlink("tmp3.txt");
};

void AppendData(const char *AppendTo, const char *AppendFrom){
	FILE *fout = fopen(AppendTo, "a");
	FILE *fin = fopen(AppendFrom, "r");

	//writing to out file
	char str[1];
	while(fread(str, sizeof(char), 1, fin))
		fwrite(str, sizeof(char), 1, fout);
	
	std::fclose(fin);
	std::fclose(fout);
};

void AppendDataB(const char *AppendTo, const char *AppendFrom){
	FILE *fout = fopen(AppendTo, "a");
	FILE *fin = fopen(AppendFrom, "r");
	FILE *fpred = fopen(PredFileName, "r");

	char str[1000];
	int class_in = 0,
		class_out = 0,
		class_old = 0;
	float prec = 0;

	fscanf(fpred, "%*[^\n]s"); //skipping first line

	//reading input file and pred file...
	while(fscanf(fin, "%d %[^\n]s", &class_in, str) != EOF && 
		fscanf(fpred, "%d %*f %f", &class_out, &prec) != EOF)
			//if precision is big enough then adding data to training set
			//if(class_out == class_in && (class_in == 1 ? prec : 1 - prec) > threashold_value)
			if((prec > 0.5 ? prec : 1 - prec) < 0.75)
				fprintf(fout, "%d %s\n", class_out, str);
	
	std::fclose(fin);
	std::fclose(fout);
	std::fclose(fpred);
};

unsigned int CountWellRecognizedObjects(const char *PredFile, float Threshold){
	FILE *fin = fopen(PredFile, "r");

	float prec = 0;
	unsigned int ObjCount = 0;

	fscanf(fin, "%*[^\n]s"); //skipping first line

	//encounting well-recognized objects
	while(fscanf(fin, "%*d %*f %f", &prec) != EOF)
		if((prec > 0.5 ? prec : 1 - prec) > Threshold)
			ObjCount++;

	std::fclose(fin);
	return ObjCount;
};

unsigned int TestAlgQuality(const char *TestFile, float Threshold){
	//predicting labels in test file
	system(((string)"svm-predict -b 1 " + TestFile + " " + ModelFileName + " " + PredFileName).c_str());

	//encounting accuracy
	return CountWellRecognizedObjects(PredFileName, Threshold);
};

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc < 5){
		printf("Wrong arguments. Wants: fs-self-train.exe portion_size rec_threshold rec_count_threshold test_file\n");
		return 0;
	}

	//reading args
	int PortionSize = 100;
	float RecThreshold = 0.5, RecCountThreshold = 0.75;
	char testing_file[300] = "";

	wcstombs(testing_file, argv[4], 300);
	swscanf(argv[1], L"%d", &PortionSize);
	swscanf(argv[2], L"%f", &RecThreshold);
	swscanf(argv[3], L"%f", &RecCountThreshold);

	//downloading first data
	fsSession //sessions for indoor and outdoor classes
		fs_in = PrepareSession(-1), 
		fs_out = PrepareSession(1); 
	
	DownloadData(DownloadFileName, PortionSize, false, fs_in, fs_out);

	//training alg...
	unsigned int WellRecognized = 0;
	do{
		//appending downloaded data to training set
		AppendData(TrainFileName, DownloadFileName);

		//self-training alg with training set
		system(((string)"self-train.exe 100 0.1" + " " + TrainFileName + " " + testing_file).c_str());

		//downloading another portion of data
		DownloadData(DownloadFileName, PortionSize, false, fs_in, fs_out);

		//testing alg with new data
		WellRecognized = TestAlgQuality(DownloadFileName, RecThreshold);
	}while((float)WellRecognized/PortionSize < RecCountThreshold);

	//testing alg with test file
	system(((string)"svm-predict -b 1 " + testing_file + " " + ModelFileName + " " + PredFileName).c_str());
	system(((string)"calc-proc.exe " + testing_file + " " + PredFileName).c_str());

	//killing tmp
	unlink(TrainFileName);
	unlink(DownloadFileName);
	FreeSession(fs_in);
	FreeSession(fs_out);
	unlink(PredFileName);

	_getch();

	return 0;
}

