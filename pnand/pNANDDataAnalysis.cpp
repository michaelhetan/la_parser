#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"

char optstring[] = "shv?i:o:t:";
bool SimpleFlag = false;
unsigned char CurCMD = 0;

void PrintHelpMsg(void);
void PrintVer(void);
void ParseCmd(unsigned char);
void PrintAddr(FILE *fp);
void PrintDataIn(FILE *fp);
void PrintDataOut(FILE *fp);
void PrintCmdList(FILE *fp, int *pcmdList, int cmdCount);

typedef struct {
	int nSNANDCMD_RDID;
	int nSNANDCMD_RESET;
	int nSNANDCMD_GETF;
	int nSNANDCMD_SETF;
	int nSNANDCMD_WREN;
	int nSNANDCMD_WRDIS;
	int nSNANDCMD_BLKE;
	int nSNANDCMD_RD;
	int nSNANDCMD_RDCACHE;
	int nSNANDCMD_RDCACHEX2;
	int nSNANDCMD_RDCACHEX4;
	int nSNANDCMD_PROGRAMEXE;
	int nSNANDCMD_PROGRAMLOAD;
	int nSNANDCMD_PROGRAMLOADRAND;
} CMDCOUNT;

struct {
	unsigned char AddrLen;
	unsigned char AddrArray[16];
	unsigned int DataInLen;
	unsigned char DataInArray[10000];
	unsigned int DataOutLen;
	unsigned char DataOutArray[10000];
	CMDCOUNT nCmdCount;
} AnalysisStatus;

int main(int argc, char** argv)
{
	char *inFileName = NULL, *outFileName = NULL;
	FILE *inFilePtr, *outFilePtr;
	char *tempStr = NULL;

	char line[256] = {};
	const int maxLineLen = 256;

	unsigned int CE = 0, ALE = 0, CLE = 0, RE = 0, WE = 0, RB = 0 , IO = 0, WP = 0;
	unsigned char RE_EDGE = 0, WE_EDGE = 0;
	unsigned preRE = 0, preWE = 0;
	unsigned char command = 0;
	unsigned long addr = 0;
	unsigned char data[1024]; // 1024 is enough for data payload
	char timeStr[64];
	unsigned int cmdCount = 0;
	int cmdList[256] = {0};

	int opt = 0;

	while (1){
		opt = getopt(argc, argv, optstring);
		if (opt == -1)
			break;
		switch (opt){
		case 's':
			SimpleFlag = true;
			break;
		case 'i':
			inFileName = optarg;
			break;
		case 'o':
			outFileName = optarg;
			break;
		case 'v':
			PrintVer();
			break;
		case 'h':
		case '?':
			PrintHelpMsg();
			break;

		default:
			break;
		}
	}

	if (optind == 1){
		PrintHelpMsg();
		return 1;
	}

	if ((inFileName != NULL) && (outFileName != NULL)){
		inFilePtr = fopen(inFileName, "r");
		outFilePtr = fopen(outFileName, "w");
		if ((inFilePtr == NULL) || (outFilePtr == NULL))
		{
			printf("input/output File Error\n");
			PrintHelpMsg();
			//C:\All\LA tool code\PNANDDataAnalysis_sleep(1000);
			return 1;
		}
	}
	else
		return 1;

	fgets(line, maxLineLen, inFilePtr);
	printf("the input file format is: %s", line);

	while (!feof(inFilePtr))
	{
		fgets(line, maxLineLen, inFilePtr);
//		sscanf(line, "%[^,],%d,%d,%d,%d,%d,%d,%x", timeStr, &CE, &ALE, &CLE, &RE, &WE, &RB, &IO);
		sscanf(line, "%[^,],%d,%d,%d,%d,%d,%d, %d,%x", timeStr,&CE, &ALE, &CLE, &RE, &WE, &RB, &WP, &IO);
		if ((preRE == 0) && (RE == 1))
			RE_EDGE = 1;
		else
			RE_EDGE = 0;

		if ((preWE == 0) && (WE == 1))
			WE_EDGE = 1;
		else
			WE_EDGE = 0;

		preRE = RE;
		preWE = WE;

		if (CE == 0)
		{
			if ((ALE == 0) && (CLE == 1) && (WE_EDGE == 1))
			{
				cmdCount++;
				PrintAddr(outFilePtr);
				PrintDataIn(outFilePtr);
				PrintDataOut(outFilePtr);
	
				command = IO;
				CurCMD = command;
				ParseCmd(command);
				fprintf(outFilePtr, "TS: %s\n", timeStr);
				fprintf(outFilePtr, "CMD: %x\n", command);
				
				cmdList[command]++;
				AnalysisStatus.AddrLen = 0;
				AnalysisStatus.DataInLen = 0;
				AnalysisStatus.DataOutLen = 0;
			}
			else if ((ALE == 0) && (CLE == 0) && (WE_EDGE == 1))
			{
				AnalysisStatus.DataInArray[AnalysisStatus.DataInLen] = IO;
				AnalysisStatus.DataInLen++;
			}
			else if ((ALE == 0) && (CLE == 0) && (RE_EDGE == 1))
			{
				AnalysisStatus.DataOutArray[AnalysisStatus.DataOutLen] = IO;
				AnalysisStatus.DataOutLen++;
			}
			else if ((ALE == 1) && (CLE == 0) && (WE_EDGE == 1))
			{
				AnalysisStatus.AddrArray[AnalysisStatus.AddrLen] = IO;
				AnalysisStatus.AddrLen++;
			}
		}
	}
	PrintAddr(outFilePtr);
	PrintDataIn(outFilePtr);
	PrintDataOut(outFilePtr);
	PrintCmdList(outFilePtr, cmdList, cmdCount);
	fclose(outFilePtr);
	fclose(inFilePtr);
}

void ParseCmd(unsigned char command)
{
	switch (command)
	{
		case 0xFF:
			break;

		case 0x90:
			break;
		defalut:
			break;
	}
}

void PrintHelpMsg(){
	printf("--------------------------------------------------------------------------------\n");
	printf("Micron PNAND LA DATA ANALYSIS TOOL\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("This Tool is used to analysis LA data.\n");
	printf("Usage Mode: pNANDDataAnalysis.exe -v -h -i <input file> -o <output file>\n");
	printf("    -v    print version information\n");
	printf("    -h    print current help information\n");
	printf("    -i    input LA data\n");
	printf("    -o    output result file\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("Input File format:\n");
	printf("    <time>,<CE>,<ALE>,<CLE>,<RE#>,<WE#>,<R/B#>,<IO>\n");
	printf("\n");
}

void PrintVer(){
	printf("--------------------------------------------------------------------------------\n");
	printf("Micron PNAND LA DATA ANALYSIS TOOL\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("Author: Blair Pan   blairpan@micron.com\n");
	printf("Version: V0.1\n");
	printf("Last Update: 2014-09-16\n");
}

void PrintAddr(FILE *fp){
	unsigned char i = 0;
	if (AnalysisStatus.AddrLen != 0)
	{
		fprintf(fp, "ADDR: ");
		for (i = 0; i != AnalysisStatus.AddrLen; i++)
		{
			fprintf(fp, "%3x", AnalysisStatus.AddrArray[i]);
		}
		fprintf(fp, "\n");
	}
}

void PrintDataIn(FILE *fp){
	unsigned int i = 0;
	if (AnalysisStatus.DataInLen != 0)
	{
		fprintf(fp, "Data In:\n");
		for (i = 0; i != AnalysisStatus.DataInLen; i++)
		{
			fprintf(fp, "%2x", AnalysisStatus.DataInArray[i]);
			if ((i + 1) % 16 == 0)
				fprintf(fp, "\n");
			else
				fprintf(fp, " ");
		}
		if (i % 16 != 0)
			fprintf(fp, "\n");
	}
}

void PrintDataOut(FILE *fp){
	unsigned int i = 0;
	if (AnalysisStatus.DataOutLen != 0)
	{
		fprintf(fp, "Data Out:\n");
		if (SimpleFlag == true && ((CurCMD == 0x30) || (CurCMD == 0x31) || (CurCMD == 0x3F)))
		{
			fprintf(fp, "DataOutput Length is %d\n", AnalysisStatus.DataOutLen);
			return;
		}
		
		for (i = 0; i != AnalysisStatus.DataOutLen; i++)
		{
			fprintf(fp, "%2x", AnalysisStatus.DataOutArray[i]);
			if ((i + 1) % 16 == 0)
				fprintf(fp, "\n");
			else
				fprintf(fp, " ");
		}
		if (i % 16 != 0)
			fprintf(fp, "\n");
		
		fprintf(fp, "DataOutput Length is %d\n", AnalysisStatus.DataOutLen);
	}
}

void PrintCmdList(FILE *fp, int *pcmdList, int cmdCount){
	unsigned int i = 0;
	if (cmdCount != 0)
	{
		fprintf(fp, "Total Command Count: %d\n", cmdCount);
		for (i = 0; i != 256; i++)
		{
			if (pcmdList[i] != 0)
				fprintf(fp, "Command %2xH count: %d\n", i, pcmdList[i]);
		}
	}
}

