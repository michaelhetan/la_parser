#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#include "N25Q.h"
#include "snand.h"

const unsigned char defNormDummy = 8;
const unsigned char defQuadDummy = 10;
const unsigned char defDTRDummy = 6;
const unsigned char defQuadDTRDummy = 8;
const unsigned char CMDLEN = 8;

char optstring[] = "shv?i:o:t:";
bool SimpleFlag = false;

void PrintHelpMsg(void);
void PrintVer(void);
void initAnalysisStatus(void);
bool isThereAddr(unsigned char command);
bool isReadCommand(unsigned char command);
bool ParseSNORCmd(FILE *fp, unsigned char command);
bool ParseSNANDCmd(FILE *fp, unsigned char command);
bool PrintAddr(FILE *fp, unsigned long address);
bool PrintData(FILE *fp, unsigned char *dPtr, unsigned int length);
void PrintCmdCount(FILE *fp);

typedef enum{
	DUMMY_0 = 0,
	DUMMY_1,
	DUMMY_2,
	DUMMY_3,
	DUMMY_4,
	DUMMY_5,
	DUMMY_6,
	DUMMY_7,
	DUMMY_8,
	DUMMY_9,
	DUMMY_10,
	DUMMY_11,
	DUMMY_12,
	DUMMY_13,
	DUMMY_14,
}DummySetting;

typedef enum{
	NORMADDRMODE = 1,
	EXTENDADDRMODE,
}AddressMode;

typedef enum{
	SPI_111_MODE = 1,
	SPI_112_MODE,
	SPI_122_MODE,
	SPI_222_MODE,
	SPI_114_MODE,
	SPI_144_MODE,
	SPI_444_MODE,
}SPI_IO_INTERFACE;

typedef enum{
	SPI_EXTENDED_MODE = 1,
	SPI_DUAL_MODE,
	SPI_QUAD_MODE,
} SPI_IO_MODE;

typedef enum{
	SPI_READ_OPERATION = 1,
	SPI_WRITE_OPERATION,
} SPI_IO_OPERATION;

typedef enum{
	TYPE_SPI_NOR = 1,
	TYPE_SPI_NAND,
} PROTOCAL_TYPE;

typedef struct {
	int nCMD_RDID1;
	int nCMD_RDID2;
	int nCMD_MULTIIORDID;
	int nCMD_RDPARAM;
	int nCMD_READ;
	int nCMD_FASTRD;
	int nCMD_DORD;
	int nCMD_DIORD;
	int nCMD_QORD;
	int nCMD_QIORD;
	int nCMD_DTRRD;
	int nCMD_DTRDORD;
	int nCMD_DTRDIORD;
	int nCMD_DTRQORD;
	int nCMD_DTRQIORD;
	int nCMD_READ_4B;
	int nCMD_FASTRD_4B;
	int nCMD_DORD_4B;
	int nCMD_DIORD_4B;
	int nCMD_QORD_4B;
	int nCMD_QIORD_4B;
	int nCMD_WREN;
	int nCMD_WRDIS;
	int nCMD_RDSR;
	int nCMD_WRSR;
	int nCMD_RDLR;
	int nCMD_WRLR;
	int nCMD_RDFSR;
	int nCMD_CLRFSR;
	int nCMD_RDNVCR;
	int nCMD_WRNVCR;
	int nCMD_RDVCR;
	int nCMD_WRVCR;
	int nCMD_RDVECR;
	int nCMD_WRVECR;
	int nCMD_RDEAR;
	int nCMD_WREAR;
	int nCMD_PP;
	int nCMD_PP_4B;
	int nCMD_DIP;
	int nCMD_EDIP;
	int nCMD_QIP;
	int nCMD_QIP_4B;
	int nCMD_EQIP;
	int nCMD_SSE;
	int nCMD_SSE_4B;
	int nCMD_SE;
	int nCMD_SE_4B;
	int nCMD_BE;
	int nCMD_RESUME;
	int nCMD_SUSPEND;
	int nCMD_RDOTP;
	int nCMD_WROTP;
	int nCMD_EN4B;
	int nCMD_EX4B;
	int nCMD_ENQUAD;
	int nCMD_EXQUAD;
	int nCMD_UNREG;

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
	bool bCSFallingEdge;
	bool bCmdInProcessing;
	bool bAddrInProcessing;
	bool bDataInProcessing;

	PROTOCAL_TYPE pType;

	SPI_IO_MODE SPI_MODE;
	SPI_IO_INTERFACE SPIInterface;
	AddressMode SPIAddrMode;
	SPI_IO_OPERATION SPIOperation;

	DummySetting nDummy;
	
	bool bDummyAvailable;
	unsigned char addrLen;
	CMDCOUNT nCmdCount;
} AnalysisStatus;

void main(int argc, char** argv)
{
	char *inFileName = NULL, *outFileName = NULL;
	FILE *inFilePtr, *outFilePtr;
	char *tempStr = NULL;

	char line[256] = {};
	const int maxLineLen = 256;

	unsigned int dq0 = 0;
	unsigned int dq1 = 0;
	unsigned int dq2 = 0;
	unsigned int dq3 = 0;
	unsigned int cs = 0;
	unsigned int clk = 0;
	unsigned char command = 0;
	unsigned long addr = 0;
	unsigned char data[1024]; // 1024 is enough for data payload
	double timeVal = 0;
	char timeStr[64];
	unsigned int cmdCount = 0;
	unsigned char cmdStartNum = 0;
	unsigned char addrStartNum = 0;
	unsigned int dataStartNum = 0;

	bool bLongDataPayload = false;
	unsigned int dataPayload = 0;

	unsigned char bitPos = 0;
	unsigned int dataIndex = 0;
	int i;

	unsigned long nNormReadCount = 0;
	unsigned long nFastReadCount = 0;
	unsigned long nPPCount = 0;

	unsigned char nDummyLen = 0;

	int opt = 0;

	AnalysisStatus.pType = TYPE_SPI_NOR;

	while (1){
		opt = getopt(argc, argv, optstring);
		if (opt == -1)
			break;
		switch (opt){
		case 't':
			tempStr = optarg;
			tempStr = strupr(tempStr);
			if (strcmp(tempStr, "SNOR") == 0){
			}
			else if (strcmp(tempStr, "SNAND") == 0){
				AnalysisStatus.pType = TYPE_SPI_NAND;
			}
			else{
				PrintHelpMsg();
				return;
			}
			break;
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
		return;
	}

	if ((inFileName != NULL) && (outFileName != NULL)){
		inFilePtr = fopen(inFileName, "r");
		outFilePtr = fopen(outFileName, "w");
		if ((inFilePtr == NULL) || (outFilePtr == NULL))
		{
			printf("input/output File Error\n");
			PrintHelpMsg();
			_sleep(1000);
			return;
		}
	}
	else
		return;

	fgets(line, maxLineLen, inFilePtr);
	printf("the input file format is: %s", line);

	for (i = 0; i != 1024; i++){
		data[i] = 0;
	}
	initAnalysisStatus();

	while (!feof(inFilePtr))
	{
		fgets(line, maxLineLen, inFilePtr);
		sscanf(line, "%[^,],%d,%d,%d,%d,%d,%d", timeStr, &clk, &cs, &dq0, &dq1, &dq2, &dq3);

		if (cs == 1){
			AnalysisStatus.bCSFallingEdge = true;
		}
		else if (AnalysisStatus.bCSFallingEdge) {
			AnalysisStatus.bCSFallingEdge = false;
			if ((dataStartNum != 0) || (bLongDataPayload == true))
			{
				PrintData(outFilePtr, data, dataIndex + 1);
				dataPayload += dataIndex;
				if (dataPayload > 2048){
					fprintf(outFilePtr, "payload length: %d bytes\n", dataPayload);
				}
				
				if (command == OPCODE_READ){
					nNormReadCount += dataPayload;
				}
				else if (command == OPCODE_FASTRD){
					nFastReadCount += dataPayload;
				}
				else if (command == OPCODE_PP) {
					nPPCount += dataPayload;
				}
			}
			command = 0;
			cmdStartNum = 0;

			addr = 0;
			addrStartNum = 0;

			dataStartNum = 0;
			nDummyLen = 0;
			for (i = 0; i != 1024; i++){
				data[i] = 0;
			}

			AnalysisStatus.bCmdInProcessing = true;
			AnalysisStatus.bAddrInProcessing = false;
			AnalysisStatus.bDataInProcessing = false;

			bLongDataPayload = false;
			dataPayload = 0;
		}
		else if (AnalysisStatus.bCmdInProcessing){
			if (AnalysisStatus.SPI_MODE == SPI_EXTENDED_MODE){
				command = command | (dq0 << (CMDLEN - cmdStartNum - 1));
				cmdStartNum++;
			}
			else if (AnalysisStatus.SPI_MODE == SPI_DUAL_MODE){
				command = command | (dq1 << (CMDLEN - cmdStartNum - 1)) | (dq0 << (CMDLEN - cmdStartNum - 2));
				cmdStartNum += 2;
			}
			else if (AnalysisStatus.SPI_MODE == SPI_QUAD_MODE){
				command = command | (dq3 << (CMDLEN - cmdStartNum - 1)) | (dq2 << (CMDLEN - cmdStartNum - 2)) | (dq1 << (CMDLEN - cmdStartNum - 3)) | (dq0 << (CMDLEN - cmdStartNum - 4));
				cmdStartNum += 4;
			}

			if (cmdStartNum == CMDLEN){
				fprintf(outFilePtr, "TS: %s\n", timeStr);
				cmdCount++;
				if (AnalysisStatus.pType == TYPE_SPI_NOR){
					ParseSNORCmd(outFilePtr, command);
				}
				else if (AnalysisStatus.pType == TYPE_SPI_NAND) {
					ParseSNANDCmd(outFilePtr, command);
				}
				if (AnalysisStatus.bDummyAvailable){
					nDummyLen = AnalysisStatus.nDummy;
					if (nDummyLen == DUMMY_0){
						if ((AnalysisStatus.SPIInterface == SPI_144_MODE) || (AnalysisStatus.SPIInterface == SPI_444_MODE)){
							nDummyLen = defQuadDummy;
						}
						else
							nDummyLen = defNormDummy;
					}
				}
			}
		}

		else if (AnalysisStatus.bAddrInProcessing){
			bitPos = AnalysisStatus.addrLen - addrStartNum;

			if ((AnalysisStatus.SPIInterface == SPI_122_MODE) || (AnalysisStatus.SPIInterface == SPI_222_MODE)){
				addr = addr | (dq1 << (AnalysisStatus.addrLen - addrStartNum - 1)) | (dq0 << (AnalysisStatus.addrLen - addrStartNum - 2));
				addrStartNum += 2;
			}
			else if ((AnalysisStatus.SPIInterface == SPI_144_MODE) || (AnalysisStatus.SPIInterface == SPI_444_MODE)){
				addr = addr | (dq3 << (AnalysisStatus.addrLen - addrStartNum - 1)) | (dq2 << (AnalysisStatus.addrLen - addrStartNum - 2)) | 
						(dq1 << (AnalysisStatus.addrLen - addrStartNum - 3)) | (dq0 << (AnalysisStatus.addrLen - addrStartNum - 4));
				addrStartNum += 4;
			}
			else {
				addr = addr | (dq0 << (AnalysisStatus.addrLen - addrStartNum - 1));
				addrStartNum++;
			}

			if (addrStartNum == AnalysisStatus.addrLen){
				AnalysisStatus.bAddrInProcessing = false;
				PrintAddr(outFilePtr, addr);
			}
		}

		else if (AnalysisStatus.bDataInProcessing){
			if (AnalysisStatus.bDummyAvailable){
				if (nDummyLen){
					nDummyLen--;
					continue;
				}
			}

			bitPos = sizeof(char)* 8 - dataStartNum % (sizeof(char)* 8);
			dataIndex = dataStartNum / (sizeof(char) * 8);
			if (dataIndex == 1024){
				bLongDataPayload = true;
				PrintData(outFilePtr, data, dataIndex);
				dataIndex -= 1024;
				dataStartNum -= 8192;
				for (i = 0; i != 1024; i++){
					data[i] = 0;
				}
				dataPayload += 1024;
			}
			if (AnalysisStatus.SPIInterface == SPI_111_MODE){
				if (AnalysisStatus.SPIOperation == SPI_READ_OPERATION){
					data[dataIndex] = data[dataIndex] | (dq1 << (bitPos - 1));
				}
				else{
					data[dataIndex] = data[dataIndex] | (dq0 << (bitPos - 1));
				}
				dataStartNum++;
			}
			else if ((AnalysisStatus.SPIInterface == SPI_112_MODE) || (AnalysisStatus.SPIInterface == SPI_122_MODE) ||
					(AnalysisStatus.SPIInterface == SPI_222_MODE)){
				data[dataIndex] = data[dataIndex] | (dq1 << (bitPos - 1)) | (dq0 << (bitPos - 2));
				dataStartNum += 2;
			}
			else if ((AnalysisStatus.SPIInterface == SPI_114_MODE) || (AnalysisStatus.SPIInterface == SPI_144_MODE) ||
					(AnalysisStatus.SPIInterface == SPI_444_MODE)){
				data[dataIndex] = data[dataIndex] | (dq3 << (bitPos - 1)) | (dq2 << (bitPos - 2)) | (dq1 << (bitPos - 3)) | (dq0 << (bitPos - 4));
				dataStartNum += 4;
			}
		}

	}

	if (AnalysisStatus.bAddrInProcessing){
		PrintAddr(outFilePtr, addr);
	}
	if (AnalysisStatus.bDataInProcessing){
			PrintData(outFilePtr, data, dataIndex + 1);
			if (bLongDataPayload){
				fprintf(outFilePtr, "Long data payload!\n");
			}
			dataPayload += dataIndex;
			fprintf(outFilePtr, "data payload length: %d bytes\n", dataPayload);

			if (command == OPCODE_READ){
				nNormReadCount += dataPayload;
			}
			else if (command == OPCODE_FASTRD){
				nFastReadCount += dataPayload;
			}
			else if (command == OPCODE_PP) {
				nPPCount += dataPayload;
			}
	}

	fprintf(outFilePtr, "Search out the input file, analysis complete!\nLast sample at %s\n", timeStr);

	fprintf(outFilePtr, "total count of command is %d \n", cmdCount);
	PrintCmdCount(outFilePtr);
	fprintf(outFilePtr, "total byte count of Norm Read is %d \n", nNormReadCount);
	fprintf(outFilePtr, "total byte count of Fast Read is %d \n", nFastReadCount);
	fprintf(outFilePtr, "total byte count of nPPCount is %d", nPPCount);

	fclose(inFilePtr);
	fclose(outFilePtr);
}

void PrintHelpMsg(){
	printf("--------------------------------------------------------------------------------\n");
	printf("Micron SPI NOR LA DATA ANALYSIS TOOL\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("This Tool is used to analysis LA data.\n");
	printf("Currently only support sync mode with CS both edge and clk rising edge sampling");
	printf("Usage Mode: LADataAnalysis.exe -t -v -h -s -i <input file> -o <output file>\n");
	printf("    -t    select device type SNOR/SNAND\n");
	printf("    -v    print version information\n");
	printf("    -h    print current help information\n");
	printf("    -s    enable simple output file\n");
	printf("    -i    input LA data\n");
	printf("    -o    output result file\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("Input File format:\n");
	printf("    <time>,<clk>,<cs>,<DQ0>,<DQ1>,<DQ2>,<DQ3>\n");
	printf("Output File format:\n");
	printf("    [command]\n");
	printf("    <address>\n");
	printf("    <data>\n");
	printf("\n");
}

void PrintVer(){
	printf("--------------------------------------------------------------------------------\n");
	printf("Micron SPI NOR LA DATA ANALYSIS TOOL\n");
	printf("--------------------------------------------------------------------------------\n");
	printf("Author: Blair Pan   blairpan@micron.com\n");
	printf("Version: V0.4\n");
	printf("Last Update: 2014-02-12\n");
}

void initAnalysisStatus(){
	AnalysisStatus.bCSFallingEdge = false;
	AnalysisStatus.bCmdInProcessing = false;
	AnalysisStatus.bAddrInProcessing = false;
	AnalysisStatus.bDataInProcessing = false;

	AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE;
	AnalysisStatus.SPIInterface = SPI_111_MODE;
	AnalysisStatus.SPIAddrMode = NORMADDRMODE;
	AnalysisStatus.SPIOperation = SPI_READ_OPERATION;

	AnalysisStatus.nDummy = DUMMY_0;

	AnalysisStatus.bDummyAvailable = false;
	AnalysisStatus.addrLen = 24;

	AnalysisStatus.nCmdCount.nCMD_RDID1 = 0;
	AnalysisStatus.nCmdCount.nCMD_RDID2 = 0;
	AnalysisStatus.nCmdCount.nCMD_MULTIIORDID = 0;
	AnalysisStatus.nCmdCount.nCMD_RDPARAM = 0;
	AnalysisStatus.nCmdCount.nCMD_READ = 0;
	AnalysisStatus.nCmdCount.nCMD_FASTRD = 0;
	AnalysisStatus.nCmdCount.nCMD_DORD = 0;
	AnalysisStatus.nCmdCount.nCMD_DIORD = 0;
	AnalysisStatus.nCmdCount.nCMD_QORD = 0;
	AnalysisStatus.nCmdCount.nCMD_QIORD = 0;
	AnalysisStatus.nCmdCount.nCMD_DTRRD = 0;
	AnalysisStatus.nCmdCount.nCMD_DTRDORD = 0;
	AnalysisStatus.nCmdCount.nCMD_DTRDIORD = 0;
	AnalysisStatus.nCmdCount.nCMD_DTRQORD = 0;
	AnalysisStatus.nCmdCount.nCMD_DTRQIORD = 0;
	AnalysisStatus.nCmdCount.nCMD_READ_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_FASTRD_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_DORD_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_DIORD_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_QORD_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_QIORD_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_WREN = 0;
	AnalysisStatus.nCmdCount.nCMD_WRDIS = 0;
	AnalysisStatus.nCmdCount.nCMD_RDSR = 0;
	AnalysisStatus.nCmdCount.nCMD_WRSR = 0;
	AnalysisStatus.nCmdCount.nCMD_RDLR = 0;
	AnalysisStatus.nCmdCount.nCMD_WRLR = 0;
	AnalysisStatus.nCmdCount.nCMD_RDFSR = 0;
	AnalysisStatus.nCmdCount.nCMD_CLRFSR = 0;
	AnalysisStatus.nCmdCount.nCMD_RDNVCR = 0;
	AnalysisStatus.nCmdCount.nCMD_WRNVCR = 0;
	AnalysisStatus.nCmdCount.nCMD_RDVCR = 0;
	AnalysisStatus.nCmdCount.nCMD_WRVCR = 0;
	AnalysisStatus.nCmdCount.nCMD_RDVECR = 0;
	AnalysisStatus.nCmdCount.nCMD_WRVECR = 0;
	AnalysisStatus.nCmdCount.nCMD_RDEAR = 0;
	AnalysisStatus.nCmdCount.nCMD_WREAR = 0;
	AnalysisStatus.nCmdCount.nCMD_PP = 0;
	AnalysisStatus.nCmdCount.nCMD_PP_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_DIP = 0;
	AnalysisStatus.nCmdCount.nCMD_EDIP = 0;
	AnalysisStatus.nCmdCount.nCMD_QIP = 0;
	AnalysisStatus.nCmdCount.nCMD_QIP_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_EQIP = 0;
	AnalysisStatus.nCmdCount.nCMD_SSE = 0;
	AnalysisStatus.nCmdCount.nCMD_SSE_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_SE = 0;
	AnalysisStatus.nCmdCount.nCMD_SE_4B = 0;
	AnalysisStatus.nCmdCount.nCMD_BE = 0;
	AnalysisStatus.nCmdCount.nCMD_RESUME = 0;
	AnalysisStatus.nCmdCount.nCMD_SUSPEND = 0;
	AnalysisStatus.nCmdCount.nCMD_RDOTP = 0;
	AnalysisStatus.nCmdCount.nCMD_WROTP = 0;
	AnalysisStatus.nCmdCount.nCMD_EN4B = 0;
	AnalysisStatus.nCmdCount.nCMD_EX4B = 0;
	AnalysisStatus.nCmdCount.nCMD_ENQUAD = 0;
	AnalysisStatus.nCmdCount.nCMD_EXQUAD = 0;
	AnalysisStatus.nCmdCount.nCMD_UNREG = 0;
}

bool isThereAddr(unsigned char command){
	switch (command)
	{
	case OPCODE_READ:
	case OPCODE_FASTRD:
	case OPCODE_DORD:
	case OPCODE_DIORD:
	case OPCODE_QORD:
	case OPCODE_QIORD:
	case OPCODE_DTRRD:
	case OPCODE_DTRDORD:
	case OPCODE_DTRDIORD:
	case OPCODE_DTRQORD:
	case OPCODE_DTRQIORD:
	case OPCODE_READ_4B:
	case OPCODE_FASTRD_4B:
	case OPCODE_DORD_4B:
	case OPCODE_DIORD_4B:
	case OPCODE_QORD_4B:
	case OPCODE_QIORD_4B:
	case OPCODE_RDLR:
	case OPCODE_WRLR:
	case OPCODE_PP:
	case OPCODE_PP_4B:
	case OPCODE_DIP:
	case OPCODE_EDIP:
	case OPCODE_QIP:
	case OPCODE_QIP_4B:
	case OPCODE_EQIP:
	case OPCODE_SSE:
	case OPCODE_SSE_4B:
	case OPCODE_SE:
	case OPCODE_SE_4B:
	case OPCODE_RDOTP:
	case OPCODE_WROTP:
		return true;

	default:
		return false;
	}
}

bool isReadCommand(unsigned char command){
	switch (command)
	{
	case OPCODE_RDID1:
	case OPCODE_RDID2:
	case OPCODE_MULTIIORDID:
	case OPCODE_RDPARAM:
	case OPCODE_READ:
	case OPCODE_FASTRD:
	case OPCODE_DORD:
	case OPCODE_DIORD:
	case OPCODE_QORD:
	case OPCODE_QIORD:
	case OPCODE_DTRRD:
	case OPCODE_DTRDORD:
	case OPCODE_DTRDIORD:
	case OPCODE_DTRQORD:
	case OPCODE_DTRQIORD:
	case OPCODE_READ_4B:
	case OPCODE_FASTRD_4B:
	case OPCODE_DORD_4B:
	case OPCODE_DIORD_4B:
	case OPCODE_QORD_4B:
	case OPCODE_QIORD_4B:
	case OPCODE_RDSR:
	case OPCODE_RDLR:
	case OPCODE_RDFSR:
	case OPCODE_RDNVCR:
	case OPCODE_RDVCR:
	case OPCODE_RDVECR:
	case OPCODE_RDEAR:
	case OPCODE_RDOTP:
	case OPCODE_WROTP:
		return true;

	default:
		return false;
	}
}

bool ParseSNORCmd(FILE *fp, unsigned char command){

	if (AnalysisStatus.SPI_MODE == SPI_EXTENDED_MODE){
		AnalysisStatus.SPIInterface = SPI_111_MODE;
	}
	else if (AnalysisStatus.SPI_MODE == SPI_DUAL_MODE){
		AnalysisStatus.SPIInterface = SPI_222_MODE;
	}
	else if (AnalysisStatus.SPI_MODE == SPI_QUAD_MODE){
		AnalysisStatus.SPIInterface = SPI_444_MODE;
	}

	if (AnalysisStatus.SPIAddrMode == NORMADDRMODE){
		AnalysisStatus.addrLen = 24;
	}
	else if (AnalysisStatus.SPIAddrMode == EXTENDADDRMODE){
		AnalysisStatus.addrLen = 32;
	}

	AnalysisStatus.bDummyAvailable = false;

	AnalysisStatus.bCmdInProcessing = false;
	AnalysisStatus.bAddrInProcessing = false;
	AnalysisStatus.bDataInProcessing = true;

	AnalysisStatus.SPIOperation = SPI_READ_OPERATION;

	switch (command){

	case OPCODE_RDID1:
		AnalysisStatus.nCmdCount.nCMD_RDID1++;
		if (AnalysisStatus.SPI_MODE != SPI_EXTENDED_MODE)
			fprintf(fp, "Error command: Read ID(0x%x), this cmd only effect in single IO mode\n", command);
		else
			fprintf(fp, "CMD: Read ID1(0x%x)\n", command);
		break;

	case OPCODE_RDID2:
		AnalysisStatus.nCmdCount.nCMD_RDID2++;
		if (AnalysisStatus.SPI_MODE != SPI_EXTENDED_MODE)
			fprintf(fp, "Error command: Read ID(0x%x), this cmd only effect in single IO mode\n", command);
		else
			fprintf(fp, "CMD: Read ID2(0x%x)\n", command);
		break;

	case OPCODE_MULTIIORDID:
		AnalysisStatus.nCmdCount.nCMD_MULTIIORDID++;
		if (AnalysisStatus.SPI_MODE == SPI_EXTENDED_MODE)
			fprintf(fp, "Error command: Read ID(0x%x), this cmd can not effect in single IO mode\n", command);
		else
			fprintf(fp, "CMD: MultiIO Read ID(0x%x)\n", command);
		break;

	case OPCODE_RDPARAM:
		AnalysisStatus.nCmdCount.nCMD_RDPARAM++;
		fprintf(fp, "CMD: Read Flash Parameter(0x%x)\n", command);
		AnalysisStatus.bDummyAvailable = true;
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 24;
		break;

	case OPCODE_READ:
		AnalysisStatus.nCmdCount.nCMD_READ++;
		fprintf(fp, "CMD: Norm Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		break;

	case OPCODE_FASTRD:
		AnalysisStatus.nCmdCount.nCMD_FASTRD++;
		fprintf(fp, "CMD: Fast Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		break;

	case OPCODE_DORD:
		AnalysisStatus.nCmdCount.nCMD_DORD++;
		fprintf(fp, "CMD: Dual Output Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_112_MODE;
		}
		break;

	case OPCODE_DIORD:
		AnalysisStatus.nCmdCount.nCMD_DIORD++;
		fprintf(fp, "CMD: Dual IO Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_122_MODE;
		}
		break;

	case OPCODE_QORD:
		AnalysisStatus.nCmdCount.nCMD_QORD++;
		fprintf(fp, "CMD: Quad Output Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_114_MODE;
		}
		break;

	case OPCODE_QIORD:
		AnalysisStatus.nCmdCount.nCMD_QIORD++;
		fprintf(fp, "CMD: Quad IO Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_144_MODE;
		}
		break;

	case OPCODE_DTRRD:
		AnalysisStatus.nCmdCount.nCMD_DTRRD++;
		fprintf(fp, "CMD: DTR Read(0x%x)\n", command);
		fprintf(fp, "this operation is not supportted currently\n", command);
		break;

	case OPCODE_DTRDORD:
		AnalysisStatus.nCmdCount.nCMD_DTRDORD++;
		fprintf(fp, "CMD: DTR Dual Output Read(0x%x)\n", command);
		fprintf(fp, "this operation is not supportted currently\n", command);
		break;

	case OPCODE_DTRDIORD:
		AnalysisStatus.nCmdCount.nCMD_DTRDIORD++;
		fprintf(fp, "CMD: DTR Dual IO Read(0x%x)\n", command);
		fprintf(fp, "this operation is not supportted currently\n", command);
		break;

	case OPCODE_DTRQORD:
		AnalysisStatus.nCmdCount.nCMD_DTRDORD++;
		fprintf(fp, "CMD: DTR Quad Output Read(0x%x)\n", command);
		fprintf(fp, "this operation is not supportted currently\n", command);
		break;

	case OPCODE_DTRQIORD:
		AnalysisStatus.nCmdCount.nCMD_DTRQIORD++;
		fprintf(fp, "CMD: DTR Quad IO Read(0x%x)\n", command);
		fprintf(fp, "this operation is not supportted currently\n", command);
		break;

	case OPCODE_READ_4B:
		AnalysisStatus.nCmdCount.nCMD_READ_4B++;
		fprintf(fp, "CMD: Norm 4-Byte Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 32;
		break;

	case OPCODE_FASTRD_4B:
		AnalysisStatus.nCmdCount.nCMD_FASTRD_4B++;
		fprintf(fp, "CMD: Fast 4-Byte Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		AnalysisStatus.addrLen = 32;
		break;

	case OPCODE_DORD_4B:
		AnalysisStatus.nCmdCount.nCMD_DORD_4B++;
		fprintf(fp, "CMD: 4-Byte Dual Output Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		AnalysisStatus.addrLen = 32;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_112_MODE;
		}
		break;

	case OPCODE_DIORD_4B:
		AnalysisStatus.nCmdCount.nCMD_DIORD_4B++;
		fprintf(fp, "CMD: 4-Byte Dual IO Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		AnalysisStatus.addrLen = 32;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_122_MODE;
		}
		break;

	case OPCODE_QORD_4B:
		AnalysisStatus.nCmdCount.nCMD_QORD_4B++;
		fprintf(fp, "CMD: 4-Byte Quad Output Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		AnalysisStatus.addrLen = 32;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_114_MODE;
		}
		break;

	case OPCODE_QIORD_4B:
		AnalysisStatus.nCmdCount.nCMD_QIORD_4B++;
		fprintf(fp, "CMD: 4-Byte Quad IO Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		AnalysisStatus.addrLen = 32;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_144_MODE;
		}
		break;

	case OPCODE_WREN:
		AnalysisStatus.nCmdCount.nCMD_WREN++;
		fprintf(fp, "CMD: Write Enable(0x%x)\n", command);
		break;

	case OPCODE_WRDIS:
		AnalysisStatus.nCmdCount.nCMD_WRDIS++;
		fprintf(fp, "CMD: Write Disable(0x%x)\n", command);
		break;

	case OPCODE_RDSR:
		AnalysisStatus.nCmdCount.nCMD_RDSR++;
		fprintf(fp, "CMD: Read Status Register(0x%x)\n", command);
		break;

	case OPCODE_WRSR:
		AnalysisStatus.nCmdCount.nCMD_WRSR++;
		fprintf(fp, "CMD: Write Status Register(0x%x)\n", command);
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_RDLR:
		AnalysisStatus.nCmdCount.nCMD_RDLR++;
		fprintf(fp, "CMD: Read Lock Register(0x%x)\n", command);
		break;

	case OPCODE_WRLR:
		AnalysisStatus.nCmdCount.nCMD_WRLR++;
		fprintf(fp, "CMD: Write Lock Register(0x%x)\n", command);
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_RDFSR:
		AnalysisStatus.nCmdCount.nCMD_RDFSR++;
		fprintf(fp, "CMD: Read Flag Status Register(0x%x)\n", command);
		break;

	case OPCODE_CLRFSR:
		AnalysisStatus.nCmdCount.nCMD_CLRFSR++;
		fprintf(fp, "CMD: Clear Flag Status Register(0x%x)\n", command);
		break;

	case OPCODE_RDNVCR:
		AnalysisStatus.nCmdCount.nCMD_RDNVCR++;
		fprintf(fp, "CMD: Read NVCR(0x%x)\n", command);
		break;

	case OPCODE_WRNVCR:
		AnalysisStatus.nCmdCount.nCMD_WRNVCR++;
		fprintf(fp, "CMD: Write NVCR(0x%x)\n", command);
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_RDVCR:
		AnalysisStatus.nCmdCount.nCMD_RDVCR++;
		fprintf(fp, "CMD: Read VCR(0x%x)\n", command);
		break;

	case OPCODE_WRVCR:
		AnalysisStatus.nCmdCount.nCMD_WRVCR++;
		fprintf(fp, "CMD: Write VCR(0x%x)\n", command);
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_RDVECR:
		AnalysisStatus.nCmdCount.nCMD_RDVECR++;
		fprintf(fp, "CMD: Read VECR(0x%x)\n", command);
		break;

	case OPCODE_WRVECR:
		AnalysisStatus.nCmdCount.nCMD_WRVECR++;
		fprintf(fp, "CMD: Write VECR(0x%x)\n", command);
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_RDEAR:
		AnalysisStatus.nCmdCount.nCMD_RDEAR++;
		fprintf(fp, "CMD: Read Extended Address Register(0x%x)\n", command);
		break;

	case OPCODE_WREAR:
		AnalysisStatus.nCmdCount.nCMD_WREAR++;
		fprintf(fp, "CMD: Write Extended Address Register(0x%x)\n", command);
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_PP:
		AnalysisStatus.nCmdCount.nCMD_PP++;
		fprintf(fp, "CMD: Page Program(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_PP_4B:
		AnalysisStatus.nCmdCount.nCMD_PP_4B++;
		fprintf(fp, "CMD: 4-Byte Page Program(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		AnalysisStatus.addrLen = 32;
		break;

	case OPCODE_DIP:
		AnalysisStatus.nCmdCount.nCMD_DIP++;
		fprintf(fp, "CMD: Dual Page Program(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_112_MODE;
		}
		break;

	case OPCODE_EDIP:
		AnalysisStatus.nCmdCount.nCMD_EDIP++;
		fprintf(fp, "CMD: Extended Dual Page Program(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_122_MODE;
		}
		break;

	case OPCODE_QIP:
		AnalysisStatus.nCmdCount.nCMD_QIP++;
		fprintf(fp, "CMD: Quad Page Program(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_114_MODE;
		}
		break;

	case OPCODE_QIP_4B:
		AnalysisStatus.nCmdCount.nCMD_QIP_4B++;
		fprintf(fp, "CMD: 4-Byte Quad Page Program(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		AnalysisStatus.addrLen = 32;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_114_MODE;
		}
		break;

	case OPCODE_EQIP:
		AnalysisStatus.nCmdCount.nCMD_EQIP++;
		fprintf(fp, "CMD: Extended Quad Page Program(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		if (AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE){
			AnalysisStatus.SPIInterface = SPI_144_MODE;
		}
		break;

	case OPCODE_SSE:
		AnalysisStatus.nCmdCount.nCMD_SSE++;
		fprintf(fp, "CMD: Sub Sector Erase(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		break;

	case OPCODE_SSE_4B:
		AnalysisStatus.nCmdCount.nCMD_SSE_4B++;
		fprintf(fp, "CMD: 4-Byte Sub Sector Erase(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 32;
		break;

	case OPCODE_SE:
		AnalysisStatus.nCmdCount.nCMD_SE++;
		fprintf(fp, "CMD: Sector Erase(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		break;

	case OPCODE_SE_4B:
		AnalysisStatus.nCmdCount.nCMD_SE_4B++;
		fprintf(fp, "CMD: 4-Byte Sector Erase(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 32;
		break;

	case OPCODE_BE:
		AnalysisStatus.nCmdCount.nCMD_BE++;
		fprintf(fp, "CMD: Bulk Erase(0x%x)\n", command);
		break;

	case OPCODE_RESUME:
		AnalysisStatus.nCmdCount.nCMD_RESUME++;
		fprintf(fp, "CMD: Resume(0x%x)\n", command);
		fprintf(fp, "this operation is not supportted currently\n", command);
		break;

	case OPCODE_SUSPEND:
		AnalysisStatus.nCmdCount.nCMD_SUSPEND++;
		fprintf(fp, "CMD: Suspend(0x%x)\n", command);
		fprintf(fp, "this operation is not supportted currently\n", command);
		break;

	case OPCODE_RDOTP:
		AnalysisStatus.nCmdCount.nCMD_RDOTP++;
		fprintf(fp, "CMD: Read OTP(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.bDummyAvailable = true;
		break;

	case OPCODE_WROTP:
		AnalysisStatus.nCmdCount.nCMD_WROTP++;
		fprintf(fp, "CMD: Write OTP(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_EN4B:
		AnalysisStatus.nCmdCount.nCMD_EN4B++;
		fprintf(fp, "CMD: Enter 4-Byte mode(0x%x)\n", command);
		AnalysisStatus.SPIAddrMode = EXTENDADDRMODE;
		break;

	case OPCODE_EX4B:
		AnalysisStatus.nCmdCount.nCMD_EX4B++;
		fprintf(fp, "CMD: Exit 4-Byte mode(0x%x)\n", command);
		AnalysisStatus.SPIAddrMode = NORMADDRMODE;
		break;

	case OPCODE_ENQUAD:
		AnalysisStatus.nCmdCount.nCMD_ENQUAD++;
		fprintf(fp, "CMD: Enter Quad Mode(0x%x)\n", command);
		AnalysisStatus.SPI_MODE = SPI_QUAD_MODE;
		break;

	case OPCODE_EXQUAD:
		AnalysisStatus.nCmdCount.nCMD_EXQUAD++;
		fprintf(fp, "CMD: Exit Quad Mode(0x%x)\n", command);
		AnalysisStatus.SPI_MODE = SPI_EXTENDED_MODE;
		break;

	default:
		AnalysisStatus.nCmdCount.nCMD_UNREG++;
		fprintf(fp, "Unrecognized command(0x%x)\n", command);
		return false;
	}
	return true;
}

bool ParseSNANDCmd(FILE *fp, unsigned char command){
	AnalysisStatus.bDummyAvailable = false;

	AnalysisStatus.bCmdInProcessing = false;
	AnalysisStatus.bAddrInProcessing = false;
	AnalysisStatus.bDataInProcessing = true;

	AnalysisStatus.SPIOperation = SPI_READ_OPERATION;

	switch (command){
	case OPCODE_SNAND_RDID:
		AnalysisStatus.nCmdCount.nSNANDCMD_RDID++;
		fprintf(fp, "CMD: Read ID(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 8;
		break;

	case OPCODE_SNAND_RESET:
		AnalysisStatus.nCmdCount.nSNANDCMD_RESET++;
		fprintf(fp, "CMD: Reset(0x%x)\n", command);
		break;

	case OPCODE_SNAND_SETF:
		AnalysisStatus.nCmdCount.nSNANDCMD_SETF++;
		fprintf(fp, "CMD: Set Feature(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 8;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_SNAND_WRDIS:
		AnalysisStatus.nCmdCount.nSNANDCMD_WRDIS++;
		fprintf(fp, "CMD: Write Disable(0x%x)\n", command);
		break;

	case OPCODE_SNAND_WREN:
		AnalysisStatus.nCmdCount.nSNANDCMD_WREN++;
		fprintf(fp, "CMD: Write Enable(0x%x)\n", command);
		break;

	case OPCODE_SNAND_BLKE:
		AnalysisStatus.nCmdCount.nSNANDCMD_BLKE++;
		fprintf(fp, "CMD: Block Erase(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 24;
		break;

	case OPCODE_SNAND_GETF:
		AnalysisStatus.nCmdCount.nSNANDCMD_GETF++;
		fprintf(fp, "CMD: Get Feature(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 8;
		break;

	case OPCODE_SNAND_RD:
		AnalysisStatus.nCmdCount.nSNANDCMD_RD++;
		fprintf(fp, "CMD: Page Read(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 24;
		break;

	case OPCODE_SNAND_PROGRAMEXE:
		AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMEXE++;
		fprintf(fp, "CMD: Program Execute(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 24;
		break;

	case OPCODE_SNAND_PROGRAMLOAD:
		AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMLOAD++;
		fprintf(fp, "CMD: Program Load(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.SPIOperation = SPI_WRITE_OPERATION;
		break;

	case OPCODE_SNAND_PROGRAMLOADRAND:
		AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMLOADRAND++;
		fprintf(fp, "CMD: Program Load Random(0x%x)\n", command);
		break;

	case OPCODE_SNAND_RDCACHE:
	case OPCODE_SNAND_RDCACHE2:
		AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHE++;
		fprintf(fp, "CMD: Read Cache(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 16;
		break;

	case OPCODE_SNAND_RDCACHEX2:
		AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHEX2++;
		fprintf(fp, "CMD: Read Cache X2(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 16;
		break;

	case OPCODE_SNAND_RDCACHEX4:
		AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHEX4++;
		fprintf(fp, "CMD: Read Cache X4(0x%x)\n", command);
		AnalysisStatus.bAddrInProcessing = true;
		AnalysisStatus.addrLen = 16;
		break;	

	default:
		AnalysisStatus.nCmdCount.nCMD_UNREG++;
		fprintf(fp, "Unrecognized command(0x%x)\n", command);
		return false;
	}
	return true;
}

bool PrintAddr(FILE *fp, unsigned long addr){
	fprintf(fp, "ADDR: 0x%x\n", addr);
	return true;
}

bool PrintData(FILE *fp, unsigned char *dPtr, unsigned int length){
	unsigned int i;
	fprintf(fp, "Data:");
	for (i = 0; i < length; i++)
	{
		if (i % 16 == 0){
			if (SimpleFlag && (i == 16)){
				fprintf(fp, "...");
				break;
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "%2x ", *dPtr);
		dPtr++;
	}
	fprintf(fp, "\n");
	return true;
}

void PrintCmdCount(FILE *fp){
	if (AnalysisStatus.nCmdCount.nCMD_RDID1 != 0)
		fprintf(fp, "RDID1 count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDID1);

	if (AnalysisStatus.nCmdCount.nCMD_RDID2 != 0)
		fprintf(fp, "RDID2 count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDID2);

	if (AnalysisStatus.nCmdCount.nCMD_MULTIIORDID != 0)
		fprintf(fp, "MULTIIORDID count: %d\n", AnalysisStatus.nCmdCount.nCMD_MULTIIORDID);

	if (AnalysisStatus.nCmdCount.nCMD_RDPARAM != 0)
		fprintf(fp, "RDPARAM count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDPARAM);

	if (AnalysisStatus.nCmdCount.nCMD_READ != 0)
		fprintf(fp, "READ count: %d\n", AnalysisStatus.nCmdCount.nCMD_READ);

	if (AnalysisStatus.nCmdCount.nCMD_FASTRD != 0)
		fprintf(fp, "FASTRD count: %d\n", AnalysisStatus.nCmdCount.nCMD_FASTRD);

	if (AnalysisStatus.nCmdCount.nCMD_DORD != 0)
		fprintf(fp, "DORD count: %d\n", AnalysisStatus.nCmdCount.nCMD_DORD);

	if (AnalysisStatus.nCmdCount.nCMD_DIORD != 0)
		fprintf(fp, "DIORD count: %d\n", AnalysisStatus.nCmdCount.nCMD_DIORD);

	if (AnalysisStatus.nCmdCount.nCMD_QORD != 0)
		fprintf(fp, "QORD count: %d\n", AnalysisStatus.nCmdCount.nCMD_QORD);

	if (AnalysisStatus.nCmdCount.nCMD_QIORD != 0)
		fprintf(fp, "QIORD count: %d\n", AnalysisStatus.nCmdCount.nCMD_QIORD);

	if (AnalysisStatus.nCmdCount.nCMD_DTRRD != 0)
		fprintf(fp, "DTRRD count: %d\n", AnalysisStatus.nCmdCount.nCMD_DTRRD);

	if (AnalysisStatus.nCmdCount.nCMD_DTRDORD != 0)
		fprintf(fp, "DTRDORD count: %d\n", AnalysisStatus.nCmdCount.nCMD_DTRDORD);

	if (AnalysisStatus.nCmdCount.nCMD_DTRDIORD != 0)
		fprintf(fp, "DTRDIORD count: %d\n", AnalysisStatus.nCmdCount.nCMD_DTRDIORD);

	if (AnalysisStatus.nCmdCount.nCMD_DTRQORD != 0)
		fprintf(fp, "DTRQORD count: %d\n", AnalysisStatus.nCmdCount.nCMD_DTRQORD);

	if (AnalysisStatus.nCmdCount.nCMD_DTRQIORD != 0)
		fprintf(fp, "DTRQIORD count: %d\n", AnalysisStatus.nCmdCount.nCMD_DTRQIORD);

	if (AnalysisStatus.nCmdCount.nCMD_READ_4B != 0)
		fprintf(fp, "READ_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_READ_4B);

	if (AnalysisStatus.nCmdCount.nCMD_FASTRD_4B != 0)
		fprintf(fp, "FASTRD_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_FASTRD_4B);

	if (AnalysisStatus.nCmdCount.nCMD_DORD_4B != 0)
		fprintf(fp, "DORD_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_DORD_4B);

	if (AnalysisStatus.nCmdCount.nCMD_DIORD_4B != 0)
		fprintf(fp, "DIORD_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_DIORD_4B);

	if (AnalysisStatus.nCmdCount.nCMD_QORD_4B != 0)
		fprintf(fp, "QORD_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_QORD_4B);

	if (AnalysisStatus.nCmdCount.nCMD_QIORD_4B != 0)
		fprintf(fp, "QIORD_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_QIORD_4B);

	if (AnalysisStatus.nCmdCount.nCMD_WREN != 0)
		fprintf(fp, "WREN count: %d\n", AnalysisStatus.nCmdCount.nCMD_WREN);

	if (AnalysisStatus.nCmdCount.nCMD_WRDIS != 0)
		fprintf(fp, "WRDIS count: %d\n", AnalysisStatus.nCmdCount.nCMD_WRDIS);

	if (AnalysisStatus.nCmdCount.nCMD_RDSR != 0)
		fprintf(fp, "RDSR count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDSR);

	if (AnalysisStatus.nCmdCount.nCMD_WRSR != 0)
		fprintf(fp, "WRSR count: %d\n", AnalysisStatus.nCmdCount.nCMD_WRSR);

	if (AnalysisStatus.nCmdCount.nCMD_RDLR != 0)
		fprintf(fp, "RDLR count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDLR);

	if (AnalysisStatus.nCmdCount.nCMD_WRLR != 0)
		fprintf(fp, "WRLR count: %d\n", AnalysisStatus.nCmdCount.nCMD_WRLR);

	if (AnalysisStatus.nCmdCount.nCMD_RDFSR != 0)
		fprintf(fp, "RDFSR count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDFSR);

	if (AnalysisStatus.nCmdCount.nCMD_CLRFSR != 0)
		fprintf(fp, "CLRFSR count: %d\n", AnalysisStatus.nCmdCount.nCMD_CLRFSR);

	if (AnalysisStatus.nCmdCount.nCMD_RDNVCR != 0)
		fprintf(fp, "RDNVCR count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDNVCR);

	if (AnalysisStatus.nCmdCount.nCMD_WRNVCR != 0)
		fprintf(fp, "WRNVCR count: %d\n", AnalysisStatus.nCmdCount.nCMD_WRNVCR);

	if (AnalysisStatus.nCmdCount.nCMD_RDVCR != 0)
		fprintf(fp, "RDVCR count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDVCR);

	if (AnalysisStatus.nCmdCount.nCMD_WRVCR != 0)
		fprintf(fp, "WRVCR count: %d\n", AnalysisStatus.nCmdCount.nCMD_WRVCR);

	if (AnalysisStatus.nCmdCount.nCMD_RDVECR != 0)
		fprintf(fp, "RDVECR count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDVECR);

	if (AnalysisStatus.nCmdCount.nCMD_WRVECR != 0)
		fprintf(fp, "WRVECR count: %d\n", AnalysisStatus.nCmdCount.nCMD_WRVECR);

	if (AnalysisStatus.nCmdCount.nCMD_RDEAR != 0)
		fprintf(fp, "RDEAR count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDEAR);

	if (AnalysisStatus.nCmdCount.nCMD_WREAR != 0)
		fprintf(fp, "WREAR count: %d\n", AnalysisStatus.nCmdCount.nCMD_WREAR);

	if (AnalysisStatus.nCmdCount.nCMD_PP != 0)
		fprintf(fp, "PP count: %d\n", AnalysisStatus.nCmdCount.nCMD_PP);

	if (AnalysisStatus.nCmdCount.nCMD_PP_4B != 0)
		fprintf(fp, "PP_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_PP_4B);

	if (AnalysisStatus.nCmdCount.nCMD_DIP != 0)
		fprintf(fp, "DIP count: %d\n", AnalysisStatus.nCmdCount.nCMD_DIP);

	if (AnalysisStatus.nCmdCount.nCMD_EDIP != 0)
		fprintf(fp, "EDIP count: %d\n", AnalysisStatus.nCmdCount.nCMD_EDIP);

	if (AnalysisStatus.nCmdCount.nCMD_QIP != 0)
		fprintf(fp, "QIP count: %d\n", AnalysisStatus.nCmdCount.nCMD_QIP);

	if (AnalysisStatus.nCmdCount.nCMD_QIP_4B != 0)
		fprintf(fp, "QIP_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_QIP_4B);

	if (AnalysisStatus.nCmdCount.nCMD_EQIP != 0)
		fprintf(fp, "EQIP count: %d\n", AnalysisStatus.nCmdCount.nCMD_EQIP);

	if (AnalysisStatus.nCmdCount.nCMD_SSE != 0)
		fprintf(fp, "SSE count: %d\n", AnalysisStatus.nCmdCount.nCMD_SSE);

	if (AnalysisStatus.nCmdCount.nCMD_SSE_4B != 0)
		fprintf(fp, "SSE_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_SSE_4B);

	if (AnalysisStatus.nCmdCount.nCMD_SE != 0)
		fprintf(fp, "SE count: %d\n", AnalysisStatus.nCmdCount.nCMD_SE);

	if (AnalysisStatus.nCmdCount.nCMD_SE_4B != 0)
		fprintf(fp, "SE_4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_SE_4B);

	if (AnalysisStatus.nCmdCount.nCMD_BE != 0)
		fprintf(fp, "BE count: %d\n", AnalysisStatus.nCmdCount.nCMD_BE);

	if (AnalysisStatus.nCmdCount.nCMD_RESUME != 0)
		fprintf(fp, "RESUME count: %d\n", AnalysisStatus.nCmdCount.nCMD_RESUME);

	if (AnalysisStatus.nCmdCount.nCMD_SUSPEND != 0)
		fprintf(fp, "SUSPEND count: %d\n", AnalysisStatus.nCmdCount.nCMD_SUSPEND);

	if (AnalysisStatus.nCmdCount.nCMD_RDOTP != 0)
		fprintf(fp, "RDOTP count: %d\n", AnalysisStatus.nCmdCount.nCMD_RDOTP);

	if (AnalysisStatus.nCmdCount.nCMD_WROTP != 0)
		fprintf(fp, "WROTP count: %d\n", AnalysisStatus.nCmdCount.nCMD_WROTP);

	if (AnalysisStatus.nCmdCount.nCMD_EN4B != 0)
		fprintf(fp, "EN4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_EN4B);

	if (AnalysisStatus.nCmdCount.nCMD_EX4B != 0)
		fprintf(fp, "EX4B count: %d\n", AnalysisStatus.nCmdCount.nCMD_EX4B);

	if (AnalysisStatus.nCmdCount.nCMD_ENQUAD != 0)
		fprintf(fp, "ENQUAD count: %d\n", AnalysisStatus.nCmdCount.nCMD_ENQUAD);

	if (AnalysisStatus.nCmdCount.nCMD_EXQUAD != 0)
		fprintf(fp, "EXQUAD count: %d\n", AnalysisStatus.nCmdCount.nCMD_EXQUAD);



	if (AnalysisStatus.nCmdCount.nSNANDCMD_BLKE != 0)
		fprintf(fp, "BLKE count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_BLKE);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_GETF != 0)
		fprintf(fp, "Get Feature count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_GETF);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMEXE != 0)
		fprintf(fp, "Program Execute count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMEXE);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMLOAD != 0)
		fprintf(fp, "Program Load count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMLOAD);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMLOADRAND != 0)
		fprintf(fp, "Program Load Random count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_PROGRAMLOADRAND);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_RD != 0)
		fprintf(fp, "Page Read count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_RD);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHE != 0)
		fprintf(fp, "Read Cache count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHE);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHEX2 != 0)
		fprintf(fp, "Read Cache X2 count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHEX2);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHEX4 != 0)
		fprintf(fp, "Read Cache X4 count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_RDCACHEX4);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_RDID != 0)
		fprintf(fp, "Read ID count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_RDID);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_RESET != 0)
		fprintf(fp, "Reset count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_RESET);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_SETF != 0)
		fprintf(fp, "Set Feature count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_SETF);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_WRDIS != 0)
		fprintf(fp, "Write Disable count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_WRDIS);

	if (AnalysisStatus.nCmdCount.nSNANDCMD_WREN != 0)
		fprintf(fp, "Write Enable count: %d\n", AnalysisStatus.nCmdCount.nSNANDCMD_WREN);

	if (AnalysisStatus.nCmdCount.nCMD_UNREG != 0)
		fprintf(fp, "unrecognized comand count: %d\n", AnalysisStatus.nCmdCount.nCMD_UNREG);
}



















































