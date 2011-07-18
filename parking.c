/*
 * parking.c
 *
 *  Created on: 01/12/2010
 *      Author: daniel
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>			/* NULL */
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_CARS 1000

// Definition of the variables
int totalSlots;
int levels;
int parkingSpaces;
int** parking;
FILE *file;
int identifierMax=0;

pthread_cond_t parkingFull = PTHREAD_COND_INITIALIZER;
pthread_cond_t leaveSpace = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexin = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexout = PTHREAD_MUTEX_INITIALIZER;

void *assignSpace(void *threadid){
	// When thread goes in, it checks if the parking is full.
	pthread_mutex_lock(&mutexin);
	if(totalSlots <= 0){
		printf("Parking full. Thread %d will wait\n",threadid);
		//printf("Parking full. Thread %d will wait\n",pthread_self());
		pthread_cond_wait(&leaveSpace, &mutexin);
	}
	totalSlots--;
	if ((fprintf(file,"[%d] in the parking, %d vacancies\n", threadid, totalSlots))<0) {
				perror("write error");
	}
	//printf("%d in the parking, %d vacancies\n",pthread_self(), totalSlots);
	int i,j, ib,jb,k, counter = 0;
	// The car must look for a place in p floor.. then p+1 etc..
	for( i = 0; i < levels; i++){
	   for( j = 0; j < parkingSpaces; j++){
	      if(parking[i][j] == 0){
	    	  printf("Level %d. Position %d is free. Thread %d is taking it...",i,j,threadid);
	    	  parking[i][j]=1;
	    	  printf("TAKEN \n");
	    	  ib=i; jb=j;
	    	  for(k = 0 ; k < parkingSpaces; k++)
	    		  if(parking[i][k] == 0)
	    			  counter++;
	    	  if ((fprintf(file,"[%d] arrives at level %d, %d free spaces on the level\n", threadid, i, counter))<0) {
	    		  perror("write error");
	    	  }
	    	  j=parkingSpaces;i=levels; // This forces the program to go out of the loop
	      }
	   }
	}
	pthread_mutex_unlock(&mutexin);
	pthread_mutex_lock(&mutexout);
	sleep(random()%10);
	//pthread_mutex_unlock(&mutexout);
	//printf("%d leaves parking, %d vacancies\n",pthread_self(), totalSlots);
	counter=0;
	parking[ib][jb]=0;
	for(k = 0 ; k < parkingSpaces; k++)
		    		  if(parking[ib][k] == 0)
		    			  counter++;
	if ((fprintf(file,"[%d] leaves level %d, %d free spaces on the level\n",threadid, ib, counter))<0) {
		   perror("write error");
	}
	totalSlots++;
	printf("%d leaves parking, %d vacancies\n",threadid, totalSlots);
	if ((fprintf(file,"[%d] leaves the parking, %d vacancies\n", threadid, totalSlots))<0) {
					perror("write error");
	}
	// As soon as the thread finishes a signal is emitted to the threads waiting.
	pthread_cond_signal(&leaveSpace);
	pthread_mutex_unlock(&mutexout);
	pthread_exit(0);
}

int main (int argc, char *argv[]){
	if (argc != 4){
		printf("Sorry, at least three parameters, number of levels, number of spaces"
				" and file name are required to run the program. \n You wrote %d parameters"
				"\nExiting... ", argc);
		exit(1);
	}
	printf("Assigning levels...");
	levels = atoi(argv[1]);
	printf("DONE\n");
	printf("Assigning parking spaces per level...");
	parkingSpaces = atoi(argv[2]);
	printf("DONE\n");
	totalSlots = levels * parkingSpaces;
	printf("Initializing parking spots...");
	int i;
	parking = malloc(levels*sizeof(int *));
	if(parking == NULL){
				perror("out of memory\n");
				exit(1);
	}
	for( i = 0; i < levels; i++){
		parking[i] = malloc(parkingSpaces*sizeof(int));
		if(parking[i] == NULL){
			perror("out of memory\n");
			exit(1);
		}
	}
	// Convention is : 0 empty spot, 1 occupied spot.
	printf("DONE\n");
	printf("The parking has %d total slots. All of them are empty\n",totalSlots);
	printf("Creating output file...");
	const char *textfilename = argv[3];
	if ((file = fopen(textfilename,"a+")) == NULL) {
			perror("Error when creating text file");
			return 1;
	}
	printf("DONE\n");
	int nrOfCars;
	int t, rc;
	int stillCarsIn = 1;
	pthread_t threads[MAX_CARS];
	while(stillCarsIn){
		printf("How many cars are trying to enter the parking?\n");
		scanf("%d",&nrOfCars);
		if(nrOfCars < 0){
			printf("Sorry, you cannot have a negative number of cars. Exiting...");
			fclose(file);
			exit(1);
		}
		if(nrOfCars == 0){
			printf("Wait until the cars exit the parking...\n");
			//Wait for the cars to exit
			while(totalSlots != levels * parkingSpaces);
			printf("All the cars are out. Parking is empty.\n");
			printf("Exiting...");
			fclose(file);
			exit(1);
		}
		if(nrOfCars > 0){
			//Assign a thread to each car trying to enter.
			for(t=0;t < nrOfCars;t++){
					printf("Creating thread for car %d\n", identifierMax+t);
					// Assign a car to a position if possible in assignSpace
					rc = pthread_create(&threads[t], NULL, assignSpace, (void *)identifierMax+t);
					if (rc){
					   printf("ERROR; return code from pthread_create() is %d\n", rc);
					   exit(-1);
					}
			}
			identifierMax=nrOfCars;
			//If not possible, car waits.
		}

	}
	return 0;
}
