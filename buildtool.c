/*Abhinav Singhal
 *ICSI 333
 *BuildTool
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h> 

/* global variable declaration */
char cwd[1024];
pthread_t id[4];
sem_t mutex;
char parentDirectory[1024] = "";

/* function declaration */
void recursedir(char *name);
void *handleFile(void *value);
void linker();
char *getExt(char *path);
char *changeExt(char *cfile);
char *getFolder(char *folderPath);


/*
 * Function: main
 * --------------
 * Recursively descends into child directories and compiles c code
 * Creates object files in directorys and links them into library files
 * Moves Library files to start directory and creates executable
 *
 * return: zero if successfull
 */
int main() {
	char copyCWD[1024];
	//semaphore initialized at 4.
	sem_init(&mutex, 0, 4);
	//get current directory
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
       perror("getcwd() error");
       return 1;
	}
	//create copy of current directory
	strcpy(copyCWD, cwd);
	//get the parent folder name 
	strcpy(parentDirectory, getFolder(copyCWD));
	//add .a to be used later with strcmp
	strcat(parentDirectory, ".a");
	//start recursion on current directory
	recursedir(cwd);
	linker(); //link all files 
	
    return 0;
}

/*
 * Function: recursedir
 * --------------------
 * Recursively descends into child directories and compiles c code
 * Creates libraries from .o files
 * 
 * path: given directory path
 */
void recursedir(char *path){
	int filecounter = 0;
	char command[20000] = "";
	char directory[1024] = "";
	char tempPath[1024] = "";
	DIR *dir, *count;
	struct dirent *next, *entry;
	//set current directory as work directory
	if(chdir(path) == -1) printf("Couldn't change active directory");
	
	//go through the current directory to count how many c files there are
	if ((count = opendir(path)) == NULL){
		printf("Directory failed to open\n");
		return;
	}
	while((entry = readdir(count)) != NULL){
		if((entry->d_type == DT_REG) && (strcmp(getExt(entry->d_name), ".c") == 0)){
			filecounter++;
		}
	}
	closedir(count);
	
	//if the current directory has c files
	if(filecounter > 0 ) {
		//create the start of the system command to create library
		strcpy(command, "ar -rc ");
		//copy current path
		strcpy(tempPath, path);
		//get name of current directory
		strcpy(directory, getFolder(tempPath));
		//add .a to end of name
		strcat(directory, ".a");
	}
	
	//create a 2d array to store file names
	char *ar[filecounter];
	for(int i = 0; i < filecounter; i++){
		ar[i] = (char *)malloc(1024 * sizeof(char));
	}
	filecounter = 0; //reset counter
	
	//open directory for recursion
    if ((dir = opendir(path)) == NULL){
		printf("Directory failed to open\n");
		return;
	}
	
	while((next = readdir(dir)) != NULL){
		if(next->d_type == DT_DIR){
			//skip "." and ".."
			if (strcmp(next->d_name, ".") == 0 || strcmp(next->d_name, "..") == 0) continue;
			//create new path for child directory
			char newPath[1024];
			snprintf(newPath, sizeof(newPath), "%s/%s", path, next->d_name);
			recursedir(newPath); //recurse next
		}
		else{
			char newPath[1024];
			snprintf(newPath, sizeof(newPath), "%s/%s", path, next->d_name);
			if(strcmp(getExt(next->d_name), ".c") == 0){
				if(chdir(path) == -1) printf("Couldn't change active directory");
				int count;
				sem_wait(&mutex);//lock slot
				sem_getvalue(&mutex, &count);
				//start thread on of the 4 allowed threads
				pthread_create(&id[count], NULL, handleFile, (void*)(newPath));
				//save name of c file in array
				strcpy(ar[filecounter], changeExt(next->d_name));
				filecounter++;
			}
		}
		//wait for all threads to finish
		for(int i = 3; i > -1; i--) pthread_join(id[i], NULL);
	}
	
	if(chdir(path) == -1) printf("Couldn't change active directory");
	//add directory name to command
	strcat(command, directory);
	//add all files compiled to command
	for(int i = 0; i < filecounter; i++){
		strcat(command, " ");
		strcat(command, ar[i]);
	}
	
	//call system to execute command while not in top most directory
	if(strcmp(directory, parentDirectory) != 0) system(command);
	
	//copy top most directory 
	strcpy(tempPath, cwd);
	strcat(tempPath, "/");
	strcat(tempPath, directory);
	
	//move .a files to top most directory
	if(strcmp(directory, parentDirectory) != 0) rename(directory, tempPath);
	
	//free memory and resources
	for(int i = 0; i < filecounter; i++){
		free(ar[i]);
	}
    closedir(dir);
}

/*
 * Function: handleFile
 * --------------------
 * compile given c file into an object file
 * 
 * value: path to c file
 */
void *handleFile(void *value){
	char path[1024];
	char command[2048] = "gcc -c \"";
	
	strcpy(path, value);
	printf("%s\n", path);
	strcat(command, path);
	//add " to the end 
	strcat(command, "\"");
	
	//excute command
	system(command);
	//unlock semaphore slot and end thread
	sem_post(&mutex);
	pthread_exit(NULL);
}

/*
 * Function: getExt
 * --------------------
 * Find the extension of a given file
 * 
 * path: path to file
 *
 * return: the extension starting with '.'
 */
char *getExt(char *path){
	//creates substring containg extension
	char *ext = strrchr(path, '.');
	//if strrchr failed
	if(ext == NULL) ext = "";
	return ext;
}

/*
 * Function: changeExt
 * --------------------
 * changes the extension of a c file to .o
 * 
 * cfile: The c file to change extension of
 *
 * return: Name of c file with new extension
 */
char *changeExt(char *cfile){
	char *file = cfile;
	//change last letter to o
	file[strlen(file) - 1] = 'o';
	return file;
}

/*
 * Function: getFolder
 * --------------------
 * Retreive the name of the current folder from a given path
 * 
 * folderPath: Path to folder
 *
 * return: Name of the folder with the path
 */
char *getFolder(char *folderPath){
	//creates substring containing folder name
	char *name = strrchr(folderPath, '/');
	//removes the first character which is /
	if (name[0] == '/') memmove(name, name+1, strlen(name));
	return name;
}

/*
 * Function: linker
 * --------------------
 * Links buildtool.o with all the .a library files in a directory
 */
void linker(){
	int count = 0;
	char command[20000] = "gcc -o program buildtool.o";
	DIR *dir;
	struct dirent *next;
	//set working directory to top most directory
	chdir(cwd);
	if ((dir = opendir(cwd)) == NULL){
		printf("Directory failed to open\n");
		return;
	}
	while((next = readdir(dir)) != NULL){
		if(strcmp(getExt(next->d_name), ".a") == 0){
			//add on to command every .a file
			strcat(command, " ");
			strcat(command, next->d_name);
		}
	}
	//add -lpthread at the end
	strcat(command, " -lpthread");
	//execute command
	system(command);
	closedir(dir);
}