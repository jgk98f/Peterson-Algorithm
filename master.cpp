/**
 * Description: The logging mechanism provides functionality such as clearing, adding a log message, getlog, and savelog. 
 * This is utilized via the main function which uses getopt and perror to parse the command line for arguments and the master process
 * then creates slaves and the slaves are subject to race conditions while trying to capture a two resources which are a shared
 * variable and a file to write a message to named 'test.out'.
 *
 *  Author: Jason Klamert
 *  Date: 9/25/2016
 *  Log: logfile.txt
 */
#include "header.h" 
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
using namespace std;

#define SHMSZ 1024
#define DEBUG false
#define MAX 20

int logMessage(char*,string);
void usage();
void usage2();
timespec getTimeSinceEpoch();
string buildMessageFormat(char**, string);
void signal_callback_handler(int signum);
string timeStampToHumanReadble(const time_t rawtime);
pid_t master, slave;

/**
 * Description: function that returns the time in nanoseconds from the epoch time.
 * 
 * Author: Jason Klamert
 * Date: 9/25/2016
 */
timespec getTimeSinceEpoch(){
       
       struct timespec ts;
       clock_gettime(CLOCK_REALTIME, &ts);
       
       return ts;
}

/**
 * Description: function that logs the given message to given file.
 * 
 * Author: Jason Klamert
 * Date: 9/25/2016
 */
int logMessage(char* fileName, string message){
     	
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
        return 0;
        }
        //If log filewrite was unsuccessful, return -1.
        else{
             errno = EIO;
             perror("Error: savelog unsuccessful. File open unsuccessful.");
             return -1;
        }
}
 
 /**
 * Description: function that prints the proper usage of the routine.
 * Author: Jason Klamert
 * Date: 9/25/2016
 */
void usage(){
     perror("Previous unrecognized argument. Please retry the program.\nUse the format master [-h, -s #slaves, -i #critical section iterations, -t #seconds till master termination, or -l fileName] \n");
}  

 /**
 * Description: function that prints the proper usage of the routine.
 * Author: Jason Klamert
 * Date: 9/25/2016
 */
void usage2(){
     cout << "Usage format: master [-h, -s #slaves, -i #critical section iterations, -t #seconds till master termination, or -l fileName] \n";
} 

 /**
 * Description: function that returns a buffer containing the current human readable timestamp.
 * Author: Jason Klamert
 * Date: 9/25/2016
 **/
string timeStampToHumanReadble(const time_t rawtime)
{
    struct tm * ts;
    char buffer [30];
    ts = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%m%d%H%M%y", ts);
    return std::string(buffer);
}

 /**
 * Description: function that prints the proper usage of the routine.
 * Author: Jason Klamert
 * Date: 9/6/2016
 */
string buildMessageFormat(char** argv, string error){
     
     string str;
     str.append(*argv);
     str.append(" : ");
     str.append(strerror(errno));
     str.append(" : ");
     str.append(error);
     str.append(".\n");
     
     return str;
}


/**
 * Author: Jason Klamert
 * Date: 9/25/2016
 * Description: Signal Handler for master.
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
    fprintf(stderr, "\nCtrl-C received. Program Exiting!\n");
   }

   if(signum == SIGALRM) {
    fprintf(stderr, "Master has timed out. Exiting Program!\n");
   }

   kill(-getpgrp(), SIGQUIT);
   kill(-getpgrp(), SIGTERM);
 
   exit(signum);
           
}
   


/**
 * Description: Routine to use every function of the logging utility at least twice.
 * This parses the commandline for flags regarding the functions to trigger.
 * Author: Jason Klamert
 * Date: 9/6/2016
 */
int main(int argc, char **argv)
{
    	signal(SIGINT, signal_callback_handler);
	
	extern char *optarg;
	extern int optind;
	char* standard = "logfile.txt";
	int numberSlaves = 5;
	int numberIterations = 3;
	int terminationTime = 20;
	//pid_t processIDs[MAX];
	char c;
	int shmid;
	key_t key;
	struct shmid_ds* buffer;	
	data *sharedData;
	int returnStatus;

	/**
	 * We'll name our shared memory segment
	 * "5678".
	 **/
    	key = 56780321;

   	/**      
  	* Create the segment.
 	**/
    	if ((shmid = shmget(key, sizeof(data)*2, IPC_CREAT | 0777)) < 0) {
        perror("shmget");
        exit(1);
    	}

	/**      
	 * Now we attach the segment to our data space.
	 **/
    	if ((sharedData = (data*) shmat(shmid, NULL, 0)) == (void*) -1) {
        perror("shmat");
        exit(1);
    	}

	
while ((c = getopt(argc, argv, ":hi:t:l:s:")) != -1)
{
	switch (c)
	{

		case 'h':
		{
                 if(DEBUG == true){
                 	cout << "Inside of case: h \n"; 
                 }
                 cout << "-----------------------------------------------------------------------\n";
                 cout << "Instructions:\n";
                 cout << "-----------------------------------------------------------------------\n";
				 usage2();
		 return 0;
		 break;
		}
			
		case 's':
		{
                 if(DEBUG == true){
                 cout << "Inside of case: s \n";
                 cout << "optarg: " << optarg << "\n"; 
                 }
                 numberSlaves = atoi(optarg);
			
		 if(numberSlaves > MAX){
			 cout << "Cannot have greater than 20 slaves! We will use 20 for the program since you broke the cap.\n";
			 numberSlaves = 20;
		 }

                 cout << "-----------------------------------------------------------------------\n";
                 cout << "Number of Slaves: " << numberSlaves << "\n";
                 cout << "-----------------------------------------------------------------------\n";
		 break;
		}
			
		case 'i':
		{
                 if(DEBUG == true){
                 cout << "Inside of case: i \n"; 
                 cout << "optarg: " << optarg << "\n";
                 }
                 numberIterations = atoi(optarg);
                 cout << "-----------------------------------------------------------------------\n";
                 cout << "Number of Slave Iterations in Critical Section: " << numberIterations << "\n";
                 cout << "-----------------------------------------------------------------------\n";
		 break;
		}
			
		case 'l':
		{
                 if(DEBUG == true){
                 cout << "Inside of case: l \n";
                 cout << "OPTARG: ";
                 cout << optarg << "\n"; 
                 }
                 standard = (char*)optarg;
                 cout << "-----------------------------------------------------------------------\n";
                 cout << "Log File: " << standard << "\n";
                 cout << "-----------------------------------------------------------------------\n";
		 break;
		}
			
		case 't':
		{
                	 if(DEBUG == true){
                	 cout << "Inside of case: t \n";
                	 cout << "OPTARG: ";
                	 cout << atoi(optarg) << "\n";
                	 }
                	 terminationTime = atoi(optarg);
                	 cout << "-----------------------------------------------------------------------\n";
			 cout << "Termination Time: " << terminationTime << " seconds\n";
			 cout << "-----------------------------------------------------------------------\n";
			 break;
		}
			
		default:
                 if(DEBUG == true){
                	 cout << "Inside of case: default \n";
                 }
                	 errno = EINVAL;
			 perror("GetOpt Default");
                	 cout << "The flag argument did not match the approved flags.\n";
			 string str = "The flag argument did not match the approved flags.\n";
			 logMessage(standard,str);
                	 usage2();
                	 return -1;
                	 break;
                 
			
		}
	}

		sharedData->sharedNum =  0;
        	sharedData->turn = 0;
		sharedData->numberProc = numberSlaves;

	  	//Override the alarm signal
	  	signal(SIGALRM, signal_callback_handler);
	        
	  	//set alarm to ring in terminationTime seconds
	 	alarm(terminationTime);

		//Allocate our variable to denote process xx.
		int x;
		for(x = 0; x < numberSlaves; x++) {
    			sharedData->flag[x] = idle;
  		}

		char* processX  = (char*) malloc(25);
		char* iterationArg = (char*) malloc(25);
		char* timeArg = (char*) malloc(25);

		for(x  = 0; x < numberSlaves; x++)
		{
			
			if((slave = fork()) == -1)
			{
				fprintf(stderr, "Error: fork failed!\n");
				return 1;
			}
			if(slave == 0)
			{
                                slave = getpid();
                                pid_t pGroup = getpgrp();
                                sprintf(processX, "%d", x);
                                sprintf(iterationArg, "%d", numberIterations);
                                sprintf(timeArg, "%d", terminationTime);

				if(DEBUG == true){
				cout << "Master processX: " << *processX << endl;
				cout << "Master iterationArg: " << *iterationArg << endl;
				cout << "Master timeArg: " << *timeArg << endl;
				}

                                char *slaveArgs[] = {"./slave", "-i", iterationArg, "-s", processX, "-t", timeArg, NULL};
                                const char execute[10] = "./slave";


                                int status = execvp(execute,slaveArgs);

                                if(status < 0)
                                {
                                        perror("execvp");
                                        string message = "Execvp failed for ";
                                        message.append((const char*) slave);
                                        message.append("\n");
                                        logMessage(standard,message);
                                        return -1;
                                }

			}
			
		}

		free(iterationArg);
		free(processX);
		free(timeArg);
       
		//Wait for each Slave to finish
		for(x = 1; x <= numberSlaves; x++) {
			slave = wait(&returnStatus);
		        fprintf(stderr, "Master: child %d has been killed!\n", slave);
		        fprintf(stderr, "Master: %d/%d children have been killed!\n", x, numberSlaves);
		}

		if(shmctl(shmid, IPC_RMID, buffer) < 0){
        		perror("shmctl");
       	 		exit(1);
        	}
 
		return 0;    

}

            
   

       
