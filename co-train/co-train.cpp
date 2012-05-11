// co-train.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "svm.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <string>

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))
#define uint unsigned int

#define PredFile "co_pred.txt"
#define PredFileA "co_pred_a.txt"
#define PredFileB "co_pred_b.txt"
#define OutFile "co_out.txt"

using namespace std;

char* readline(FILE *input, char **line, int *max_line_len)
{
	int len;
	
	if(fgets(*line, *max_line_len, input) == NULL)
		return NULL;

	while(strrchr(*line,'\n') == NULL)
	{
		(*max_line_len) *= 2;
		*line = (char *) realloc(*line, *max_line_len);
		len = (int) strlen(*line);
		if(fgets(*line+len, *max_line_len-len,input) == NULL)
			break;
	}
	return *line;
}

svm_node *read_problem(const char *filename, svm_problem *prob, uint max_size)
{
	int elements, max_index, inst_max_index, i, j;
	FILE *fp = fopen(filename,"r");
	char *endptr;
	char *idx, *val, *label;

	if(fp == NULL)
	{
		fprintf(stderr,"can't open input file %s\n",filename);
		return 0;
	}

	prob->l = 0;
	elements = 0;

	if(max_size == 0)
		max_size = 0xFFFFFFFF;

	int max_line_len = 1024;
	uint cur_size = 0;
	char *line = Malloc(char,max_line_len);
	while(readline(fp, &line, &max_line_len) != NULL && prob->l < max_size)
	{
		char *p = strtok(line," \t"); // label

		// features
		while(1)
		{
			p = strtok(NULL," \t");
			if(p == NULL || *p == '\n') // check '\n' as ' ' may be after the last feature
				break;
			++elements;
		}
		++elements;
		++prob->l;
	}
	rewind(fp);

	prob->y = Malloc(double,prob->l);
	prob->x = Malloc(struct svm_node *,prob->l);
	svm_node *x_space = Malloc(struct svm_node,elements);

	max_index = 0;
	j=0;
	for(i=0;i<prob->l;i++)
	{
		inst_max_index = -1; // strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0
		readline(fp, &line, &max_line_len);
		prob->x[i] = &x_space[j];
		label = strtok(line," \t\n");
		if(label == NULL) // empty line
			return 0;

		prob->y[i] = strtod(label,&endptr);
		if(endptr == label || *endptr != '\0')
			return 0;

		while(1)
		{
			idx = strtok(NULL,":");
			val = strtok(NULL," \t");

			if(val == NULL)
				break;

			errno = 0;
			x_space[j].index = (int) strtol(idx,&endptr,10);
			if(endptr == idx || errno != 0 || *endptr != '\0' || x_space[j].index <= inst_max_index)
				return 0;
			else
				inst_max_index = x_space[j].index;

			errno = 0;
			x_space[j].value = strtod(val,&endptr);
			if(endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
				return 0;

			++j;
		}

		if(inst_max_index > max_index)
			max_index = inst_max_index;
		x_space[j++].index = -1;
	}

	/*
	for(i=0;i<prob->l;i++)
	{
		if (prob->x[i][0].index != 0)
		{
			fprintf(stderr,"Wrong input format: first column must be 0:sample_serial_number\n");
			exit(1);
		}
		if ((int)prob->x[i][0].value <= 0 || (int)prob->x[i][0].value > max_index)
		{
			fprintf(stderr,"Wrong input format: sample_serial_number out of range\n");
			exit(1);
		}
	}
	*/
	free(line);
	fclose(fp);
	return x_space;
}

void SetParamValues(svm_parameter *param){
	param->svm_type = C_SVC;
	param->kernel_type = CHI2;
	param->degree = 3;
	param->gamma = 0.5;
	param->coef0 = 0;
	param->nu = 0.5;
	param->cache_size = 100;
	param->C = 1;
	param->eps = 1e-3;
	param->p = 0.1;
	param->shrinking = 1;
	param->probability = 1;
	param->nr_weight = 0;
	param->weight_label = NULL;
	param->weight = NULL;
};

bool ShiftDataPointer(svm_problem *data_set, int shift){
	//shifting input data pointer:
	//  !  <- pointer                  =>                  !
	//||  test_set  |  train_set  ||   =>   ||  test_set  |  train_set  ||  

	//warning: negative shifts are unsafe, but possible!
	if(shift < data_set->l){
		data_set->x += shift;
		data_set->y += shift;
		data_set->l -= shift;

		return true;
	}
	return false;
}

svm_node *LoadPredictedLabels(const char *prob_file, uint pos, uint count){
	//loading predicted labels file
	FILE *fin = fopen(prob_file, "r");

	if(fin){
		svm_node *label_set = Malloc(svm_node, count);

		char *buffer = Malloc(char, 1024);
		uint i = 0;
		int max_line_length = 1024;

		while(readline(fin, &buffer, &max_line_length) && i < count + pos){
			if(i >= pos){
				float value;
				int index;
				sscanf(buffer, "%d %f", &index, &value);
				label_set[i - pos].index = index;
				label_set[i - pos].value = value;
			}
			i++;
		};

		free(buffer);
		fclose(fin);

		return label_set;
	};
	return 0;
};

void CopyLabels(double *old_labels, svm_node *new_labels, uint pos, uint count){
	if(old_labels && new_labels)
		for(uint i = pos; i < pos + count; i++)
			old_labels[i] = new_labels[i].index; //new_labels[i].value > 0.6 ? new_labels[i].index : old_labels[i]; // <- active learning
};

void CopyLabels(double *old_labels, double *new_labels, uint pos, uint count){
	if(old_labels && new_labels)
		memcpy((void*)&old_labels[pos], (void*)&new_labels[pos], sizeof(double)*count);
};

svm_problem *FilterNoises(svm_problem *data, svm_node *new_labels, uint pos, uint count, double threshold){
	svm_problem *new_data = new svm_problem;
	new_data->y = new double[data->l];
	new_data->x = new svm_node*[data->l];
	new_data->l = 0;

	for(uint i = 0; i < pos + count; i++)
		if(new_labels[i].value > threshold || i < pos){
			new_data->l++;
			new_data->y[new_data->l - 1] = i < pos ? data->y[i] : new_labels[i].index; 
			new_data->x[new_data->l - 1] = data->x[i]; 	
		}

	new_data->y = (double*)realloc(new_data->y, sizeof(double) * new_data->l);
	new_data->x = (svm_node**)realloc(new_data->x, sizeof(svm_node*) * new_data->l);

	return new_data;
};

void PredictLabels(const svm_model *model, svm_problem *data_set, svm_node *labels){
	double *value = new double[svm_get_nr_class(model)];

	for(uint i = 0; i < data_set->l; i++){
		labels[i].index = (int)svm_predict_probability(model, data_set->x[i], value);
		labels[i].value = value[0] > value[1] ? value[0] : value[1];
	};

	delete [] value;
};

void PrintLabels(const char *out_file, const svm_problem *data){
	FILE *fout = fopen(out_file, "w");

	if(fout){
		for(uint i = 0; i < data->l; i++)
			fprintf(fout, "%d\n", (int)data->y[i]);
		fclose(fout);
	}
};

bool TrainSVM(svm_problem *data_set, svm_node *PredLabels, svm_node *CurPredLabels, const svm_parameter *params, uint initial_set_size, 
	uint train_set_size, uint test_set_size, uint portion_size)
{
	//setting up data size
	uint OrigSize = data_set->l;
	data_set->l = train_set_size + test_set_size;

	//shifting input data pointer from test set to train set:
	ShiftDataPointer(data_set, test_set_size);
	data_set->l -= portion_size; //excluding recently downloaded data from training set
		
	//training classifier
	CopyLabels(data_set->y, &PredLabels[test_set_size], initial_set_size, data_set->l - initial_set_size);
	//svm_problem *filtered_data = FilterNoises(data_set, &PredLabels[test_set_size], initial_set_size, data_set->l - initial_set_size, 0.55); //filtering noises
	svm_model *model = svm_train(data_set /*filtered_data*/, params);
	
	//shifting data pointer back to test set
	data_set->l += portion_size; //including recently downloaded data to training set
	ShiftDataPointer(data_set, -test_set_size);

	//predicting new labels and saving them to file
	PredictLabels(model, data_set, CurPredLabels);
	
	//killing resources
	svm_free_and_destroy_model(&model);
	//delete [] filtered_data->x;
	//delete [] filtered_data->y;
	//delete filtered_data;

	//restoring data size
	data_set->l = OrigSize;

	return true; //all is ok
};

int ReadLabelAndProb(FILE *fin, int *label, float *prob){
	int result = fscanf(fin, "%d %*f %f\n", label, prob);
	*prob = *prob > 0.5 ? *prob : 1 - *prob;
	
	return result;
};

bool ReLabel(svm_node *PredLabels, svm_node *PredLabels_a, svm_node *PredLabels_b, uint train_set_size, uint test_set_size, uint initial_set_size){
	int label_a, label_b, label = 0;
	float prob_a, prob_b, prob = 0;
	uint WasChanged = 0;
	
	//relabeling...
	for(uint i = 0; i < test_set_size + train_set_size; i++){	
		prob = 0;

		//memory
		
		if(i >= test_set_size){
			prob = PredLabels[i].value;
			label = PredLabels[i].index;
		}
			
		//labels
		label_a = PredLabels_a[i].index;
		label_b = PredLabels_b[i].index;
		prob_a = PredLabels_a[i].value;
		prob_b = PredLabels_b[i].value;

		//maximum probablity	
		
		if(!(prob > prob_a && prob > prob_b)){
			if(prob_a > prob_b){
				PredLabels[i].index = label_a;
				PredLabels[i].value = prob_a;
			}else{
				PredLabels[i].index = label_b;
				PredLabels[i].value = prob_b;
			}
		}

		if(label && PredLabels[i].index != label)
			WasChanged++;
		
		//average weight for co-training
		/*
		int label_n = (label*prob + label_a*prob_a + label_b*prob_b)/(prob + prob_a + prob_b) > 0.0 ? 1 : -1;
		double prob_n = (prob + prob_a + prob_b)/ (prob == 0.0) ? 2 : 3;
			
		if(prob_n > prob){
			PredLabels[i].index = label_n;
			PredLabels[i].value = prob_n;
		}*/

		//average weight for self-training
		/*
		int label_n = (label*prob + label_a*prob_a)/(prob + prob_a) > 0.0 ? 1 : -1;
		double prob_n = (prob + prob_a)/2;
			
		if(prob_n > prob){
			PredLabels[i].index = label_n;
			PredLabels[i].value = prob_n;
		}*/
	}

	return WasChanged/(train_set_size + test_set_size) > 0.0;
};

double CalcAccuracy(double *OrigLabels, svm_node *PredLabels, uint test_set_size, const char *res_file, bool print){
	FILE *fout = fopen(res_file, "a");
	double acc;
	uint Np = 0, Nn = 0, FN = 0, FP = 0, TP = 0, TN = 0;

	uint i = 0;
	for(uint i = 0; i < test_set_size; i++){
		if(OrigLabels[i] == 1){
			Np++;
			if(PredLabels[i].index == 1)
				TP++;
			else
				FN++;
		}else{
			Nn++;
			if(PredLabels[i].index == -1)
				TN++;
			else
				FP++;
		}
	}

	acc = (double)(TP + TN)/(Np + Nn);

	if(fout && print)
		fprintf(fout, "Accuracy = %.2f%%\nPrecision = %.2f%%\nRecall = %.2f%%\n", 
			(float)acc*100, (float)TP/(TP + FP)*100, (float)TP/(TP + FN)*100);

	if(fout)
		std::fclose(fout);

	return acc;
};

void SpareData(const char *in_file, const char *out_file_a, const char *out_file_b){
	svm_problem data;
	svm_node *items = read_problem(in_file, &data, 0);

	FILE *fouta = fopen(out_file_a, "w");
	FILE *foutb = fopen(out_file_b, "w");

	if(fouta && foutb && items){
		svm_node *np = 0;
		for(uint i = 0; i < data.l; i++){
			//printing label
			fprintf(fouta, "%d", (int)data.y[i]);
			fprintf(foutb, "%d", (int)data.y[i]);

			//writing data
			np = data.x[i];
			uint index = 0;
			while(np->index != -1){
				if(np->index < 5)
					fprintf(fouta, " %d:%g", np->index, np->value);
				else
					fprintf(foutb, " %d:%g", np->index - 4, np->value);
				np++;
			};

			fprintf(fouta, "\n");
			fprintf(foutb, "\n");
		};
	};

	if(fouta)
		fclose(fouta);
	if(foutb)
		fclose(foutb);
	if(items){
		free(items);
		free(data.x);
		free(data.y);
	}

};

void fcopy(const char *old_file, const char *new_file){
	FILE *fold = fopen(old_file, "r");
	FILE *fnew = fopen(new_file, "w");

	if(fold && fnew){
		char *buff = Malloc(char, 1024);
		while(fread(buff, sizeof(char), 1024, fold))
			fwrite(buff, sizeof(char), 1024, fnew);
	}

	if(fold)
		fclose(fold);
	if(fnew)
		fclose(fnew);
};

void CoTrain(svm_problem *data_a, svm_problem *data_b, double *OrigLabels, svm_node *PredLabels, svm_parameter *params, 
	uint initial_set_size, uint train_set_size, uint test_set_size, uint portion_size)
{
	//creating intermediate labels
	svm_node 
		*PredLabels_a = new svm_node[train_set_size + test_set_size], 
		*PredLabels_b = new svm_node[train_set_size + test_set_size];

	//training...
	do{
		//training classifier A with train set
		TrainSVM(data_a, PredLabels, PredLabels_a, params, initial_set_size, train_set_size, test_set_size, portion_size);

		//training classifier B with train set
		TrainSVM(data_b, PredLabels, PredLabels_b, params, initial_set_size, train_set_size, test_set_size, portion_size);

		//re-labeling data with most confident labels (exit when no labels were changed)
	}while(ReLabel(PredLabels, PredLabels_a, PredLabels_b, train_set_size, test_set_size, initial_set_size));

	//killing resources
	delete [] PredLabels_a;
	delete [] PredLabels_b;
};

int _tmain(int argc, _TCHAR* argv[])
{
	//reading input args
	if(argc < 5){
		std::printf("Not enough arguments. Wants: co-train in_file_a in_file_b initial_set_size test_set_size portion_size\n"
			"Note that in-files should be like: || test_set |    train_set    ||\n");
		return 0;
	}

	char in_file_a[300] = "", in_file_b[300] = "";
	uint initial_set_size = 100, test_set_size = 100, portion_size = 100;

	wcstombs(in_file_a, argv[1], 300);
	wcstombs(in_file_b, argv[2], 300);
	swscanf(argv[3], L"%d", &initial_set_size);
	swscanf(argv[4], L"%d", &test_set_size);
	swscanf(argv[5], L"%d", &portion_size);

	//killing previous out files
	fclose(fopen(OutFile, "w"));
	unlink(PredFile);

	//creating svm params
	svm_parameter params;
	SetParamValues(&params);

	//reading data
	svm_problem data_a, data_b;
	svm_node *items_a = read_problem(in_file_a, &data_a, 0);
	svm_node *items_b = read_problem(in_file_b, &data_b, 0);

	//storing true labels
	double *OrigLabels = Malloc(double, data_a.l);
	memcpy(OrigLabels, data_a.y, data_a.l*sizeof(double));

	//creating predicted labels
	svm_node *PredLabels = Malloc(svm_node, data_a.l);
	for(uint i = 0; i < data_a.l; i++)
		PredLabels[i].value = 0;

	//co-training...
	uint train_set_size = initial_set_size;
	uint initial_portion_size = portion_size;
	
	do{
		//"downloading" new data portion
		//portion_size = train_set_size;
		train_set_size += portion_size;

		//doing co-train
		CoTrain(&data_a, &data_b, OrigLabels, PredLabels, &params, initial_set_size, train_set_size, test_set_size, portion_size);

		//fixing predicted labels
		initial_set_size = train_set_size - portion_size;
		
		//calculating accuracy and writing it to out file
		CalcAccuracy(OrigLabels, PredLabels, test_set_size, OutFile, true); 
	}while(train_set_size + portion_size + test_set_size <= data_a.l);

	//killing data
	delete [] OrigLabels;
	delete [] PredLabels;

	return 0;
}

