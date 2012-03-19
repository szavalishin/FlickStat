// comp-kernel.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>

using namespace std;

#define VectSize 28

typedef struct{
	int ClassNum;
	unsigned int Size;
	float *Val;
} ParamVector;

string format_in = 
	(string)"%d 1:%f 2:%f 3:%f 4:%f 5:%f 6:%f 7:%f 8:%f 9:%f 10:%f 11:%f 12:%f " + 
	"13:%f 14:%f 15:%f 16:%f 17:%f 18:%f 19:%f 20:%f 21:%f 22:%f 23:%f 24:%f " +
	"25:%f 26:%f 27:%f 28:%f\n";

void NormalizeVector(ParamVector *X, const ParamVector *Max, const ParamVector *Min){
	for(unsigned int i = 0; i < Max->Size; i++)
		if(Max->Val[i] != 0)
			X->Val[i] = (X->Val[i] - Min->Val[i])/(Max->Val[i] - Min->Val[i]);
};

unsigned int LoadVectors(ParamVector **Xi, wchar_t *in_file){
	FILE *fin = _wfopen(in_file, L"r");

	unsigned int i = 0;
	while(fscanf(fin, format_in.c_str(), 
			&((*Xi)[i].ClassNum), &((*Xi)[i].Val[0]), &((*Xi)[i].Val[1]), &((*Xi)[i].Val[2]), 
			&((*Xi)[i].Val[3]), &((*Xi)[i].Val[4]), &((*Xi)[i].Val[5]), &((*Xi)[i].Val[6]), 
			&((*Xi)[i].Val[7]), &((*Xi)[i].Val[8]), &((*Xi)[i].Val[9]), &((*Xi)[i].Val[10]), 
			&((*Xi)[i].Val[11]), &((*Xi)[i].Val[12]), &((*Xi)[i].Val[13]), &((*Xi)[i].Val[14]), 
			&((*Xi)[i].Val[15]), &((*Xi)[i].Val[16]), &((*Xi)[i].Val[17]), &((*Xi)[i].Val[18]), 
			&((*Xi)[i].Val[19]), &((*Xi)[i].Val[20]), &((*Xi)[i].Val[21]), &((*Xi)[i].Val[22]), 
			&((*Xi)[i].Val[23]), &((*Xi)[i].Val[24]), &((*Xi)[i].Val[25]), &((*Xi)[i].Val[26]), 
			&((*Xi)[i].Val[27])) != EOF)
	{
		*Xi = (ParamVector*)realloc((void*)*Xi, (++i + 1)*sizeof(ParamVector));
		(*Xi)[i].Val = (float*)malloc(sizeof(float)*VectSize);
		(*Xi)[i].Size = VectSize;
	}

	fclose(fin);
	return i;
};

//Xi should be &Xi[Xl], where Xl is X param number
float CalcKernel(const ParamVector *X, const ParamVector *Y){
	//calculating Chi2 kernel
	float Chi = 0;
	unsigned int Size = min(X->Size, Y->Size);
	for(unsigned int i = 0; i < Size; i++){
		float x = X->Val[i];
		float y = Y->Val[i];
		if(x + y != 0)
			Chi += 2*(pow(x - y, 2)/(x + y));
	}
	
	//calculating exp Chi2 Kernel
	float gamma = 0.5;
	return exp(-gamma*Chi); //return 1 - Chi; - for standart Chi2
};

void CalcNewVector(const ParamVector *Xi, ParamVector *X){
	ParamVector tmp = *X;
	tmp.Val = (float*)malloc(sizeof(float)*X->Size);
	memcpy(tmp.Val, X->Val, X->Size*sizeof(float));

	for(unsigned int i = 0; i < X->Size; i++)
		X->Val[i] = CalcKernel(&tmp, &Xi[i]);

	free(tmp.Val);
};

void CalcVectors(ParamVector *Xi, wchar_t *out_file, unsigned int XSize){
	FILE *fout = _wfopen(out_file, L"w");

	ParamVector X;
	X.Size = XSize;
	X.Val = (float*)malloc(X.Size*sizeof(float));

	//reading vector from input file
	for(unsigned int i = 0; i < XSize; i++){
		//copying vector data to new vector
		memcpy(X.Val, Xi[i].Val, Xi[i].Size*sizeof(float));
		X.ClassNum = Xi[i].ClassNum;
		
		//calculating new vector
		CalcNewVector(Xi, &X);

		//printing vector
		fprintf(fout, "%d 0:%d", X.ClassNum, i+1);
		for(unsigned int j = 0; j < XSize; j++){
			fprintf(fout, " %d:%f", j+1, X.Val[j]);
		}
		fprintf(fout, "\n");
	};

	free(X.Val);
	fclose(fout);
};

void GetNormCoefs(const ParamVector *Xi, ParamVector *Max, ParamVector *Min, unsigned int VectCount){
	for(unsigned int i = 0; i < Max->Size; i++){
		Max->Val[i] = Xi[0].Val[i];
		Min->Val[i] = Xi[0].Val[i];
	}

	for(unsigned int i = 0; i < VectCount; i++)
		for(unsigned int j = 0; j < Max->Size; j++){
			if(Max->Val[j] < Xi[i].Val[j])
				Max->Val[j] = Xi[i].Val[j];
			
			if(Min->Val[j] > Xi[i].Val[j])
				Min->Val[j] = Xi[i].Val[j];
		}
};

void NormalizeVectors(ParamVector *Xi, unsigned int VectCount){
	ParamVector Min, Max;
	Min.Size = VectSize;
	Min.Val = (float*)malloc(sizeof(float)*VectSize);
	Max.Size = VectSize;
	Max.Val = (float*)malloc(sizeof(float)*VectSize);

	GetNormCoefs(Xi, &Max, &Min, VectCount);

	for(unsigned int i = 0; i < VectCount; i++)
		NormalizeVector(&Xi[i], &Max, &Min);

	free(Min.Val);
	free(Max.Val);
};

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc < 3){
		printf("Not enough arguments. Wants: comp-kernel in_file out_file\n");
		return 0;
	}

	ParamVector *Xi = (ParamVector*)malloc(sizeof(ParamVector));
	Xi[0].Val = (float*)malloc(sizeof(float)*VectSize);
	Xi[0].Size = 28;

	//loading input vectors
	unsigned int VectCount = LoadVectors(&Xi, argv[1]);

	//norm vectors
	NormalizeVectors(Xi, VectCount);

	//calculating kernel
	CalcVectors(Xi, argv[2], VectCount);

	//killing resorces
	for(unsigned int i = 0; i < VectCount; i++)
		free(Xi[i].Val);
	free(Xi);

	return 0;
}

