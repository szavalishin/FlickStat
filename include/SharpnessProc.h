//Sharpness and Exposure functions
//Copyright (c) by Marta Egorova and Ilia Safonov 2011

//SharpnessProc library
//Copyright (c) by Sergey Zavalishin 2011

//SharpnessProc.h
//SharpnessProc - Main header file

#pragma once

#include "stdafx.h"

#include <stdio.h>
#include <malloc.h>
#include <string>

//testing if we're building dll or just linking it
#ifdef SHARPNESS_PROC_INTERNAL //if building
	#define DLLSPEC _declspec(dllexport)
#else 
	#ifdef SHARPNESS_PROC_EXERNAL //if linking
		#define DLLSPEC _declspec(dllimport)
	#else
		#define DLLSPEC //if static link needed
	#endif
#endif

//defining functions
DLLSPEC int shproc_LoadJPEGFromFile(char* filename , unsigned char ** pImageR, 
					unsigned char ** pImageG, unsigned char ** pImageB, 
					int* Width, int* Height); 
DLLSPEC int shproc_SaveJPEGToFile(char *filename , unsigned char *pImageR, unsigned char *pImageG, 
					unsigned char *pImageB, int width, int height);
DLLSPEC int shproc_SetBlack(unsigned char* pSrcImageR, unsigned char* pSrcImageG, unsigned char* pSrcImageB, int w, int h);
DLLSPEC int shproc_CalcSharpnessCreteSafonov (unsigned char* pSrcImageR, unsigned char* pSrcImageG, unsigned char* pSrcImageB, int w, int h, 
							   float* sharpFeatures, double* sumWeights);
DLLSPEC int shproc_CalcSharpnessCrete (unsigned char* pSrcImageR, unsigned char* pSrcImageG, unsigned char* pSrcImageB, int w, int h);
DLLSPEC int shproc_CalcExposure(unsigned char* pSrcImageR, unsigned char* pSrcImageG, unsigned char* pSrcImageB, int w, int h,
				 float* exposureFeatures, double* sumWeights);
DLLSPEC int shproc_SeparateRGBA(unsigned int* pSrcImage, unsigned char** pImageR, unsigned char** pImageG, unsigned char** pImageB, int w, int h);
DLLSPEC int shproc_ComposeRGBA(unsigned int* pDstImage, unsigned char* pImageR, unsigned char* pImageG, unsigned char* pImageB, int w, int h);

//use this function to delete memory blocks inside this dll
DLLSPEC void shproc_MemFree(void* ptr);