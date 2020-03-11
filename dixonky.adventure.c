/*******************************************************************************
* ** Author: Kyle Dixon
* ** Date: 4/30/2019-
* ** Descriptions: Adventure Program (Program 2)
* ** 	
* ****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>		//for opendir
#include <unistd.h> 	//for getpid()
#include <sys/stat.h>	//for mkdir(), stat()
#include <sys/types.h>	//for mkdir(), stat()
#include <pthread.h>	//for threads

//Game Constant Constraints		
	//Use constants to change game mechanics easily
#define CHAR_BUFFER 256
#define PERMISSIONS 0770
#define TOTAL_ROOMS 10
#define SELECTED_ROOMS 7
#define MAX_CONNECTIONS 6
#define MIN_CONNECTIONS 3
#define MAX_STEPS 1000


//GLOBAL GAME VARIABLES

//Enum room type
	//Room types (start, mid, end)
enum RoomType {START_ROOM, MID_ROOM, END_ROOM};


//Room Struct
	//Contains: string name, Room Type, int connections count, Room ptrs, 
struct ROOM {
	char name[CHAR_BUFFER];
	enum RoomType type;
	int connCount;
	struct ROOM* connArray[MAX_CONNECTIONS];	
};


//Game Files, change with each play
struct ROOM gameArray[SELECTED_ROOMS];
char activeDirectory[CHAR_BUFFER];
pthread_mutex_t ptrMutex;
char * timeFileName = "currentTime.txt";


//Set Directory Function
	//saves the most recent subdirectory into the active directory char[]
	//adapted from https://stackoverflow.com/questions/3554120/open-directory-using-c
void SetDirectory()
{
	char* name = "dixonky.rooms";							
	struct dirent * stream;
	stream = malloc(sizeof(struct dirent));
	DIR * directory;										//dir to search for subdir holding room files
	char tempDirectory[CHAR_BUFFER];						//name holder for subdir 
	memset(activeDirectory, '\0', sizeof(activeDirectory));	//clear the active Directory name holder
	memset(tempDirectory, '\0', sizeof(tempDirectory));		//clear and set up the temp directory
    getcwd(tempDirectory, sizeof(tempDirectory));
    directory = opendir(tempDirectory);
	struct stat * buffer;									//create stat files to look for most recent subfolder
	buffer = malloc(sizeof(struct stat));
	time_t temp;
	time_t recent=0;										//holds the most recent subfolder
	
	if (directory != NULL) 
	{
        while (stream = readdir(directory)) 			//read through the subdirectories
		{	
            if (strstr(stream->d_name,name) != NULL)	//if the pointed to directory contains the string partial of name
			{
                stat(stream->d_name, buffer);			//set up the buffer and get the modification time of the directory
                temp = buffer->st_mtime;

                if(temp > recent) 						//check the current mod time against the saved most recent mod time
				{
                    recent = temp;						//if more recent than save the directory name into activeDirectory
                    strcpy(activeDirectory, stream->d_name);
                }
            }
        }
    }  
    //printf("%s\n", activeDirectory);
    closedir(directory);
}


//Find Room Function
	//returns in the location (index) of the passed in room name
	//failure to find the room returns -1
int FindRoom(char * name)
{
	int i;
	for (i=0; i<SELECTED_ROOMS; i++) 				//loop through the room files in the active directory
	{
		if (strcmp(gameArray[i].name, name) == 0) 
		{
        	return i;
        }	
	}
	return -1;
}


//Find Start Function
	//returns in the location (index) of the start room
	//failure to find the start returns -1
int FindStart()
{
	int i;
	for (i=0; i<SELECTED_ROOMS; i++) 				//loop through the room files in the active directory
	{
		if (gameArray[i].type == START_ROOM) 
		{
			//printf("%d\n", i);
        	return i;
        }	
	}
	return -1;
}


//Connect Function
	//adds a connection between the two passed in room indeces and increments room connection count
void Connect(int x, int y)
{
	gameArray[x].connArray[gameArray[x].connCount] = &gameArray[y];
	gameArray[x].connCount++;
}


//Set Game Array Function
	//sets up the game array to hold room files
void SetGameArray()
{
	int i, j;
	for (i=0; i<SELECTED_ROOMS; i++) 
	{
		memset(gameArray[i].name, '\0', sizeof(gameArray[i].name));
        gameArray[i].connCount = 0;	
        for (j=0; j<MAX_CONNECTIONS; j++)
		{
        	gameArray[i].connArray[j] = NULL;
        }
    }
}


//Get Game Array Function
	//fills the game array with the file names (room names)
	//adapted from https://stackoverflow.com/questions/3554120/open-directory-using-c
void GetGameArray()
{
	int i = 0;
    struct dirent * stream;
    DIR * directoryPtr;

    if ((directoryPtr = opendir(activeDirectory)) != NULL) 
	{
        while ((stream = readdir(directoryPtr)) != NULL) 
		{
            if (strlen(stream->d_name) > 2) 
			{
                strcpy(gameArray[i].name, stream->d_name);
                i++;
            }
        }
    }
}


//Clean Data Function
	//get the file lines ready to be compared
void CleanData(char *name, char *data)
{
    int i;
    strtok(name, ":");					//split name by the colon to differentiate from data
    strcpy(data, strtok(NULL, ""));		//add second part of split to data
    data[strlen(data)-1] = '\0';		//add ending characters
    name[strlen(name)-1] = '\0';
    
    for(i = 0; i < strlen(data); i++) 	//convert data array by shifting
	{
        data[i] = data[i + 1];
    }
}


//Fill Game Array Function
	//adds the connections and the type to the game array
	//game array already contains room file names
void FillGameArray()
{
	int i;
	char fileName[CHAR_BUFFER];						//set up name and data holders
	char fileData[CHAR_BUFFER];
	chdir(activeDirectory);							//change to the active directory to get room files
	FILE * filePtr;
	
	for (i=0; i<SELECTED_ROOMS; i++) 				//loop through the room files in the active directory
	{
		filePtr = fopen(gameArray[i].name,"r");		//open the file based on the name attribute in the game array
		memset(fileName, '\0', sizeof(fileName));	//clear holders
        memset(fileData, '\0', sizeof(fileData));
		
		while(fgets(fileName, sizeof(fileName),filePtr) != NULL) 	//loop through the lines of the openned room file
		{
			CleanData(fileName, fileData);							//takes the full line and splits approriately into fileName and FileData
			//printf("%s\n", fileName);
			//printf("%s\n", fileData);
			if(strcmp(fileName, "ROOM TYP") == 0) 		//if the next line in the file relates to the room type (E is removed because the letters following connection were removed)
			{
                if(strcmp(fileData, "MID_ROOM") == 0)	//set the game array room type according to the matched string
				{
                    gameArray[i].type = MID_ROOM;
                }
                else if(strcmp(fileData, "END_ROOM") == 0) 
				{
                    gameArray[i].type = END_ROOM;
                }
                else
				{
                    gameArray[i].type = START_ROOM;
                }
            }
            else if(strcmp(fileName,"CONNECTION ") == 0) //if the next line in the file relates to the connections (trailing letters removed from comparison)
			{
        		int roomIndex = FindRoom(fileData);
        		Connect(i,roomIndex);
            }
		}
		fclose(filePtr);
	}
	chdir("..");	//go back to the parent directory (from the room subdirectory)
}


//Time Function
	//opens the time file and prints result to screen
void Time()
{
    char timeResult[CHAR_BUFFER];						//holder for time data
    memset(timeResult, '\0', sizeof(timeResult));		//clear holder
    FILE * timeFile;									//open the file that holds the time data
    timeFile = fopen(timeFileName, "r");
    while(fgets(timeResult, CHAR_BUFFER, timeFile) != NULL) 
	{
        printf("%s\n", timeResult);						//read the time data to the console
    }
    fclose(timeFile);									//close the file that holds the time data
}


//Get Time File Ptr Function
	//open the time file and print converted time to the console
void * GetTimeFile()
{
    char timeDisplay[CHAR_BUFFER];						//holder for time data
    memset(timeDisplay, '\0', sizeof(timeDisplay));		//clear holder
    time_t currentTime;									//create time variables
    struct tm *timeData;
    FILE * timeFile;					
    time(&currentTime);									//get and convert the current time
    timeData = localtime(&currentTime);					
    strftime(timeDisplay, CHAR_BUFFER, "%I:%M%p, %A, %B %d, %Y", timeData);     //https://www.geeksforgeeks.org/strftime-function-in-c/
    timeFile = fopen(timeFileName, "w");				//save the converted time to the time file
    fprintf(timeFile,"%s\n",timeDisplay);
    fclose(timeFile);									//close the file
}


//New Thread Function
	//create a new time thread and run GetTimeFile in the new thread
	//waits for the time thread to finish before returning
int NewThread()
{
    pthread_t timeThread;									//name new thread
    pthread_mutex_lock(&ptrMutex);							//lock mutex
    pthread_create(&timeThread, NULL, GetTimeFile, NULL);	//create new thread running get time file function
    pthread_mutex_unlock(&ptrMutex);						//unlock mutex
    pthread_join(timeThread, NULL);							//wait for new thread to finish

    return 1;
}


//Start Function
	//runs through the turn
void Start()
{
	int i, j, index;
	int lastStep = 0;
	int roomFlag = -1;
    int steps[MAX_STEPS];
    steps[0] = FindStart();		//return the index of the starting room as the begining room
    struct ROOM room;			//room currently "housing" user
    char input[CHAR_BUFFER];	//holds the user input

	//Each Turn or Step
    do {
    	roomFlag = -1;					//reset found room flag 	
        index = steps[lastStep];		//save the current room index
        room = gameArray[index];		//save the current room
        printf("CURRENT LOCATION: %s\n", room.name);
        printf("POSSIBLE CONNECTIONS:");
        for (i = 0; i < room.connCount - 1; i++)	//loop through possible room connections and print corresponding name
		{
            printf(" %s,", room.connArray[i]->name);
        }
        printf(" %s.\n", room.connArray[i]->name);	//add final period per directions
        memset(input,'\0', sizeof(input));			//clear the user input		
        printf("WHERE TO? >");
        //https://www.tutorialspoint.com/c_standard_library/c_function_scanf.htm
        //https://stackoverflow.com/questions/15813408/using-the-scanf-function
        scanf("%255s", input);						//get the user input (prevent buffer overrun by limiting size to one less than input allows)
        printf("\n");

        for (j = 0; j < room.connCount; j++) 		//loop through possible room connections 
		{
            if(strcmp(input, room.connArray[j]->name) == 0) 
			{
                lastStep++;							//if user input is validated than increment the last step
                steps[lastStep] = FindRoom(input);	//save the new room index
                index = steps[lastStep];			//update the active room index
                room = gameArray[index];			//update the active room
                roomFlag = 1;						//update the found room flag
                if(room.type == END_ROOM) 			//test the room type to see if it is the end room
				{
                    printf("YOU WIN !!!\n");		//print success screen with step count
                    printf("STEPS:  %d   \nPATH: ",lastStep);
                    for (i = 1; i <= lastStep; i++) //print path without including the first room per directions
					{
        				printf("\t%s\n", &gameArray[steps[i]].name);
   					}
                    return;
                }
            }
        }
        if(strcmp(input,"time") == 0) 
		{
            if (NewThread() == 1) 
			{
                Time();
        	}
    	}
		else if (roomFlag == -1) 					//if failure to find room, print fail screen and restart turn
		{
            printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        }
    } while(1);
}


//Get Rooms Function
	//sets up the gameArray using the room files in the most recently modified subdirectory w/ "dixonky.rooms."
void GetRooms()
{
	SetDirectory(); //set the most recently created subdirectory as the active directory name
	SetGameArray(); //set up the game array to hold the room files
	GetGameArray(); //add room file names to the game array
	FillGameArray();//add the data from the room files to the game array
}


//Main Function
int main()
{
	GetRooms();
	Start();
	return 0;
}
