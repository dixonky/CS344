/*******************************************************************************
* ** Author: Kyle Dixon
* ** Date: 4/30/2019-
* ** Descriptions: Build Rooms Program (Program 2)
* ** 	
* ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h> 	//for getpid()
#include <sys/stat.h>	//for mkdir() 
#include <sys/types.h>	//for mkdir() 


//Game Constant Constraints						//Use constants to change game mechanics easily
#define CHAR_BUFFER 256
#define PERMISSIONS 0770
#define TOTAL_ROOMS 10
#define SELECTED_ROOMS 7
#define MAX_CONNECTIONS 6
#define MIN_CONNECTIONS 3


//GLOBAL GAME VARIABLES
//Room name array
	//names based on the game Clue
char* ROOMS[TOTAL_ROOMS] = {"Dining", "Lounge", "Kitchen", "Study", "Grand", "Game", "Patio", "Ballroom", "Library", "Cellar"};


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


//Create Directory Function
	//creates a new directory is the same location as the c file
	//directory is dixonky.rooms. followed by the process ID
void CreateDir()
{
	char* name = "dixonky.rooms.";				
	int processID = getpid();					
	char nameDir[CHAR_BUFFER];					//create a char array to hold the name for the directory
	memset(nameDir, '\0', sizeof(nameDir));		//fill the char array with new line characters
	sprintf(nameDir, "%s%d", name, processID);	//add my name and the process id as the name
	mkdir(nameDir, PERMISSIONS); 							//create the directory
	chmod(nameDir, PERMISSIONS);				//give permissions for the directory
	if(chdir(nameDir) != 0)						//Change active directory to new directory and test success
	{
        printf("DIR NOT CHANGED TO: %s\n", nameDir);
        return;
    }
}


//Generate Rooms Function
	//first fills roomIndexes with 7 unique ints
void GenerateRooms()
{
	int i = 0, j = 0;
	int roomIndexes[SELECTED_ROOMS];
	for (i=0; i <SELECTED_ROOMS;i++)			//create an array of unique indexes from 0-9, to be used for generating unique rooms
	{
		int randInd = rand() % TOTAL_ROOMS;
		roomIndexes[i] = randInd;
		for (j=0; j<i; j++)
		{
			while (roomIndexes[i] == roomIndexes[j])	//check the random number against all saved numbers
			{
				randInd = rand() % TOTAL_ROOMS;	//change the number if it occurs already and restart check
				roomIndexes[i] = randInd;
				j=0;
			}
		}
	}

	for (i=0; i <SELECTED_ROOMS;i++)			//Add rooms to game array based on index array
	{
		memset(gameArray[i].name,'\0', sizeof(gameArray[i].name));
		int x = roomIndexes[i];					
        strcpy(gameArray[i].name,ROOMS[x]);		//get the string name from the room name array
        gameArray[i].type = MID_ROOM;			//set all rooms to mid as default
        gameArray[i].connCount = 0;				//set the initial connections as zero
	}
	
	gameArray[0].type = START_ROOM;				//set the start and end rooms
    gameArray[SELECTED_ROOMS - 1].type = END_ROOM;
}


//Connect Function
	//adds a connection between the two passed in room indeces and increments room connection counts
void Connect(int x, int y)
{
	struct ROOM roomA = gameArray[x];
	struct ROOM roomB = gameArray[y];
	int countA = roomA.connCount;
	int countB = roomB.connCount;
	int i;
	for (i=0;i<countA;i++)
	{
		if (strcmp(roomA.connArray[i]->name, roomB.name) == 0)
		{
			return;
		}
	}
	if (countA >= MAX_CONNECTIONS || countB >= MAX_CONNECTIONS)
	{
		return;
	}
	else
	{
		gameArray[x].connArray[countA] = &gameArray[y];
		gameArray[y].connArray[countB] = &gameArray[x];
		gameArray[x].connCount++;
		gameArray[y].connCount++;
		return;
	}
}


//Connect Rooms Function
	//Loops through the game room array, randomly assigning rooms at least the minimum connections
void ConnectRooms()
{
	int i;
	for (i=0; i <SELECTED_ROOMS;i++)				//loop through the rooms
	{
		do
		{
			int x = rand() % SELECTED_ROOMS;		//generate a random index for another room
			while(i == x)							//make sure that the index is not for the current room
			{
				x = rand() % SELECTED_ROOMS;		
			}
			Connect(i,x);							//connect the two rooms with the connect function
		}while(gameArray[i].connCount <= MIN_CONNECTIONS);
	}
}


//Export Files Function
	//Creates new room files according to the project directions
void ExportFiles()
{
	FILE * filePtr;
	int i;
	for(i = 0; i < SELECTED_ROOMS; i++)
	{
        filePtr = fopen(gameArray[i].name,"w");
        fprintf(filePtr,"ROOM NAME: %s\n", gameArray[i].name);
        int j;
        for(j = 0; j < gameArray[i].connCount; j++)
		{
            fprintf(filePtr,"CONNECTION %d: %s\n", j+1, gameArray[i].connArray[j]->name);
        }
        if(gameArray[i].type == START_ROOM)
		{
            fprintf(filePtr,"ROOM TYPE: %s\n", "START_ROOM");
        }
        else if(gameArray[i].type == MID_ROOM)
		{
            fprintf(filePtr,"ROOM TYPE: %s\n", "MID_ROOM");
        }
        else if(gameArray[i].type == END_ROOM)
		{
            fprintf(filePtr,"ROOM TYPE: %s\n", "END_ROOM");
        }
        fclose(filePtr);
    }
}


//Main Function
int main()
{
	srand(time(NULL));	//seed random number generator
	CreateDir();		//Create Directory for room files
	GenerateRooms();	//Fills game array with unique rooms
	ConnectRooms();		//Create the connections between the rooms
	ExportFiles();		//Export the room files into the new directory
	return 0;
}
