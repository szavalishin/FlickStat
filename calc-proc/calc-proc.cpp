// calc-proc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc < 3){
		printf("Not enough arguments. Wants: calc-proc.exe test_file result_file\n");
	}else{
		FILE *fin, *fout;
		fin = _wfopen(argv[1], L"r");
		fout = _wfopen(argv[2], L"r");

		if(fin && fout){
			unsigned int 
				Np = 0,
				Nn = 0,
				FN = 0,
				FP = 0;

			int 
				valin = 0,
				valout = 0;

			fscanf(fout, "%*[^\n]s"); //skipping first line

			while(fscanf(fin, "%d %*[^\n]s", &valin) != EOF && fscanf(fout, "%d %*[^\n]s", &valout) != EOF){
				if(valin == 1){
					Np++;
					if(valout != 1)
						FP++;
				}else{
					Nn++;
					if(valout != -1)
						FN++;
				}
			}

			fclose(fout);
			fclose(fin);

			unsigned int TP = Np - FN;
			printf("Accuracy = %.2f%%\nPrecision = %.2f%%\nRecall = %.2f%%\n\nPress any key...\n", 
				(1.0 - (float)(FP + FN)/(Np + Nn))*100, (float)TP/(TP + FP)*100, (float)TP/(TP + FN)*100);
		}else
			printf("Wrong file names\n");
	}

	_getch();

	return 0;
}

