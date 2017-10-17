#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <time.h>
#pragma pack(1)

typedef struct Record{
	 unsigned char SequenceNumber[2];// offset 0 to 1 2
	 unsigned char AuxDstFlag; //set bit 7 for Aux, bit 6 for DST 2
	 unsigned char TimeStamp[4]; //offset 3 to 6  4
	 unsigned short Reserved; //offset 7 to 8 2
	 unsigned char TextMessage[29]; // offset 9 to 38	
	 unsigned char Checksum;// offset 39
}Record;

typedef struct Log	{
	unsigned char NumberOfRecords[2];
	unsigned short Reserved;
	Record * Record;
	unsigned char Checksum;
}Log;


void PrintTimeStamp(char * secondsElapsedSinceY2k)
{
	long secondsElapsed= (secondsElapsedSinceY2k[0] << 24) | (secondsElapsedSinceY2k[1] << 16) | 
	(secondsElapsedSinceY2k[2] << 8) | secondsElapsedSinceY2k[3];
		
	time_t epochY2k = 946684800; // Unix timestamp January 1, 2000
	time_t timestamp = epochY2k + secondsElapsed;
	printf("TimeStamp: %s\n", asctime(gmtime(&timestamp)));
}

void PrintRecord(Record record)
{
	int aux = 0; 
	int dst = 0;
	
	aux = (record.AuxDstFlag & 128)!=0; // check if bit 7 for aux is set
	dst = (record.AuxDstFlag & 64)!=0;  // check if bit 6 for dst is set

	printf("SequenceNumber: %i\n\r", record.SequenceNumber[0] << 8 | record.SequenceNumber[1] );
	printf("Aux : %i	DST : %i \n\r", aux, dst );
	printf("TextMessage: %s\n", record.TextMessage);
	PrintTimeStamp(record.TimeStamp);	
}
 
int main(int argc, char *argv[])
{
	if(argc < 1)
	{
		printf("please provide argument -f LogFile");
		return 0;
	}
	
	int logFileFlag=0;
	char * logFileName;
	for(int i=0; i< argc; i++)
	{
		if (strcmp(argv[i], "-f") == 0)
		{
			logFileName =argv[i+1];			
			logFileFlag=1;			
			i++;
		}
	}
	
	if(logFileFlag == 0)
	{
		printf("please provide argument -f LogFile ");
		return 0;
	}
	
	FILE * fp;
	unsigned short numRecords=0;
	
	if((fp = fopen(logFileName, "rb"))!=NULL)
	{
		numRecords = fgetc(fp) << 8 | fgetc(fp);
	}
	
	if(fp == NULL)
	{
		printf("Unable to open file for reading.");
		return -1;
	}
	//skip reserved bytes 
	fgetc(fp);
	fgetc(fp);
	
	printf("Number of records found : %i\n\n\r", numRecords);	

	Record record;
	
	while (fread (&record, sizeof(struct Record)+1, 1, fp))
	{
		PrintRecord(record);
	}
	return 0;
}