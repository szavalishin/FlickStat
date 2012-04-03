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

//set fout = 0 to use reading only
void WriteLineToFile(FILE *fin, FILE *fout){
	char tmp = 'x';
	while(tmp != '\n' && fread(&tmp, sizeof(char), 1, fin)){
		if(fout)
			fwrite(&tmp, sizeof(char), 1, fout);
	}
};

bool CreateTrainingSet(const char* learning_file, const char* TrainFile, unsigned int train_set_size){
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
		int class_val = 0;

		//reading input file...
		while(fscanf(fin, "%d", &class_val) != EOF){
			//adding data to train file...
			if((class_val == 1 && PosCount < MaxCount) || (class_val == -1 && NegCount < MaxCount)){
				fprintf(ftrain, "%d", class_val);
				WriteLineToFile(fin, ftrain);
				class_val == 1 ? PosCount++ : NegCount++;
			}else
				WriteLineToFile(fin, 0);

			//storing labels
			fprintf(fold, "%d 0\n", class_val*-1);
		}

		fclose(fin);
		fclose(ftrain);
		fclose(fold);
	}else
		return false;

	return true;
};

unsigned int UpdateTrainingSet(char* learning_file, char* TrainFile, char* PredFile, float threshold, unsigned int train_set_size){
	unsigned int Upd = 0;
	
	FILE* fin = fopen(learning_file, "r");
	FILE* ftrain = fopen(TrainFile, "w");
	FILE* fpred = fopen(PredFile, "r");
	FILE* fold = fopen(OldFileName, "r");
	FILE* fnew = fopen(NewFileName, "w");

	//checking files
	if(fin && ftrain && fpred && fold && fnew){
		int class_in = 0,
			class_out = 0,
			class_old = 0;
		float prec = 0, 
			  oldprec = 0;

		fscanf(fpred, "%*[^\n]s"); //skipping first line

		//reading input file and pred file...
		unsigned int 
			PosCount = 0, 
			NegCount = 0, 
			MaxCount = train_set_size/2;

		while(fscanf(fin, "%d", &class_in) != EOF && 
			fscanf(fpred, "%d %*f %f", &class_out, &prec) != EOF &&
			fscanf(fold, "%d %f", &class_old, &oldprec) != EOF)
		{
			prec = prec > 0.5 ? prec : 1 - prec;

			//writing new labels to file
			fprintf(fnew, "%d %f\n", class_out, prec);

			//checking if any labels was changed
			if(class_out != class_old)
				Upd++;

			//adding learning set to training set
			if((class_in == 1 && PosCount < MaxCount) || (class_in == -1 && NegCount < MaxCount))
			{
				fprintf(ftrain, "%d", class_in);
				WriteLineToFile(fin, ftrain);
				class_in == 1 ? PosCount++ : NegCount++;
			}
			else{ //updating training set with predicted labels
				if(prec < oldprec){
					prec = oldprec;
					class_out = class_old;
				}

				if(prec > threshold){
					fprintf(ftrain, "%d", class_out);
					WriteLineToFile(fin, ftrain);
				}else
					WriteLineToFile(fin, 0);
			}
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
	if(argc < 4){
		printf("Wrong arguments. Wants: self-train train_set_size threshold learning_file [testing_file] [use_existing_model]\n");
		return 0;
	}

	//reading arguments
	unsigned int train_set_size = 100;
	char 
		learning_file[300] = "",
		testing_file[300] = "0";
	float threshold = 0.5;
	bool UseExistingModel = false;
	
	if(argc > 5) 
		if(argv[5][0] == '1') 
			UseExistingModel = true;

	wcstombs(learning_file, argv[3], 150);

	if(argc > 4)
		wcstombs(testing_file, argv[4], 150);

	swscanf(argv[1], L"%d", &train_set_size);
	swscanf(argv[2], L"%f", &threshold);

	//creating training set from learning_file
	if(CreateTrainingSet(learning_file, TrainFileName, train_set_size)){
		//self-training...
		unsigned int UpdCount = train_set_size;
		do{ 
			//training SVM
			if(!UseExistingModel){
				system(((string)"svm-train -t 4 -b 1 " + TrainFileName + " " + ModelFileName).c_str());
				//UseExistingModel = false;
			}

			//predicting labels
			system(((string)"svm-predict -b 1 " + learning_file + " " + ModelFileName + " " + PredFileName).c_str());
			
			//updating training set
			UpdCount = UpdateTrainingSet(learning_file, TrainFileName, PredFileName, threshold, train_set_size);
		}while(UpdCount);

		//calculating self-trained SVM accuracy 
		if(testing_file[0] != '0'){
			system(((string)"svm-predict -b 1 " + testing_file + " " + ModelFileName + " " + PredFileName).c_str());
			system(((string)"calc-proc " + testing_file + " " + PredFileName).c_str());
		}
	}else{
		printf("Wrong file names\n");
	};

	//unlink(TrainFileName);
	unlink(PredFileName);
	unlink(OldFileName);
	unlink(NewFileName);

	return 0;
}

