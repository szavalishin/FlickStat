// self-train.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <conio.h>

#define TrainFileName "train.txt"
#define ModelFileName "model.txt"
#define PredFileName "pred.txt"
#define OldFileName "old.txt"
#define NewFileName "new.txt"

using namespace std;

bool CreateTrainingSet(char* learning_file, char* TrainFile, unsigned int train_set_size){
	//opening files
	FILE* fin = fopen(learning_file, "r");
	FILE* ftrain = fopen(TrainFile, "w");
	FILE* fold = fopen(OldFileName, "w");

	//checking files...
	if(fin && ftrain && fold){
		unsigned int 
			PosCount = 0, 
			NegCount = 0, 
			MaxCount = train_set_size/2;
		char str[1000];
		int class_val = 0;

		//reading input file...
		while(fscanf(fin, "%d %[^\n]s", &class_val, str) != EOF){
			//adding data to train file...
			if((class_val == 1 && PosCount < MaxCount) || (class_val == -1 && NegCount < MaxCount)){
				fprintf(ftrain, "%d %s\n", class_val, str);
				class_val == 1 ? PosCount++ : NegCount++;
			}

			//storing labels
			fprintf(fold, "%d\n", class_val);
		}

		fclose(fin);
		fclose(ftrain);
		fclose(fold);
	}else
		return false;

	return true;
};

unsigned int UpdateTrainingSet(char* learning_file, char* TrainFile, char* PredFile, float threashold_value){
	unsigned int Upd = 0;
	
	FILE* fin = fopen(learning_file, "r");
	FILE* ftrain = fopen(TrainFile, "w");
	FILE* fpred = fopen(PredFile, "r");
	FILE* fold = fopen(OldFileName, "r");
	FILE* fnew = fopen(NewFileName, "w");

	//checking files
	if(fin && ftrain && fpred && fold && fnew){
		char str[1000];
		int class_in = 0,
			class_out = 0,
			class_old = 0;
		float prec = 0;

		fscanf(fpred, "%*[^\n]s"); //skipping first line

		//reading input file and pred file...
		while(fscanf(fin, "%d %[^\n]s", &class_in, str) != EOF && 
			fscanf(fpred, "%d %*f %f", &class_out, &prec) != EOF &&
			fscanf(fold, "%d", &class_old) != EOF){
				//if precision is big enough then adding data to training set
				//if(class_out == class_in && (class_in == 1 ? prec : 1 - prec) > threashold_value)
				if((prec > 0.5 ? prec : 1 - prec) > threashold_value)
					fprintf(ftrain, "%d %s\n", class_out, str);

				//checking if any labels was changed
				if(class_out != class_old)
					Upd++;

				//writing new labels to file
				fprintf(fnew, "%d\n", class_out, str);
		}

		fclose(fin);
		fclose(ftrain);
		fclose(fpred);
		fclose(fold);
		fclose(fnew);

		//changing old saved labels with new ones
		unlink(OldFileName);
		rename(NewFileName, OldFileName);
	}

	return Upd;
};

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc < 5){
		printf("Wrong arguments. Wants: self-train.exe train_set_size threshold_value learning_file testing_file\n");
		return 0;
	}

	//reading arguments
	unsigned int train_set_size = 100;
	float threshold_value = 0.5;
	char 
		learning_file[300] = "",
		testing_file[300] = "";

	wcstombs(learning_file, argv[3], 150);
	wcstombs(testing_file, argv[4], 150);
	swscanf(argv[1], L"%d", &train_set_size);
	swscanf(argv[2], L"%f", &threshold_value);

	//creating training set from learning_file
	if(CreateTrainingSet(learning_file, TrainFileName, train_set_size)){
		//self-training...
		unsigned int UpdCount = train_set_size;
		do{ 
			//training SVM
			system(((string)"svm-train -g 0.5 -b 1 " + TrainFileName + " " + ModelFileName).c_str());

			//predicting labels
			system(((string)"svm-predict -b 1 " + learning_file + " " + ModelFileName + " " + PredFileName).c_str());

			//updating training set
			UpdCount = UpdateTrainingSet(learning_file, TrainFileName, PredFileName, threshold_value);
		}while(UpdCount);

		//calculating self-trained SVM accuracy 
		system(((string)"svm-predict -b 1 " + testing_file + " " + ModelFileName + " " + PredFileName).c_str());
		system(((string)"calc-proc " + testing_file + " " + PredFileName).c_str());
	}else{
		printf("Wrong file names\n");
	};

	//unlink(TrainFileName);
	unlink(PredFileName);
	unlink(OldFileName);
	unlink(NewFileName);

	return 0;
}

