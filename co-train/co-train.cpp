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
			old_labels[i] = new_labels[i].index;
};

void PredictLabels(const char *pred_file, const svm_model *model, const svm_problem *data_set){
	FILE *fout = fopen(pred_file, "w");

	double predict_label;
	uint nr_class = svm_get_nr_class(model);
	double *prob_estimates = (double *) malloc(nr_class*sizeof(double));

	for(uint i = 0; i < data_set->l; i++){
		predict_label = svm_predict_probability(model, data_set->x[i], prob_estimates);

		fprintf(fout, "%g", predict_label);
		for(uint j = 0; j < nr_class; j++)
			fprintf(fout, " %g", prob_estimates[j]);
		fprintf(fout,"\n");
	};

	free(prob_estimates);
	fclose(fout);
};

void PrintLabels(const char *out_file, const svm_problem *data){
	FILE *fout = fopen(out_file, "w");

	if(fout){
		for(uint i = 0; i < data->l; i++)
			fprintf(fout, "%d\n", (int)data->y[i]);
		fclose(fout);
	}
};

bool TrainSVM(const char *in_file, const char *prob_file, const char *pred_file, const svm_parameter *params, 
	uint initial_set_size, uint train_set_size, uint test_set_size, uint portion_size)
{
	//loading data set
	svm_problem data_set;
	svm_node *items = read_problem(in_file, &data_set, train_set_size + test_set_size);

	if(data_set.x && data_set.y && items){
		//shifting input data pointer from test set to train set:
		ShiftDataPointer(&data_set, test_set_size);
		data_set.l -= portion_size; //excluding recently downloaded data from training set
		
		//training classifier
		svm_node *labels = LoadPredictedLabels(prob_file, test_set_size, data_set.l);
		CopyLabels(data_set.y, labels, initial_set_size, data_set.l - initial_set_size);
		svm_model *model = svm_train(&data_set, params);
		
		//shifting data pointer back to test set
		data_set.l += portion_size; //including recently downloaded data to training set
		ShiftDataPointer(&data_set, -test_set_size);

		//predicting new labels and saving them to file
		PredictLabels(pred_file, model, &data_set);

		//killing resources
		free(data_set.x);
		free(data_set.y);
		free(items);
		if(labels)
			free(labels);
		svm_free_and_destroy_model(&model);
	}else{
		if(data_set.x) 
			free(data_set.x);
		if(data_set.y) 
			free(data_set.y);
		if(items){ 
			free(items);
			free(data_set.x);
			free(data_set.y);
		}

		return false; //error occured
	}

	return true;
};

int ReadLabelAndProb(FILE *fin, int *label, float *prob){
	int result = fscanf(fin, "%d %*f %f\n", label, prob);
	*prob = *prob > 0.5 ? *prob : 1 - *prob;
	
	return result;
};

void ReLabel(const char *pred_file, const char *pred_file_a, const char *pred_file_b, uint test_set_size, uint initial_set_size){
	FILE *fin = fopen(pred_file, "r");
	FILE *fin_a = fopen(pred_file_a, "r");
	FILE *fin_b = fopen(pred_file_b, "r");
	FILE *fout = fopen("tmp.txt", "w");

	if(fin_a && fin_b){
		int label_a, label_b, label = 0;
		float prob_a, prob_b, prob = 0;
		uint i = 0;

		while(ReadLabelAndProb(fin_a, &label_a, &prob_a) != EOF && 
			ReadLabelAndProb(fin_b, &label_b, &prob_b)  != EOF)
		{
			prob = 0;

			//memory
			
			if(fin)
				fscanf(fin, "%d %f\n", &label, &prob);
			if(!fin || i++ < test_set_size)
				prob = 0;
			

			//maximum probablity
			
			if(fin && prob > prob_a && prob > prob_b)
				fprintf(fout, "%d %f\n", label, prob);
			else 
				if(prob_a > prob_b)
					fprintf(fout, "%d %f\n", label_a, prob_a);
				else
					fprintf(fout, "%d %f\n", label_b, prob_b);
			

			//average weight
			/*
			int label_n = (label*prob + label_a*prob_a + label_b*prob_b)/(prob + prob_a + prob_b) > 0.0 ? 1 : -1;
			float prob_n = (prob + prob_a + prob_b)/3;
			
			if(prob_n > prob)
				fprintf(fout, "%d %f\n", label_n, prob_n);
			else
				fprintf(fout, "%d %f\n", label, prob);
			*/
		}
	}

	if(fin){
		fclose(fin);
		unlink(pred_file);
	}

	if(fout){
		fclose(fout);
		rename("tmp.txt", pred_file);
	}

	if(fin_a)
		fclose(fin_a);
	if(fin_b)
		fclose(fin_b);
};

double CalcAccuracy(const char *file_in, const char *file_out, uint test_set_size, const char *res_file, bool print){
	FILE *fin = fopen(file_in, "r");
	FILE *fout = fopen(file_out, "r");
	FILE *fres = fopen(res_file, "a");

	double acc;

	if(fin && fout){
		uint Np = 0, Nn = 0, FN = 0, FP = 0, TP = 0, TN = 0;

		int valin = 0, valout = 0;
		uint i = 0;
		while(fscanf(fin, "%d %*[^\n]s", &valin) != EOF && fscanf(fout, "%d %*[^\n]s", &valout) != EOF &&
			i++ < test_set_size)
		{
			if(valin == 1){
				Np++;
				if(valout == 1)
					TP++;
				else
					FN++;
			}else{
				Nn++;
				if(valout == -1)
					TN++;
				else
					FP++;
			}
		}

		acc = (double)(TP + TN)/(Np + Nn);

		if(print)
			fprintf(fres, "Accuracy = %.2f%%\nPrecision = %.2f%%\nRecall = %.2f%%\n", 
				(float)acc*100, (float)TP/(TP + FP)*100, (float)TP/(TP + FN)*100);
	};

	if(fres)
		std::fclose(fres);
	if(fin)
		std::fclose(fin);
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

int _tmain(int argc, _TCHAR* argv[])
{
	//reading input args
	if(argc < 5){
		printf("Not enough arguments. Wants: co-train in_file_a in_file_b initial_set_size test_set_size portion_size\n"
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
	
	//co-training...
	uint train_set_size = initial_set_size;
	uint initial_portion_size = portion_size;
	bool IsError = false;

	//reading data
	svm_problem data_a, data_b;
	//svm_node *items_a = read_problem(in_file_a, &data_a, 0);
	//svm_node *items_b = read_problem(in_file_b, &data_b, 0);

	do{
		//"downloading" new data portion
		train_set_size += portion_size;

		//training classifier A with train set
		IsError = !TrainSVM(in_file_a, PredFile, PredFileA, &params, initial_set_size, train_set_size, test_set_size, portion_size);

		//training classifier B with train set
		IsError = !TrainSVM(in_file_b, PredFile, PredFileB, &params, initial_set_size, train_set_size, test_set_size, portion_size);

		//re-labeling data with most confident labels
		ReLabel(PredFile, PredFileA, PredFileB, test_set_size, initial_set_size);
		
		//calculating accuracy and writing it to out file
		CalcAccuracy(in_file_a, PredFile, test_set_size, OutFile, true); 
	}while(!IsError);

	//killing files
	unlink(PredFile);
	unlink(PredFileA);
	unlink(PredFileB);

	return 0;
}

