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
	 unsigned char * TextMessage; // offset 9 to 38	
	 unsigned char Checksum;// offset 39
}Record;

typedef struct Log	{
	unsigned char NumberOfRecords[2];
	unsigned short Reserved;
	Record * Record;
	unsigned char Checksum;
}Log;

unsigned char chksum8(void * data, unsigned short sz)
{   
	unsigned char *ptr = (unsigned char *)data;		
	unsigned short sum=0;	
	
	for(int i=0; i< sz; i++)
	{		
		sum += ptr[i];
		
		if(sum > 255)
		{		
			unsigned char temp = (unsigned char)sum;			
			unsigned short carry = sum >> 8;						
			sum = temp + carry;			
		}
	}
	
	unsigned char checksum = (unsigned char) ~sum;	
	unsigned char t = checksum + (unsigned char) sum;
	
	if(t == 0xff)
	{
		if(sz == sizeof(Record))
		{
			Record *rec = (Record*)data;
			rec->Checksum = (unsigned char)checksum;
			printf("found valid checksum for record %hx\n", t);
		}
		else if(sz == sizeof(Log))
		{
			Log * logfile = (Log*)data;
			logfile->Checksum = checksum;
			printf("found valid checksum for logfile %hx\n", t);
		}
		else{
			printf("invalid sz %i\n", sz);
		}
	}
	else{
		printf("invalid checksum %hx\n", t);
	}

	return checksum;
}

Record CreateLogRecord(int auxFlag, char* textMessage, int numRecords)
{
	Record record;			
	time_t epochY2k = 946684800;
	time_t unixEpochTime;	
	time(&unixEpochTime);
	
	long secondsElapsedSinceY2k = difftime(unixEpochTime , epochY2k);	
	
	unsigned short SequenceNumber = numRecords + 1;	
	unsigned char offset0 = SequenceNumber >> 8;
	unsigned char offset1 = SequenceNumber & 0xff;		

	record.SequenceNumber[0] = offset0;	
	record.SequenceNumber[1] = offset1;	
	
	unsigned short AuxDstFlag=0;
		
	if(auxFlag == 1)
	{		
		AuxDstFlag |= 1 << 7;
	}
	else
	{
		AuxDstFlag &= ~(1 << 7);
	}

	if(localtime(&unixEpochTime)->tm_isdst == 1)
	{
		AuxDstFlag |= 1 << 6;
	}
	else
	{
		AuxDstFlag  &= ~(1 << 6);
	}
		
	record.AuxDstFlag = AuxDstFlag;
	
	record.Reserved = 0;
	record.TextMessage = malloc(sizeof(char)*29);
	
	char *t = malloc(sizeof(char)*29);
	sprintf(t, "%-29s",textMessage);
	strcpy(record.TextMessage, t);
	free(t);

	//timestamp checksum	
	unsigned char offset3To6[4];
	offset3To6[0] = (secondsElapsedSinceY2k >> 24) & 0xFF;
	offset3To6[1] = (secondsElapsedSinceY2k >> 16) & 0xFF;
	offset3To6[2] = (secondsElapsedSinceY2k >> 8) & 0xFF;
	offset3To6[3] =	secondsElapsedSinceY2k & 0xFF;
	
	memcpy(record.TimeStamp , offset3To6, sizeof(record.TimeStamp) );


	unsigned char checksum = 0;
	checksum = chksum8(&record, sizeof(record));
	
	return record;	
}



int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("please provide argument -f LogFile -t Text_Message");
		return 0;
	}
	
	int auxFlag = 0;
	int logFileFlag=0;
	int textMessageFlag=0;
	
	char * logFileName;
	char * textMessage = malloc(sizeof(char)*29);
	
	for(int i=0; i< argc; i++)
	{
		if (strcmp(argv[i], "-a") == 0)
		{
			auxFlag = 1;			
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			logFileName =argv[i+1];			
			logFileFlag=1;			
			i++;
		}
		else if (strcmp(argv[i], "-t") == 0)
		{
			if(strlen(argv[i+1]) > 29)
			{
				printf("-t TextMessage supports 29 characters only.\n");
				return -1;
			}
			
			memcpy(textMessage, argv[i+1], sizeof(char)*29);			
			printf("memcpy %s\n", textMessage);
			textMessageFlag=1;
			i++;
		}
	}
	if(logFileFlag == 0 || textMessageFlag == 0)
	{
		printf("please provide argument -f LogFile -t Text_Message");
		return 0;
	}
	
	Log LogFile;		
	FILE * fp;
	unsigned short numRecords=0;
	
	if((fp = fopen(logFileName, "rb+"))!=NULL)
	{
		// file exists		
		numRecords = fgetc(fp) << 8 | fgetc(fp);
		printf("File exists, opening for reading\n\r");
		printf("Number of records found %i\n\r",numRecords);
	}
	else
	{
		printf("file does not exists, open file for writing.. \n\r");

		fp = fopen(logFileName, "wb+");		
		if(fp==NULL)
		{
			printf("Unable to open file for writing %s\n\r ", logFileName);
			return 0;
		}
	}
	
	Record record = CreateLogRecord(auxFlag, textMessage,numRecords);
	
	memcpy(LogFile.NumberOfRecords, record.SequenceNumber, 2);
	int newCount = record.SequenceNumber[0] << 8 | record.SequenceNumber[1];
	
	LogFile.Record = (Record*)malloc(sizeof(Record)*newCount);	
	
	LogFile.Record[newCount-1] = record;
	
	LogFile.Reserved = 0;
	LogFile.Checksum = 0;
	
	// update number of records on offset 0 to 1
	fseek(fp,0,SEEK_SET);
	fputc(LogFile.NumberOfRecords[0],fp);
	fputc(LogFile.NumberOfRecords[1],fp);		
	
	// reserved
	fputc(0, fp);
	fputc(0, fp);
	
	//Append New Record
	if(numRecords > 0 )
	{
		fseek(fp,-1,SEEK_END);
	}
	
	fputc(record.SequenceNumber[0],fp);
	fputc(record.SequenceNumber[1],fp);
	
	fputc(record.AuxDstFlag,fp);
	
	fputc(record.TimeStamp[0],fp);
	fputc(record.TimeStamp[1],fp);
	fputc(record.TimeStamp[2],fp);
	fputc(record.TimeStamp[3],fp);
	
	fputc(0, fp);
	fputc(0, fp);
	
	for(int i=0; i< 30; i++)
	{
		fputc(record.TextMessage[i], fp);
	}
	
	fputc(record.Checksum,fp);
	
	chksum8(&LogFile, sizeof(Log));
	//Update Checksum
	fputc(LogFile.Checksum, fp);	
	
	free(LogFile.Record);		
	fclose(fp);	

	printf("New Record Appended to LogFile.\n");
    return 0;
}