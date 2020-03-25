/**********************************************************
* Author: Peng Zhang
* CS 344 Program 3 smallsh
* Due date: 11/20/2019
***********************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

int TSTP_toggle = 1; // global variable to toggle, work for sending TSTP signal.

//function to send a SIGTSTP signal, determined by bool toggle
void SSIG_TSTP(int signo){
	if (TSTP_toggle == 1){
		char* message = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 49);
		TSTP_toggle=0;
	}
	else{
		char* message = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 29);
		TSTP_toggle=1;
	}


}
//the funtion is to do fork and exectue
void forkexcu(char *argument[], char fileInput[], char fileOutput[], int *bg_process, int *childExitMethod){
			// fork to do more process
			// some code cited from lecture 3.1
			// redirecting /file operation is cited leture 3.4
			// citation: https://oregonstate.instructure.com/courses/1738955/pages/block-3
			int inputfile, outputfile;//check if the file is avaliable
			int ck_dup2;//redirection checking
			pid_t spawnPid = -5;
			spawnPid = fork();
			switch (spawnPid){
				case -1:{perror("Hull Breach!\n"); exit(1);break;}
				case 0: {	
					if(strcmp(fileInput, "") != 0){
						inputfile = open(fileInput, O_RDONLY); //open the file
						ck_dup2 = dup2(inputfile, 0); //check the redirection
						if (inputfile == -1) {
							printf("cannot open %s for input\n", fileInput);
							fflush(stdout);
							exit(1);
						}
						if(ck_dup2 == -1){
							perror("cannot do dup2\n");
							exit(1);
						}
						fcntl(inputfile, F_SETFD, FD_CLOEXEC); //close file 
					}
					if(strcmp(fileOutput, "") != 0){
						outputfile = open(fileOutput, O_WRONLY | O_CREAT | O_TRUNC, 0644);//open the output file
						ck_dup2 = dup2(outputfile, 1); //check the redirection
						if (outputfile == -1) {
							printf("cannot open %s for output\n", fileOutput);
							fflush(stdout);
							exit(1);
						}
						if(ck_dup2 == -1){
							perror("cannot do dup2\n");
							exit(1);
						}
						fcntl(outputfile, F_SETFD, FD_CLOEXEC); //close file 
					}
					if(execvp(argument[0], argument)){ // if no file or dire found
                   		printf("%s: no such file or directory\n", argument[0]);
                   		fflush(stdout);
                    	exit(1);
               		}
               		break;
               	}
               	default:{
               		pid_t actualPid;
               		if (TSTP_toggle == 1 && *bg_process == 1) {
						pid_t actualPid;
						actualPid=waitpid(spawnPid, childExitMethod, WNOHANG);
						printf("background pid is %d\n", spawnPid);
						fflush(stdout);
					}
			
					else {
						actualPid = waitpid(spawnPid, childExitMethod, 0);
					}
				}
				while ((spawnPid = waitpid(-1, childExitMethod, WNOHANG)) > 0) {
					//print the status
					if (WIFEXITED(*childExitMethod)) {
						int exitStatus =  WEXITSTATUS(*childExitMethod);
						printf("exit value %d\n", exitStatus);
					} 
					else {
						int exitSignal = WTERMSIG(*childExitMethod);
						printf("terminated by signal %d\n", exitSignal);
					}
					fflush(stdout);	
				}
			}
}

// main funciton to get input from user and determine what the command is.
int main(){
	int pid = getpid(); //get process id
	char *token; //split the coomand and store in it.
	int bg_process = 0; //check if background process
	char fileInput[256] =""; //input file
	char fileOutput[256] =""; //output file
	char command[2048]; // command is limit 2048 characters
	char *argument[512]; // argument is limit in 512 characters
	int childExitMethod = 0; //exit status
	// initinalize the file in/output
	
	//signal to ignore ctrl + c
	//some code is cited from lecture 3.3
	//citation: https://oregonstate.instructure.com/courses/1738955/pages/block-3
	struct sigaction sig_int = {0};
	struct sigaction sig_ts = {0};
	sig_int.sa_handler = SIG_IGN;
	sigfillset(&sig_int.sa_mask);
	sig_int.sa_flags = 0;
	sigaction(SIGINT, &sig_int, NULL);

	//send TSTP signal if receive ctrl + z, citation is same as above.
	sig_ts.sa_handler = SSIG_TSTP; //call SSIG_TSTP(int)
	sigfillset(&sig_ts.sa_mask);
	sig_ts.sa_flags = 0;
	sigaction(SIGTSTP, &sig_ts, NULL);
	int i;
	int j;
	// while loop to get input
	while(1){
		printf(": ");
		fflush(stdout); //ignore useless output
		fgets(command, 2048, stdin); // get input from user
		int checkN = 1;//boolean to check if has \n
		int countN = 0; // record how how many chars it has checked
		int count = 0; //count the number of argument

		//check if the command has \n, if has, change it to NULL
		while(checkN){ 
			if (command[countN] == '\n') {
				command[countN] = '\0'; // use null value to take place \n.
				checkN = 0; // end the loop if take place done
			}
			if(countN == 2048){ // if all size if command has no found, break
				checkN = 0; // end if no \n here
			}
			countN++;
		}
		if(strcmp(command, "")==0){
			//check if the command is blank, if so, repeat ask for input. (continue loop)
			continue;
		}

		token = strtok(command, " "); //split the command by space " "
		while(token != NULL){ // check the token until it is empty
		
			if(strcmp(token,"&") == 0){
				bg_process = 1; //check if command is & to be background process 
				token = strtok(NULL, " "); // go to next chars
			}
			else if(strcmp(token, "<") == 0){ // if input <, get input file
				token = strtok(NULL, " "); // go to next chars
				strcpy(fileInput, token);
				token = strtok(NULL, " "); // go to next chars
			}
			else if(strcmp(token, ">") == 0){ // if inout >, get output file
				token = strtok(NULL, " "); // go to next chars
				strcpy(fileOutput, token);
				token = strtok(NULL, " "); // go to next chars
			}
			else{
				argument[count] = strdup(token); //store the left command to argument	
				//check if the argument has $$ to get pid
				int count2 = 0; //position of argument
				while (argument[count][count2] != '\0'){ //whille the argument has data, do
					if(argument[count][count2] == '$' && argument[count][count2 + 1] == '$'){
						argument[count][count2] == '\0';
						snprintf(argument[count], 512, "%s%d", argument[count], pid);
					}
					count2++;		
				}	
			    token = strtok(NULL, " "); // go to next chars
			}
			count++;
		}
		if (argument[0][0] == '#') { // if the argument is #, then it is comment, continue loop
			continue;
		}
		else if(strcmp(argument[0], "exit") == 0){//if input exit, exit program
			exit(0);
			
		}
		else if(strcmp(argument[0], "cd") == 0){ // if input cd
			if (argument[1]){ //if has input, go that dire
				chdir(argument[1]);
			}
			else{
				chdir(getenv("HOME")); //if no directory entered, go home dire.
			}
		}
		else if(strcmp(argument[0], "status") == 0){
			//check the exit status
			//some code cited from leture 3.1
			// citation: https://oregonstate.instructure.com/courses/1738955/pages/block-3
			if (WIFEXITED(childExitMethod)) {
				int exitStatus =  WEXITSTATUS(childExitMethod);
				printf("exit value %d\n", exitStatus);
			} 
			else {
				int exitSignal = WTERMSIG(childExitMethod);
				printf("terminated by signal %d\n", exitSignal);
			}
		}
		else{
			//call forkexcu to do fork and execute
			forkexcu(argument, fileInput, fileOutput, &bg_process, &childExitMethod);
	
		}
		//clear all data of the variable.
		fileInput[0] = '\0';
		fileOutput[0] = '\0';
		for (i=0; argument[i]; i++) {
			argument[i] = NULL;
		}
		bg_process = 0;
		
	}
	return 0;
}