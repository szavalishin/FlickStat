//FlickStat library
//Copyright (c) by Sergey Zavalishin 2011

//ImageProcessor.cpp
//Image Processor - main file

#define SHARPNESS_PROC_EXERNAL //imports SharpnessProc.h functions

#include "stdafx.h"
#include "Core.h"
#include "FlickStat.h"
#include "SharpnessProc.h"

using namespace FlickStat::ImgProc;

ImgRGB8* FlickStat::ImgProc::LoadJpegToRGB8(string FileName){
	//checking if file exists
	FILE* f = fopen(FileName.c_str(), "rb");
	if(!f)
		return 0;
	fclose(f);

	//creating RGB8 structure
	ImgRGB8* Img = new(ImgRGB8);

	//loading new image
	if(!shproc_LoadJPEGFromFile((char*)FileName.c_str(), &Img->R, &Img->G, &Img->B, (int*)&Img->Width, (int*)&Img->Height))
		return Img;

	//if something gone wrong
	free(Img);
	return 0;
};

bool FlickStat::ImgProc::SaveJpegFromRGB8(string FileName, ImgRGB8* Img){
	//checking if file does not exist
	FILE* f = fopen(FileName.c_str(), "rb");
	if(f){
		fclose(f);
		return false;
	}

	//saving file...
	if(!shproc_SaveJPEGToFile((char*)FileName.c_str(), Img->R, Img->G, Img->B, Img->Width, Img->Height))
		return true;
	
	//if error occured
	return false;
}; 

bool FlickStat::ImgProc::InitImgRGB8(ImgRGB8* Img, uint Width, uint Height){
	Img->Width = Width;
	Img->Height = Height;

	Img->R = (RGB8_CHANNEL_TYPE*)malloc(sizeof(RGB8_CHANNEL_TYPE)*Width*Height);
	Img->G = (RGB8_CHANNEL_TYPE*)malloc(sizeof(RGB8_CHANNEL_TYPE)*Width*Height);
	Img->B = (RGB8_CHANNEL_TYPE*)malloc(sizeof(RGB8_CHANNEL_TYPE)*Width*Height);

	if(!(Img->R && Img->G && Img->B)){
		FreeImgRGB8(Img);
		return false;
	}

	return true;
};

bool FlickStat::ImgProc::FreeImgRGB8(ImgRGB8* Img){
	if(!Img)
		return true;

	if(Img->R)
		shproc_MemFree(Img->R);
	if(Img->G)
		shproc_MemFree(Img->G);
	if(Img->B)
		shproc_MemFree(Img->B);

	free(Img);

	return true;
};

bool FlickStat::ImgProc::ImgRGB8_ARGBToRGB(ImgRGB8* Img){
	//testing if we have an image
	if(!Img)
		return false;

	if(!(Img->R && Img->G && Img->B))
		return false;

	//creating intermediate ARGB8 image
	RGB8_TYPE* SrcImg = (RGB8_TYPE*)malloc(sizeof(RGB8_TYPE)*Img->Width*Img->Height);

	if(!SrcImg)
		return false;

	//filling ARGB8 image with data
	for(uint i=0; i < Img->Height; i++)
		for(uint j=0; j < Img->Width; j++)
			SrcImg[i*Img->Width + j] = ((RGB8_TYPE)Img->R[i*Img->Width + j] << 16) + 
										((RGB8_TYPE)Img->G[i*Img->Width + j] << 8) + 
										(RGB8_TYPE)Img->B[i*Img->Width + j] ;

	//killing old ARGB channels
	shproc_MemFree(Img->R);
	shproc_MemFree(Img->G);
	shproc_MemFree(Img->B);

	//removing ARGB
	shproc_SeparateRGBA(SrcImg, &Img->R, &Img->G, &Img->B, Img->Width, Img->Height);
	shproc_ComposeRGBA(SrcImg, Img->R, Img->G, Img->B, Img->Width, Img->Height);

	//killing intermediate image
	free(SrcImg);

	return true;
};

int FlickStat::ImgProc::CalcExposureSafonov(ImgRGB8* Img, float* Metrics, double* Weights){
	//checking if data is correct
	if(!Img)
		return -1;

	if(!(Img->R && Img->G && Img->B))
		return -1;

	//preparing metrics array
	if(!Metrics)
		Metrics = (float*)malloc(sizeof(float)*5);

	if(!Weights)
		Weights = new(double);

	//calculating exposure
	return shproc_CalcExposure(Img->R, Img->G, Img->B, Img->Width, Img->Height, Metrics, Weights);
};

int FlickStat::ImgProc::CalcSharpnessSafonov(ImgRGB8* Img, float* Metrics, double* Weights){
	//checking if data is correct
	if(!Img)
		return -1;

	if(!(Img->R && Img->G && Img->B))
		return -1;

	//preparing metrics array
	if(!Metrics)
		Metrics = (float*)malloc(sizeof(float)*4);

	if(!Weights)
		Weights = new(double);

	//calculating exposure
	return shproc_CalcSharpnessCreteSafonov(Img->R, Img->G, Img->B, Img->Width, Img->Height, Metrics, Weights);
};

int FlickStat::ImgProc::CalcSharpnessEgorova(ImgRGB8* Img){
	//checking if data is correct
	if(!Img)
		return -1;

	if(!(Img->R && Img->G && Img->B))
		return -1;

	//calculating exposure
	return shproc_CalcSharpnessCrete(Img->R, Img->G, Img->B, Img->Width, Img->Height);
};

bool FlickStat::ImgProc::InitLST64fChannel(LST64f_CHANNEL_TYPE*** LSTChannel, uint Width, uint Height){
	//creating channel data
	*LSTChannel = (LST64f_CHANNEL_TYPE**)malloc(sizeof(LST64f_CHANNEL_TYPE*)*Height);

	if(!*LSTChannel)
		return false;

	for(uint i = 0; i < Height; i++){
		(*LSTChannel)[i] = (LST64f_CHANNEL_TYPE*)malloc(sizeof(LST64f_CHANNEL_TYPE)*Width);

		if(!(*LSTChannel)[i]){
			FreeLST64fChannel(*LSTChannel, Width, Height);
			return false;
		}
	}

	return true;
};

bool FlickStat::ImgProc::FreeLST64fChannel(LST64f_CHANNEL_TYPE** LSTChannel, uint Width, uint Height){
	if(!LSTChannel)
		return true;

	for(uint i = 0; i < Height; i++)
		if(LSTChannel[i])
			free(LSTChannel[i]);

	free(LSTChannel);

	return true;
};

bool FlickStat::ImgProc::InitImgLST64f(ImgLST64f* Img, uint Width, uint Height){
	//creating new image
	Img->L = 0;
	Img->S = 0;
	Img->T = 0;
	Img->Width = Width;
	Img->Height = Height;
	
	if(	InitLST64fChannel(&Img->L, Img->Width, Img->Height) &&
		InitLST64fChannel(&Img->S, Img->Width, Img->Height) &&
		InitLST64fChannel(&Img->T, Img->Width, Img->Height) )
		return true;
	else
		FreeImgLST64f(Img);

	return false;
};

bool FlickStat::ImgProc::FreeImgLST64f(ImgLST64f* Img){
	if(!Img)
		return true;

	FreeLST64fChannel(Img->L, Img->Width, Img->Height);
	FreeLST64fChannel(Img->S, Img->Width, Img->Height);
	FreeLST64fChannel(Img->T, Img->Width, Img->Height);

	free(Img);
	
	return true;
};

//LST channel name
typedef enum{LST_L, LST_S, LST_T} LSTChannelName;

//converts RGB8 pixel to LST64f channel pixel
//k = 255/max(R, G, B);
LST64f_CHANNEL_TYPE RGB8_To_LST64f(RGB8_CHANNEL_TYPE RGB_R, RGB8_CHANNEL_TYPE RGB_G, 
	RGB8_CHANNEL_TYPE RGB_B, double k, LSTChannelName LSTChannel){
	switch(LSTChannel){
		case LST_L: return k/sqrt(3.0)*(double)(RGB_R + RGB_G + RGB_B);
		case LST_S: return k/sqrt(2.0)*(double)(RGB_R - RGB_B);
		case LST_T: 
		default: return k/sqrt(6.0)*(double)(RGB_R - 2*RGB_G + RGB_B);
	};
};

ImgLST64f* FlickStat::ImgProc::ImgRGB8_To_ImgLST64f(ImgRGB8* Img){
	//cheking data
	if(!Img)
		return 0;

	//creating LST image
	ImgLST64f* LSTImg = new(ImgLST64f);
	if(!InitImgLST64f(LSTImg, Img->Width, Img->Height))
		return 0;

	//converting RGB8 to LST64f
	for(uint i = 0; i < Img->Width; i++)
		for(uint j = 0; j < Img->Height; j++){
			RGB8_CHANNEL_TYPE R = Img->R[Img->Width*j + i];
			RGB8_CHANNEL_TYPE G = Img->G[Img->Width*j + i];
			RGB8_CHANNEL_TYPE B = Img->B[Img->Width*j + i];

			RGB8_CHANNEL_TYPE Max = max(max(R, G), B);
			double k = (Max > 0) ? 1.0/Max : 1.0; //k = 255/max(R, G, B);

			LSTImg->L[j][i] = RGB8_To_LST64f(R, G, B, k, LST_L);
			LSTImg->S[j][i] = RGB8_To_LST64f(R, G, B, k, LST_S);
			LSTImg->T[j][i] = RGB8_To_LST64f(R, G, B, k, LST_T);
		}

	return LSTImg;
}; 

bool MySignalExtend(double *u, int ul, int l, double *u_ext)
{
////////////////////////////////////////////////////////////////////////////
// This function symmetrically extends signal u of length ul to l samples //
// to the left and to the right. This function must be supplied by the    //
// output signal u_ext of size ul+2*l.                                    //
////////////////////////////////////////////////////////////////////////////

	int i;
	
	if (l < 1 || l > ul)
		return false;

	for (i=0; i<l; i++)
	{
		u_ext[l - 1 - i] = u[i];
		u_ext[l + ul + i] = u[ul - 1 - i];
	}

	for (i=0; i<ul; i++)
		u_ext[l+i] = u[i];

	return true;
}

void MyConvolution(double *u, int ul, double *v, int vl, double *w)
{
///////////////////////////////////////////////////////////////////////////////
// This function convolves signal u of length ul with signal v of length vl. //
// This function must be supplied by the output signal w of size ul+vl-1.    //
///////////////////////////////////////////////////////////////////////////////

	int k, j, wl = ul + vl - 1;

	for (k = 1; k <= wl; k++)
	{
		w[k-1] = 0;
		for (j = max(1, k + 1 - vl); j <= min(k, ul); j++)
			w[k-1] += u[j-1] * v[k - j];
	}
}

double** Transpose(double **m, int u, int v)
{
	int i, j;
	double **t = new double* [v];
	
	for (i = 0; i < v; i++)
	{
		t[i] = new double[u];
		for (j = 0; j < u; j++)
			t[i][j] = m[j][i];
	}

	return t;
}

void MyWave2(double **I, int r_num, int c_num, double **&A, double **&H, double **&V, double **&D, int &u, int &v)
{
/////////////////////////////////////////////////////////////////////////
// This function makes one-level wavelet transform of the 2D signal I  //
// of size r_num x c_num using Daubechies 4-tap filters Lo_D and Hi_D. //
/////////////////////////////////////////////////////////////////////////	

	// Daubechies 4-tap filters 
	double	Lo_D[4] = {-0.129, 0.224, 0.837, 0.483};
	double	Hi_D[4] = {-0.483, 0.837, -0.224, -0.129};
	int		fl = sizeof(Lo_D)/sizeof(double)-1;
	int		i, j;
	int sl = c_num + 2*fl; // length of array s
	double *s = new double[sl];
	double *f = new double[sl + fl];
	double *g = new double[sl + fl];
	int f_dec_c_num = (sl - fl)/2;
	double **f_dec = new double* [r_num]; // decimated array
	double **g_dec = new double* [r_num];

	for (i = 0; i < r_num; i++)
	{
		f_dec[i] = new double[f_dec_c_num];
		g_dec[i] = new double[f_dec_c_num];
	}

	for (i = 0; i < r_num; i++)
	{
		MySignalExtend( I[i], c_num, fl, s );

		MyConvolution(s, sl, Lo_D, fl+1, f);
		MyConvolution(s, sl, Hi_D, fl+1, g);
	    
		for (j = 0; j < f_dec_c_num; j++)
		{
            f_dec[i][j] = f[fl - 1 + 2 + 2*j];
			g_dec[i][j] = g[fl - 1 + 2 + 2*j];
		}
	}

	////////////////////////////////////
	// process f_dec
	////////////////////////////////////
	delete [] s;
	delete [] f;
	delete [] g;
	s = new double[r_num + 2*fl];
	sl = r_num + 2*fl; // length of array s
	
	f = new double[sl + fl];
	g = new double[sl + fl];

	double **f_dec21 = new double* [(sl-fl)/2];
	double **g_dec21 = new double* [(sl-fl)/2];
	for (i = 0; i <(sl-fl)/2; i++)
	{
		f_dec21[i] = new double[f_dec_c_num];
		g_dec21[i] = new double[f_dec_c_num];
	}
		
	// transpose f_dec so it will be a [f_dec_c_num]x[r_num] matrix
	double **temp = Transpose(f_dec, r_num, f_dec_c_num);
	for (i = 0; i < r_num; i++)
	{
		delete [] f_dec[i];
	}
	delete [] f_dec;
	f_dec = temp;
	
	for (j = 0; j < f_dec_c_num; j++)
	{
		MySignalExtend( f_dec[j], r_num, fl, s );

		MyConvolution(s, sl, Lo_D, fl+1, f);
		MyConvolution(s, sl, Hi_D, fl+1, g);
	    
		for (i = 0; i < (sl-fl)/2; i++)
		{
            f_dec21[i][j] = f[fl - 1 + 2 + 2*i];
			g_dec21[i][j] = g[fl - 1 + 2 + 2*i];
		}
	}

	////////////////////////////////////
	// process g_dec
	////////////////////////////////////
	double **f_dec22 = new double* [(sl-fl)/2];
	double **g_dec22 = new double* [(sl-fl)/2];
	for (i = 0; i < (sl-fl)/2; i++)
	{
		f_dec22[i] = new double[f_dec_c_num];
		g_dec22[i] = new double[f_dec_c_num];
	}
		
	// transpose g_dec so it will be a [f_dec_c_num]x[r_num] matrix
	temp = Transpose(g_dec, r_num, f_dec_c_num);
	for (i = 0; i < r_num; i++)
	{
		delete [] g_dec[i];
	}
	delete g_dec;
	g_dec = temp;
	
	for (j = 0; j < f_dec_c_num; j++)
	{
		MySignalExtend( g_dec[j], r_num, fl, s );

		MyConvolution(s, sl, Lo_D, fl+1, f);
		MyConvolution(s, sl, Hi_D, fl+1, g);
	    
		for (i = 0; i < (sl-fl)/2; i++)
		{
            f_dec22[i][j] = f[fl - 1 + 2 + 2*i];
			g_dec22[i][j] = g[fl - 1 + 2 + 2*i];
		}
	}

	A = f_dec21;
	H = g_dec21;
	V = f_dec22;
	D = g_dec22;

	u = (sl-fl)/2;
	v = f_dec_c_num;

	delete [] s;
	delete [] f;
	delete [] g;
	
	for (i = 0; i < f_dec_c_num; i++)
	{
		delete [] f_dec[i];
		delete [] g_dec[i];
	}

	delete [] f_dec;
	delete [] g_dec;
}

void FlickStat::ImgProc::GetWaveletFeatures( double *Feat, LST64f_CHANNEL_TYPE** Matrix, int R1, int R2, int C1, int C2 )
{
	double **A1, **H1, **V1, **D1;
	int u1, v1; // size of matrixes A1, H1, V1, D1;
	double **A2, **H2, **V2, **D2;
	int u2, v2; // size of matrixes A2, H2, V2, D2;
	int i, j;
	double c1=0, c2=0, c3=0, c4=0, c5=0, c6=0, c7=0, c8=0;
	int s1, s2;
	double **MatrixFrag;
	
	MatrixFrag = new double* [R2-R1+1];
	for(i = 0; i < R2-R1+1; i++)
		MatrixFrag[i] = new double[C2-C1+1];

	for(i = R1; i <= R2; i++)
		for(j = C1; j <= C2; j++)
			MatrixFrag[i-R1][j-C1] = Matrix[i][j];
	
	MyWave2(MatrixFrag, R2-R1+1, C2-C1+1, A1, H1, V1, D1, u1, v1);
	MyWave2(A1, u1, v1, A2, H2, V2, D2, u2, v2);

	for (i = 0; i < u1; i++)
		for (j = 0; j < v1; j++) {
			c1 += A1[i][j]*A1[i][j];
			c2 += H1[i][j]*H1[i][j];
			c3 += V1[i][j]*V1[i][j];
			c4 += D1[i][j]*D1[i][j];
		}

	s1 = u1*v1;
	c1 /= s1;
	c2 /= s1;
	c3 /= s1;
	c4 /= s1;

	for (i = 0; i < u2; i++)
		for (j = 0; j < v2; j++) {
			c5 += A2[i][j]*A2[i][j];
			c6 += H2[i][j]*H2[i][j];
			c7 += V2[i][j]*V2[i][j];
			c8 += D2[i][j]*D2[i][j];
		}

	s2 = u2*v2;
	c5 /= s2;
	c6 /= s2;
	c7 /= s2;
	c8 /= s2;
	
	Feat[0] = c1;
	Feat[1] = c2;
	Feat[2] = c3;
	Feat[3] = c4;
	Feat[4] = c5;
	Feat[5] = c6;
	Feat[6] = c7;
	Feat[7] = c8;

	for (i = 0; i < u1; i++)
	{
		delete [] A1[i];
		delete [] H1[i];
		delete [] V1[i];
		delete [] D1[i];
	}

	delete [] A1;
	delete [] H1;
	delete [] V1;
	delete [] D1;

	for (i = 0; i < u2; i++)
	{
		delete [] A2[i];
		delete [] H2[i];
		delete [] V2[i];
		delete [] D2[i];
	}

	delete [] A2;
	delete [] H2;
	delete [] V2;
	delete [] D2;
	
	for(i = 0; i < R2-R1+1; i++)
		delete [] MatrixFrag[i];

	delete [] MatrixFrag;
}