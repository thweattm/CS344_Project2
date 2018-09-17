/********************************
 * Mike Thweatt
 * CS344
 * Program 2
 * 05/08/18
 * *****************************/

#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

//Holds room names
char* roomNames[7]; 
//Holds room types (0 = start, 1 = mid, 2 = end)
int roomTypes[7] = {-1,-1,-1,-1,-1,-1,-1};
//Holds connections between rooms
int connections[7][7];
//Holds index of path taken
int path[1024]; 
//Number of steps taken
int steps=0; 
//File to hold the time keeping
char timeFile[] = "currentTime.txt";
//Mutex
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
int continueThread = 1; //Flag for second thread


//Initialize connections array to all 0's
void initializeConnections(){
   int i, j;
   for (i = 0; i < 7; i++){
      for (j = 0; j < 7; j++){
         connections[i][j] = 0;
      }
   }
}


//Return index of given room
int getRoomIndex(char* room){
   int i; 
   int j = -1;

   for (i = 0; i < 7; i++){
      if (strcmp(roomNames[i], room) == 0){
         j = i;
         break;
      }
   }
   return j;
}


//Returns the filename of most recent rooms dir
//Code assistance from 2.4 Manipulating Directories
char* getDir(){
   int timeStamp = -1;
   char target[32] = "thweattm.rooms."; //Prefix to look for
   char* newestDir = malloc(256 * sizeof(char));
   memset(newestDir, '\0', sizeof(newestDir));

   DIR* dirToCheck; //Holds start Dir
   struct dirent *fileInDir; //Holds current file/sub-dir
   struct stat dirAttributes; //Holds info about file/sub-dir

   dirToCheck = opendir("."); //Open current dir

   if (dirToCheck > 0){ //Make sure current dir can be opened
      //Loop through items in dir
      while ((fileInDir = readdir(dirToCheck)) != NULL){
         //If name matches prefix
         if (strstr(fileInDir->d_name, target) != NULL){
            //Get dir attributes
            stat(fileInDir->d_name, &dirAttributes);
            //Check for newer time (bigger int)
            if ((int)dirAttributes.st_mtime > timeStamp){
               timeStamp = (int)dirAttributes.st_mtime;
               memset(newestDir, '\0', sizeof(newestDir));
               strcpy(newestDir, fileInDir->d_name);
            }
         }
      }
   }

   closedir(dirToCheck); //Close dir
   return newestDir;
}


//Get info from the rooms files
void loadRooms(char* roomDir){
   DIR* dirToCheck; //Holds start Dir
   struct dirent *fileInDir; //Holds current file/sub-dir
   int fileDescriptor, x;
   int i=0;
   char filePath[32];

   FILE* fp;
   char* line = NULL;
   size_t len = 0;
   ssize_t read;

   char label[20], data[20];

   //First pass only gets room names
   dirToCheck = opendir(roomDir); //Starting dir to open

   if (dirToCheck > 0){ //Make sure dir can be opened
      //Loop through items in dir
      while ((fileInDir = readdir(dirToCheck)) != NULL){
         if (strstr(fileInDir->d_name, ".") == NULL){

            //Set file path
            memset(filePath, '\0', sizeof(filePath));
            sprintf(filePath, "%s/%s", roomDir, fileInDir->d_name);

            fp = fopen(filePath, "r");
            if (fp == NULL){
               fprintf(stderr, "Could not open %s\n", filePath);
               exit(1);
            }

            while ((read = getline(&line, &len, fp)) != -1){
               sscanf(line, "%*s %s %s", label, data);

               //Read in the rom name
               if (strcmp(label, "NAME:") == 0){
                  roomNames[i] = malloc(8*sizeof(char));
                  strcpy(roomNames[i], data);
               }
            }

            i++; //Advance room index reference
            fclose(fp); //Close file
         }
      }
   }
   closedir(dirToCheck);
   i = 0;

   dirToCheck = opendir(roomDir);
   if (dirToCheck > 0){

      //Loop through files again to fill out connections/types
      while ((fileInDir = readdir(dirToCheck)) != NULL){
         if (strstr(fileInDir->d_name, ".") == NULL) {
            
            //Set file path
            memset(filePath, '\0', sizeof(filePath));
            sprintf(filePath, "%s/%s", roomDir, fileInDir->d_name);

            fp = fopen(filePath, "r");
            if (fp == NULL){
               fprintf(stderr, "Could not open %s\n", filePath);
               exit(1);
            }

            //Loop through rest of lines
            while ((read = getline(&line, &len, fp)) != -1){
               sscanf(line, "%*s %s %s", label, data);

               if (strcmp(label, "NAME:") == 0){
                  //Skip the name

               //Room Type
               } else if (strcmp(label, "TYPE:") == 0){
                  if (strcmp(data, "START_ROOM") == 0){
                     roomTypes[i] = 0;
                  } else if (strcmp(data, "END_ROOM") == 0){
                     roomTypes[i] = 2;
                  } else {
                     roomTypes[i] = 1;
                  }

               //Connections
               } else {
                  x = getRoomIndex(data);
                  connections[i][x] = 1;
                  connections[x][i] = 1;
               }
            }

            fclose(fp); //Close file
            i++; //Advance current room index reference

         }
      }
   }

   closedir(dirToCheck);
   free(line);
}



//Write current time to file
//Help here: https://linux.die.net/man/3/strftime
//Also here for thread/mutex: youtu.be/GXXE42bkqQk
void* currentTime(void *arg){
   char outstr[200];
   time_t t;
   struct tm *tmp;
   int file;

   do{
      //Loop until it gets a lock
      while(pthread_mutex_trylock(&myMutex) != 0)
         wait(1);

      //Get the time string
      memset(outstr, '\0', sizeof(outstr));
      t = time(NULL);
      tmp = localtime(&t);
      if (tmp == NULL){
         perror("localtime");
         exit(EXIT_FAILURE);
      }
      if (strftime(outstr, sizeof(outstr), "%l:%M%P, %A, %B %d, %Y\n", tmp) == 0){
         fprintf(stderr, "strftime returned 0");
         exit(EXIT_FAILURE);
      }

      //printf("Result string is \"%s\"\n\n", outstr);

      //Write timestamp to file
      file = open(timeFile, O_WRONLY | O_CREAT, 0600);

      if (file < 0){
         printf("Error writing to file: %s\n", timeFile);
         exit(1);
      }

      write(file, outstr, strlen(outstr) * sizeof(char));
      close(file);

      //Unlock the mutex (which allows main to relock it)
      pthread_mutex_unlock(&myMutex);

   } while (continueThread == 1);
}




//Main function to play the game
void playGame(){
   pthread_t threadID;
   int i, x, y, counter, file;
   char currentRoom[8];

   FILE* fp;
   char* line = NULL;
   size_t len = 0;
 
   //Mutex Lock & start 2nd thread
   pthread_mutex_lock(&myMutex);
   pthread_create(&threadID, NULL, currentTime, NULL);

   //Find start room
   for (i = 0; i < 7; i++){
      if (roomTypes[i] == 0){
         x = i;
         break;
      }
   }
   memset(currentRoom, '\0', sizeof(currentRoom));
   strcpy(currentRoom, roomNames[x]);

   do {
      do {
         printf("CURRENT LOCATION: %s\n", roomNames[x]);
         printf("POSSIBLE CONNECTIONS: ");
      
         counter=0;
         for (i = 0; i < 7; i++){
            if (connections[x][i] == 1){
               if (counter > 0){
                  printf(", %s", roomNames[i]);
               } else {
                  printf("%s", roomNames[i]);
               }
               counter++;
            }
         }

         //Get user input for next destination
         printf(".\nWHERE TO? >");
         memset(currentRoom, '\0', sizeof(currentRoom)); 
         scanf("%s", currentRoom);

         //Time function
         if (strcmp(currentRoom, "time") == 0){
            do {

               //Unlock mutex to allow 2nd thread to go
               pthread_mutex_unlock(&myMutex);
               //Relock mutex
               pthread_mutex_lock(&myMutex);

               //Read timestamp file contents
               fp = fopen(timeFile, "r");
               if (fp == NULL){
                  fprintf(stderr, "Could not open %s\n", timeFile);
                  exit(1);
               }

               //Get string from file
               getline(&line, &len, fp);
               printf("\n%s", line);
               fclose(fp);

               //Get user input for next destination
               printf("\nWHERE TO? >");
               memset(currentRoom, '\0', sizeof(currentRoom)); 
               scanf("%s", currentRoom);

           } while (strcmp(currentRoom, "time") == 0);
         }

         //Validate room text
         y = x; //Keep a backup in case of fail
         x = getRoomIndex(currentRoom);
         
         //Make sure user input is valid room,
         //that it's not the same room
         //and that's the connection is valid
         if (x == -1 || x == y || connections[y][x] == 0){
            printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
            x = y; //Restore room index
         }

      } while (x == -1 || x == y);

      //Increment steps, add to path
      path[steps] = x;
      steps++;
      printf("\n"); //Add line break between rooms

   } while (roomTypes[x] != 2);

   //End Room found, print results (don't report index 0 - start room)
   printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
   printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
   for (i = 0; i < steps; i++){
      printf("%s\n", roomNames[path[i]]);
   }

   //Stop 2nd thread and wait for it to complete before exiting
   pthread_mutex_unlock(&myMutex);
   continueThread = 0;
   pthread_join(threadID, NULL);

}



int main(){
   initializeConnections(); //Initialize array
   char* dirName = getDir(); //Get newest Dir
   loadRooms(dirName); //Gets all room data
   playGame(); //Starts the game

   //Free dynamically allocated memory
   free(dirName);
   int i;
   for (i = 0; i < 7; i++){
      free(roomNames[i]);
   }

   return(0);
}
