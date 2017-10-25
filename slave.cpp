/**
 * Author: Jason Klamert
 * Date: 10/8/2016
 * Description: Slave program code to implement Peterson's algorithm and introduce race conditions.
 **/

#include <sys/wait.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <errno.h>
#include <ctime>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include "header.h" 
using namespace std;

#define SHMSZ 1024
#define DEBUG false

void signal_callback_handler(int signum);
int getRandom();
string timeStamp();
string buildMessageFormat(char** argv, pid_t processID, int sharedNum);
timespec getTimeSinceEpoch();
void killChild(int sig);
void logMessage(char* fileName, string message);
pid_t slave;
volatile sig_atomic_t atomic = 1;

int main(int argc, char** argv){
	    
	    signal(SIGINT, signal_callback_handler);

    	    srand(time(NULL));
	   
            key_t key = 56780321;
            data* sharedData;
            int shmid;
	    int Num;
	    int processID;
	    int* Iterations = (int*) malloc(sizeof(int));
	    *Iterations = 3;
	    slave = getpid();
	    int timeArg = 20;
   	    char c;
	    extern int optind;
	    extern char* optarg;
	    struct shmid_ds* buffer;

	     while ((c = getopt(argc, argv, "i:s:t:")) != -1)
             {
                switch (c)
                {

                	 case 'i':
                	 {
                	 	if(DEBUG == true){
                	 		cout << "Inside of case: i \n";
                	 		cout << "optarg: " << optarg << "\n";
                	 	}
                	 	
				*Iterations = atoi(optarg);

				if(DEBUG == true){
                	 		cout << "-----------------------------------------------------------------------\n";
                	 		cout << "Number of Slave Iterations in Critical Section: " << *Iterations << "\n";
                	 		cout << "-----------------------------------------------------------------------\n";
				}
                	 	break;
                	 }
			case 's':
			{
				if(DEBUG == true){
					cout << "Inside of case: s \n";
					cout << "optarg: " << optarg << endl;
				}
				
				processID = atoi(optarg);
				
				if(DEBUG == true){
                			cout << "-----------------------------------------------------------------------\n";
                			cout << "PID: " << processID << "\n";
                			cout << "-----------------------------------------------------------------------\n";
				}
				break;
			}
			case 't':
			{
				if(DEBUG == true){
					cout << "Inside of case: t \n";
					cout << "optarg: " << optarg << endl;
				}

				timeArg = atoi(optarg);
	
				if(DEBUG == true){
					cout << "-----------------------------------------------------------------------\n";
                         		cout << "Time Arg: " << timeArg << "\n";
                         		cout << "-----------------------------------------------------------------------\n";
                        	}
				break;
			}
	
	                 default:
			{
	                 if(DEBUG == true){
	                        cout << "Inside of case: default \n";
	                 }
				errno = EINVAL;
				perror("GetOpt Slave Default");
	                        cout << "Error: Unrecognized flag passed to slave! Failing silently.\n";
	                        return 2;
	                        break;
	
	
	                }
	            }
		}
	
		if(DEBUG == true){
		cout << "Slave timeArg: " << timeArg << endl;
		cout << "Slave ProcessID: " << processID << endl;
		cout << "Slave Iterations: " << *Iterations << endl;
		}

           /**
            * Locate the segment.
            **/
            if ((shmid = shmget(key, sizeof(data)*2, IPC_CREAT | 0777)) < 0) {
                perror("shmget");
                exit(1);
            }
		
	    if ((sharedData = (data*) shmat(shmid, NULL, 0)) == (void*) -1) {
       		perror("Shmat Error");
        	exit(1);
            }
	   
	    signal(SIGINT, SIG_IGN);
	    signal(SIGQUIT, signal_callback_handler);

  	    //Set the alarm for time constraints. This protects against zombies.
    	    signal(SIGALRM, killChild);		
	    alarm(timeArg+10);

  	int i = 0;
  	int j;
  	int random;

  	//Perform iteration times while quit has not been sent during signal handling.
    	while(i < (*Iterations) && atomic )
	{
        	do {
  
              	//Raise a flag for the process that wants in to crit.
              	sharedData->flag[processID] = want_in;   
                //Local variable for Peterson's.
              	j = sharedData->turn;
  
              	//Busy wait until it's my turn and stop on j'th process if it is not idle. Otherwise go to next process and wait.
              	while(j != processID) {
                	  j = (sharedData->flag[j] != idle) ? sharedData->turn : (j + 1) % sharedData->numberProc;
              	}
  
  
              	//Declare the processes want to enter the critical section
              	sharedData->flag[processID] = in_cs;
              	//Check that no other process is in the critical section
              	for(j = 0; j < sharedData->numberProc; j++) {
              	    if((j != processID) && (sharedData->flag[j] == in_cs)) {
              	       break;
              	    }
              	}
  	
   		    }while ((j < sharedData->numberProc) || (sharedData->turn != processID && sharedData->flag[sharedData->turn] != idle));
    		  	  //About to enter the critical section.
			  random = getRandom();
			  sleep(random);
	
			  fprintf(stderr, "Process %d entering critial at: %s\n", processID+1, timeStamp().c_str());

  	          
			  //Increment sharedNum
  		          sharedData->sharedNum++;
  		
        	 	  //Assign turn to this process and enter the critical section
        	 	  sharedData->turn = processID;
  
        	          string Message = buildMessageFormat(argv, slave, sharedData->sharedNum);

   		          logMessage("test.out",Message);

			  random = getRandom();
			  fprintf(stderr, "Process %d exiting critial at: %s\n", processID+1, timeStamp().c_str());
			  sleep(random);
 	 
 	                 //Exit the critical section and change turn.
 	                 j = (sharedData->turn + 1) % sharedData->numberProc;
 	
  			  //Find the next process that isn't idle
  			  while(sharedData->flag[j] == idle) {
  			  	j = (j + 1) % sharedData->numberProc;
  	                  }
  	
                  	  //Assign turn to next waiting process and set flag to idle
  		  	  sharedData->turn = j;
  		  	  sharedData->flag[processID] = idle;
	
		  	  sleep(rand() % 5);
		  	  i++;
		 }
		return 0;
}
     
/**
 * Author: Jason Klamert
 * Date: 9/25/2016
 * Description: Signal Handler for children.
 */
void signal_callback_handler(int signum)
{

   struct shmid_ds* buffer;
   int shmid;
   key_t key = 56780321;

   signal(SIGQUIT, SIG_IGN);
   signal(SIGINT, SIG_IGN);

   printf("\nCAUGHT SIGNAL %d\n",signum);

   if ((shmid = shmget(key, sizeof(data)*2, IPC_CREAT | 0777)) < 0) {
        perror("shmget");
        exit(1);
        }

   if(shmctl(shmid, IPC_RMID, buffer) < 0){
                        perror("shmctl");
                        exit(1);
                }

   signal(SIGQUIT, SIG_IGN);
   signal(SIGINT, SIG_IGN);

   if(signum == SIGINT) {
    fprintf(stderr, "\nCtrl-C received. Exiting program!\n");
   }

   if(signum == SIGALRM) {
    fprintf(stderr, "Master program has timed out. Shutting down program!\n");
   }

   kill(-getpgrp(), SIGQUIT);
   kill(-getpgrp(), SIGKILL);

   exit(signum);           
}

/**
 * Author: Jason Klamert
 * Date: 9/25/2016
 * Description: Function to kill child whose parent has timed out.
 **/
void killChild(int sig) {
  printf("slave %d is killing itself due to slave timeout override\n", slave);
        kill(slave, SIGKILL);
}


/**
 * Author: Jason Klamert
 * Date: 9/25/2016
 * Description: Signal Handler for master.
 */
int getRandom(){
	return (rand() % 3);
}


 /**
 * Description: function that returns a buffer containing the current human readable timestamp.
 * Author: Jason Klamert
 * Date: 9/25/2016
 **/       
string  timeStamp()
{
    time_t now;
    struct tm ts;
    char buffer[50];
    //Get current time
    time(&now);
    //Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&now);
    strftime(buffer, sizeof(buffer)+1, "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    return string(buffer);
}

/**
 * Description: function that logs the given message to given file.
 *
 * Author: Jason Klamert
 * Date: 9/25/2016
 **/
void logMessage(char* fileName, string message){

        bool status = true;
        ofstream outfile;
        outfile.open(fileName, ios::out | ios::app);
        if(outfile){
             outfile << message;
         }else{
               status = false;
         }
         outfile.close();

        //If log filewrite was successful, return 0.
                if(status){
                        printf("Success: savelog successful.\n");
                }
        //If log filewrite was unsuccessful, return -1.
                else{
                errno = EIO;
                perror("Error: savelog unsuccessful. File open unsuccessful.");
        	}
        }
        


 /**
 * Description: function that prints the proper usage of the routine.
 * Author: Jason Klamert
 * Date: 9/6/2016
 **/
string buildMessageFormat(char** argv, int processID, int sharedNum){
   
    time_t now;
    struct tm ts;
    char buf[80];
    char shared[50];
    char pid[100];

     //Get current time
     time(&now);
     //Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
     ts = *localtime(&now);
     strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
     sprintf(shared, "%i", sharedNum);
     sprintf(pid, "%i", slave);

     string str;
     str.append("File modified by process number ");
     str.append(pid);
     str.append(" at time ");
     str.append(buf);
     str.append(" with sharedNum = ");
     str.append(shared);
     str.append("\n");
     return str;
}


/**
 * Description: function that returns the time in nanoseconds from the epoch time.
 *
 * Author: Jason Klamert
 * Date: 9/25/2016
 **/
timespec getTimeSinceEpoch(){

       struct timespec ts;
       clock_gettime(CLOCK_REALTIME, &ts);

       return ts;
}


