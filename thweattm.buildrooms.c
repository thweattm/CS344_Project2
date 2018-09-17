/**********************************************************
 * Mike Thweatt
 * CS344
 * Project 2
 * 05/08/18
 * Note: Build Rooms outline used from section 2.2
 * *******************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>


//Struct for a new room
struct room{
   int id;
   char* name;
   char* type;
   int connections;
   struct room* connectedRooms[6];
};



//Make & return new room with given details
struct room* newRoom(int id, char* name){
   struct room* thisRoom = malloc(sizeof(struct room));
   thisRoom->id = id;
   thisRoom->name = calloc(16, sizeof(char));
   memset(thisRoom->name, '\0', strlen(thisRoom->name) * sizeof(char));
   strcpy(thisRoom->name, name);
   thisRoom->type = calloc(16, sizeof(char));
   memset(thisRoom->type, '\0', strlen(thisRoom->type) * sizeof(char));
   strcpy(thisRoom->type, "none");
   thisRoom->connections=0;
   return thisRoom;
}


//Return bool if graph is complete or not
bool isGraphFull(struct room* rooms[], int numRooms){
   int i;
   for (i = 0; i < numRooms; i++){
      if (rooms[i]->connections < 3)
         return false;
   }

   //Return true if no false is found
   return true;
}


//Get random room, no checks employed
struct room* getRandomRoom(struct room* rooms[], int numRooms){
   int i = rand() % numRooms;
   struct room* thisRoom = rooms[i];
   return thisRoom;
}


//Check if room has an available connection
bool canAddConnection(struct room* A){
   if (A->connections >=6)
      return false;
   else
      return true;
}


//Check if two rooms are the same room
bool isSameRoom(struct room* A, struct room* B){
   if (A->id == B->id)
      return true;
   else
      return false;
}


//Check if connection already exists between two rooms
bool alreadyConnected(struct room* A, struct room* B){
   int i;
   if (A->connections > 0){
      for (i = 0; i < A->connections; i++){
         if (A->connectedRooms[i]->name == B->name)
            return true;
      }
   }
   return false;
}


//Connect two rooms together
void connectRooms(struct room* A, struct room* B){
   int i = A->connections;
   A->connectedRooms[i] = B;
   A->connections++;
}


//Determine two random rooms to create connection between
void addRandomConnection(struct room* rooms[], int numRooms){
   struct room* A;
   struct room* B;

   //Get first room
   while (true){
     A = getRandomRoom(rooms, numRooms);
      if (canAddConnection(A) == true)
         break;
   }

   //Get second room
   do {
     B = getRandomRoom(rooms, numRooms);
   } while (canAddConnection(B) == false || 
         isSameRoom(A, B) == true || 
         alreadyConnected(A, B) == true);

   //Connect the two rooms in one direction
   connectRooms(A, B);
   //Connect the two rooms in the reverse direction
   connectRooms(B, A);
}


//Return random room name
char* getName(char* nameList[], int numRooms, struct room* rooms[], int usedRooms){
   bool match;
   int i, r;
   char* thisName;
   
   //Loop until unused name is found
   do {

      //Get random integer between 0 & numRooms
      r = rand() % numRooms;
      thisName = nameList[r];

      //Loop through existing names
      match = false;
      if (usedRooms > 0) {
	 //Loop through existing rooms to make sure name isn't used
         for (i = 0; i < usedRooms; i++){
            if (strcmp(rooms[i]->name, thisName) == 0){
               match = true;
	       break;
            }
	 }
      }
   } while (match == true);

   return thisName;
}


//Randomly assign room types
void assignRoomTypes(struct room* rooms[], int numRooms){
   int i, j;

   //Assign start room
   i = rand() % numRooms;
   strcpy(rooms[i]->type, "START_ROOM");

   //Assign End Room
   do {
      j = rand() % numRooms;
   } while (i == j);
   strcpy(rooms[j]->type, "END_ROOM");

   //Assign remainder of rooms to "mid_room"
   for (i = 0; i < numRooms; i++){
      if (strcmp(rooms[i]->type, "none") == 0){
         strcpy(rooms[i]->type, "MID_ROOM");
      }
   }
}



/***************
 * MAIN
 * ************/
int main(int argc, char* argv[]){

   srand(time(NULL)); //Seed for random number generator   

   //Create array of 10 possible room names
   int totalNames = 10;
   char *roomNames[10] = {"Kitchen", "Dining", "Theater", "Bedroom", 
      "Kidroom", "Basement", "Attic", "Bathroom", 
      "Gameroom", "Famroom"};

   //Create array of 7 room pointers
   int totalRooms = 7;
   struct room* rooms[totalRooms];

   //Misc variables
   char* newName;
   int i, j;

   //Create rooms
   for (i = 0; i < totalRooms; i++){
      newName = getName(roomNames, totalNames, rooms, i);
      rooms[i] = newRoom(i, newName);
   }

   //Assign room types
   assignRoomTypes(rooms, totalRooms);
 

   //Build graph of connected rooms
   while (isGraphFull(rooms, totalRooms) == false){
      addRandomConnection(rooms, totalRooms);
   }  

/*
   //Print out details of each room
   for (i = 0; i < totalRooms; i++){
      printf("ROOM NAME: %s\n", rooms[i]->name);
      //Print connections
      for (j = 0; j < rooms[i]->connections; j++){
         printf("CONNECTION %d: %s\n", j+1, rooms[i]->connectedRooms[j]->name);
      }
      printf("ROOM TYPE: %s\n\n", rooms[i]->type);
   }
*/

   //Output rooms to files
   int result, file;
   int PID = getpid();
   char dirName[32];
   char buffer[32];
   memset(dirName, '\0', sizeof(dirName));
   sprintf(dirName, "./thweattm.rooms.%d", PID);
   result = mkdir(dirName, 0755);
   
   //Loop through the rooms, writing each one to a file
   for (i = 0; i < totalRooms; i++){
      //Create fileName/path
      char *fileName = malloc(strlen(dirName)+strlen(rooms[i]->name)+2);
      memset(fileName, '\0', sizeof(fileName));
      sprintf(fileName, "%s/%s", dirName, rooms[i]->name);
      file = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0600);

      //Confirm no error
      if (file < 0){
         printf("Error opening/creating file: %s\n", fileName);
         exit(1);
      }

      //Write room details to file
      //Name
      memset(buffer, '\0', sizeof(buffer));
      sprintf(buffer, "ROOM NAME: %s\n", rooms[i]->name);
      write(file, buffer, strlen(buffer) * sizeof(char));

      //Connections
      for (j = 0; j < rooms[i]->connections; j++){
         memset(buffer, '\0', sizeof(buffer));
         sprintf(buffer, "CONNECTION %d: %s\n", j+1, rooms[i]->connectedRooms[j]->name);
         write(file, buffer, strlen(buffer) * sizeof(char));
      }

      //Type
      memset(buffer, '\0', sizeof(buffer));
      sprintf(buffer, "ROOM TYPE: %s\n", rooms[i]->type);
      write(file, buffer, strlen(buffer) * sizeof(char));
      
      //Close room file
      close(file);
      free(fileName); //Free fileName for next loop
   }

   //Free all dynamically allocated memory
   for (i = 0; i < totalRooms; i++){
      free(rooms[i]->name);
      free(rooms[i]->type);
      free(rooms[i]); 
   }
   

   return 0;
}
