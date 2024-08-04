#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/signal.h> 

char allCommands[7][5][50];
int numberOfCommands;
int numberOfArguments[7];
int doubleArrow;
char andOrSequence[5][4];
int numberOfConditions;
int maxNumberOfCommands;
int expectedNumberOfCommands;
int moreNumOfArguments;
int onlyspace=0;
int condition[6];
int backgroundprocessIds[50];
int currentId=-1;
int child_pid;
char *root_dir;



int checktoken(char* token)  // returns 1 if token contains other than space, else 0
{
    for(int i=0;i<strlen(token);i++) // iterates for every character in string
    {
        if(token[i]!= ' ')  //if character is other than space
        {
            return 1;
        }
    }
    return 0;
}

void storecommand(char* command,int i) // stores the command in allCommands array
{
    char *token;
    token = strtok(command, " ");  // tokenization based on space 
    int argumentCount=0;
    while (token != NULL) {
        
        if(argumentCount>=5) //if argument count > 5 then it returns to print an error
        {
            moreNumOfArguments=1; // sets the flag
            return;
        }
        
        if(strstr(token,"~")!=NULL)  // if the token contains ~ symbol, if changes into home directory
        {
            if(token[0]=='~')
            {
                token++; //removing the ~ symbol
                char *token1=malloc(50);
                strcpy(token1,root_dir); //root directory contains home directory path
                strcat(token1,token);  //adding the token to home directory path
                strcpy(allCommands[i][argumentCount],token1); // storing the command in allCommands array
            }
        }
        else
        {
            strcpy(allCommands[i][argumentCount],token); // storing the command in allCommands array
            
        }
        argumentCount++; // increasing the argument count
        token = strtok(NULL, " "); // taking next token
    }
    numberOfArguments[i]=argumentCount; // storing arguments counts for specified command
}



void tokenizecommands(char *a,char* delim)
{
    char *token;
    numberOfCommands=0;
    char *saveptr;
    token = strtok_r(a, delim, &saveptr);  // tokenizing the string based on delimiter
    while (token != NULL) { 
        if(checktoken(token)==0) //if token contains only spaces then returns
        {
            onlyspace=1; // turning onlyspace flag on
            return; // return
        }
        storecommand(token,numberOfCommands); //storing the command
        if(moreNumOfArguments==1) // if moreNumOfArguments flag is set then token contains more arguments so it is an error
        {
            return;
        }
        numberOfCommands++; // incrementing numberOfCommands
        token = strtok_r(NULL, delim,&saveptr); // next token
       
    }
}

void tokenize_double_or(char *a) // tokenizing the string based on ||
{
    char *token;
    char *saveptr;
    token = strtok_r(a, "||", &saveptr); // tokenizing the string based on delimiter
    while (token != NULL) {
        if(checktoken(token)==0) // if token contains only space then it returns
        {
            onlyspace=1;  // onlyspace flag is set
            return;  //returns because it is an error
        }
        storecommand(token,numberOfCommands); //storing the command
        numberOfCommands++;  //incrementing the numberOfCommands
        token = strtok_r(NULL, "||",&saveptr);  // next token
        if(token!=NULL)
        {
            condition[numberOfConditions]=0; // storing the condition 1 for && and 0 for ||
            numberOfConditions++;  // incrementing numberOfConditions
        }
    }
}


void tokenize_double_and(char *a,char* delim) // tokenizing the string based on &&
{
    char *token;
    numberOfCommands=0; 
    numberOfConditions=0;
    char *saveptr;
    token = strtok_r(a, delim, &saveptr);  // tokenizing the string based on delimiter
    while (token != NULL) {
        if(strstr(token,"||")!=NULL) // if the token contains || then token is tokenized based on || 
        {
            tokenize_double_or(token); // calling tokenize_double_or function
            if(onlyspace==1) // if token contains only space then it returns
            {
                return;
            }
        }
        else
        {
            if(checktoken(token)==0) // if token contains only space then it returns
            {
                onlyspace=1; // onlyspace flag is set
                return; //returns because it is an error
            }
            storecommand(token,numberOfCommands);  //storing the command
            numberOfCommands++;    //incrementing the numberOfCommands
        }
        token = strtok_r(NULL, delim,&saveptr); // next token
        if(token!=NULL)
        {
            condition[numberOfConditions]=1; // storing the condition 1 for && and 0 for ||
            numberOfConditions++; // incrementing numberOfConditions
        }
        
    }
}

void execute_single_command(int i) { // executing single command
   
    char *args[numberOfArguments[i] + 1];  //creating args char array
    for (int j = 0; j < numberOfArguments[i] && allCommands[i][j][0] != '\0'; j++) {
        args[j] = allCommands[i][j]; // loading arguments into args
    }
    args[numberOfArguments[i]] = NULL;  // storing null in last argument
    if(execvp(args[0], args)==-1)  // exectuing the command
    {
        perror("execvp");
        exit(1);
    }
}

void execute_all_commands() {    // executing pipe command
    int fd[numberOfCommands - 1][2]; // File descriptors for pipes

    for (int i = 0; i < numberOfCommands; i++) {
        if (i < numberOfCommands - 1 && pipe(fd[i]) == -1) {  //creating pipe for every iteration
            perror("pipe");
            exit(1);
        }

        pid_t pid = fork();  // creating child process
        if (pid == -1) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) { // Child process
            // Redirect input/output for the first command
            if (i == 0) {
                close(fd[0][0]); // Close read end of the first pipe
                dup2(fd[0][1], STDOUT_FILENO); // Redirect stdout to the write end of the first pipe
                close(fd[0][1]); // Close the write end of the first pipe
                execute_single_command(i); // executing the first command
            }
            // Redirect input/output for the last command
            else if (i == numberOfCommands - 1) {
                close(fd[i - 1][1]); // Close write end of the previous pipe
                dup2(fd[i - 1][0], STDIN_FILENO); // Redirect stdin to the read end of the previous pipe
                close(fd[i - 1][0]); // Close read end of the previous pipe
                execute_single_command(i); // executing the commands which are not first and last
            }
            // Redirect input/output for intermediate commands
            else {
                close(fd[i - 1][1]); // Close write end of the previous pipe
                dup2(fd[i - 1][0], STDIN_FILENO); // Redirect stdin to the read end of the previous pipe
                close(fd[i - 1][0]); // Close read end of the previous pipe
                close(fd[i][0]); // Close read end of the current pipe
                dup2(fd[i][1], STDOUT_FILENO); // Redirect stdout to the write end of the current pipe
                close(fd[i][1]); // Close the write end of the current pipe
                execute_single_command(i); // executing the last command
            }
        } else { // Parent process
            if (i > 0) {
                close(fd[i - 1][0]); // Close read end of the previous pipe
                close(fd[i - 1][1]); // Close write end of the previous pipe
            }
        }
    }

    // Close any remaining pipe file descriptors in the parent process
    for (int i = 0; i < numberOfCommands - 1; i++) {
        close(fd[i][0]);
        close(fd[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < numberOfCommands; i++) {
        wait(NULL);
    }
}


void execute_concat()  // executing concatenation
{
    char *args[numberOfCommands + 2];  // creating args char array
    args[0] = "cat"; // storing cat as first argument
    for(int i = 0; i <= numberOfCommands && allCommands[i][0][0] != '\0'; i++) {
        args[i+1] = allCommands[i][0];   // loading arguments into args
    }
    args[numberOfCommands+1] = NULL;  // storing null as last argument
    if(fork()==0)  // creating child process
    {
        execvp(args[0], args); //executing the cat command
        perror("execvp");
    }
    else{
         wait(NULL);
    }
}

void execute_arrows() // executing >> and > options
{
    char *args[numberOfArguments[0] + 1];   // creating args char array
    for (int j = 0; j < numberOfArguments[0] && allCommands[0][j][0] != '\0'; j++) {
        args[j] = allCommands[0][j]; // loading arguments into args
    }
   
    args[numberOfArguments[0]] = NULL;  // storing null as last argument
    char* filename=allCommands[1][0];  // storing filename from allCommands
    if(strstr(filename,".")==NULL)
    {
        printf("error in filename \n");
        return;
    }
    int fd;

    if(doubleArrow==1) //if special character is >>
    {
        fd=open(filename,O_CREAT | O_APPEND | O_WRONLY, 0777);  //creating a file with O_APPEND option
    }
    else //if special character is >>
    {
        remove(filename); // removing file if exists 
        fd=open(filename,O_CREAT | O_WRONLY, 0777); //creating a file with O_WRONLY option
    }

    if(fork()==0) // creating child process
    {
        dup2(fd,STDOUT_FILENO);  // redirecting standard output to file created
        close(fd); // closing fd
        execvp(args[0], args); //executing the command and output will be stored in the file
        perror("execvp");
    }
    else{
         wait(NULL); // waits for the child process to complete
    }
    close(fd); //closing fd
}


void execute_reversearrow()   //executing the special character <
{
    char *args[numberOfArguments[0] + 1];   // creating args char array
    for (int j = 0; j < numberOfArguments[0] && allCommands[0][j][0] != '\0'; j++) {
        args[j] = allCommands[0][j]; // loading arguments into args
    }
   
    args[numberOfArguments[0]] = NULL;  // storing null as last argument
    char* filename=allCommands[1][0]; // storing file name
    int fd=open(filename,O_RDONLY);  // opening file name in read only mode
    if(fd==-1) // if file doesn't exist
    {
        printf("file name doesn't exist\n");
        return;
    }

   
    
    if(fork()==0) // creating child process
    {
        dup2(fd,STDIN_FILENO); //redirecting the standard input from the file
        close(fd); //closing fd
        execvp(args[0], args);  // executing the command
        perror("execvp");
    }
    else{
         wait(NULL); // waits for the child process to complete
         allCommands[0][0][0]='\0'; //removing the data in allCommands
         allCommands[1][0][0]='\0'; //removing the data in allCommands
    }
    close(fd);
}


void execute_conditions()  // executing the conditional special characters && and || 
{
    int isSuccess=0;
    for (int i = 0; i < numberOfCommands; i++) 
    {
        pid_t pid = fork();  // creates the process
        if (pid == -1) {
            perror("fork");
            exit(1);
        }
        else if(pid==0)
        {
            execute_single_command(i);  //executes the command
        }
        else
        {
            int status;
            int k=wait(&status);  // waits for the child
            if (WIFEXITED(status))
            {
                if( WEXITSTATUS(status)==0)  //if command is executed successfully
                {
                    isSuccess=1;  //isSuccess flag is set to 1
                }
                else{
                    isSuccess=0; //if command execution is failed then isSuccess flag is set to 0
                }
            }

            while(i<numberOfConditions)
            {
                if(condition[i]==1 && isSuccess==0) //if condition is && and command is failed
                {
                    i++; //skipping next command
                }
                else if(condition[i]==0 && isSuccess==1) //if condition is || and command is successfull
                {
                    i++; //skipping next command
                }
                else
                {
                    break;
                }
            }
        }
    }
}

void execute_sequential() {
   

   for(int i=0;i<numberOfCommands;i++) //iterating loop for every command
   {
        if(fork()==0)  //creating child process
        { 
            execute_single_command(i);  //executing the command
        }
        else
        {
            wait(NULL);  //waiting for the child
        }     
   }
    
}

int checkerrorincommands()  // check if there is any error in the commands
{
    // if any of the following conditions are passsed then it will print an error
    //if any of the command contains only space it will get an error
    if(onlyspace==1 || numberOfCommands!=expectedNumberOfCommands || maxNumberOfCommands<numberOfCommands || moreNumOfArguments==1)
    {
        onlyspace=0;
        moreNumOfArguments=0;
        printf("please enter 1 to 5 number of arguments in each command (or) this code doesn't handle more than %d commands \n",maxNumberOfCommands);
        return 1;
    }
    return 0;
}

int count_characters(char *str,char delim)  //counts number of characters or delimiters in the string
{  
    int count = 0;
    for (int i = 0; i<strlen(str); i++) {
        if (str[i] == delim) {  // if current character is delimiter than it increments the count
            count++;
        }
    }
    return count; //returns the count
}

int count_and_and(const char *str) { //counts number of && and || symbols in the string
    int count = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '&' && str[i + 1] == '&') {
            count++;// Skip the second '&' to avoid double counting
            
            i++;
        }
        else if(str[i] == '|' && str[i + 1] == '|')
        {
            count++;// Skip the second '|' to avoid double counting
            i++;
        }
    }
    return count; //returns the count
}


void execute_newt(char* a)
{
    char b[strlen(a)];
    int j=0;
    for(int i=0;i<strlen(a);i++)  // trimming the command
    {
       if(a[i]!=' ')  //ignores the spaces before the command
        {
            b[j]=a[i];
            j++;
        }
    }
    b[j]='\0';
    if(strcmp(b,"newt")==0)  //if command is newt
    {
        int pid = fork();
	    if(pid == 0) {
		//execlp("xterm", "xterm", "-e", "./sample",  NULL);
        execlp("x-terminal-emulator", "x-terminal-emulator", "-e", "./shell24", NULL);  // it will create new terminal and executes this program in it
	    }
    }
    else
    {
        printf("enter command \"newt\" if you want to open new terminal\n");
    }
}



void execute_background(char* a)
{
    a[strlen(a)-1]='\0'; //removes the & character at the end
    if(strstr(a,"&")!=NULL)
    {
        printf("error \n");
    }
    else
    {
        storecommand(a,0); //stores the command 
        numberOfCommands=1;
        pid_t pid = fork(); //creates the process
        if (pid == -1) {
            perror("fork");
            exit(1);
        }
        else if(pid==0)
        {
            setpgid(0, 0);  // changes the pgid so that process executes in the background
            execute_single_command(0);  //executes the command
        }
        else
        {
            printf("pushing process id - %d to background \n",pid);
            currentId++;  // increments the currentId
            backgroundprocessIds[currentId]=pid; // stores the child pid into the array
        }
    }
}


void handlerinput(int signo)
{ 
    if ( tcsetpgrp(0, getpid()) == -1) {  // brings the process id into the foregorund
        perror("tcsetpgrp");
        exit(1);
    }
}

void handleroutput(int signo)
{ 
    if ( tcsetpgrp(1, getpid()) == -1) { // brings the process id into the foregorund
        perror("tcsetpgrp");
        exit(1);
    }
}

void execute_fg()
{
    int status;
    if(currentId==-1)  //if current id ==-1 then no process is in the background
    {
        printf("no background process to bring to foreground\n");
        return;
    }
    printf("bringing process id - %d into foreground\n",backgroundprocessIds[currentId]);
    if (tcsetpgrp(0, backgroundprocessIds[currentId]) == -1) { // brings the process id into the foregorund
        perror("tcsetpgrp"); 
        exit(1);
    }

    // Wait for the foreground process to complete
    waitpid(backgroundprocessIds[currentId--], &status, WUNTRACED);  //wait for the child to execute
}

int checkreversearrow(char *a)
{
    int i = 0;

    // Skip leading spaces
    while (a[i] == ' ') {
        i++;
    }

    // Check if the first non-space character is <
    if (a[i] == '<') {
        return 0;
    } else {
        return 1;
    }
}

int main()
{
    char a[100];
    char *token;
    signal(SIGTTIN , handlerinput); //handler for  SIGTTIN
    signal(SIGTTOU , handleroutput); //handler for  SIGTTOU
    root_dir=getenv("HOME");  // stores the home directory into root_dir
    while(1)
    {
        printf("shell24$ "); // prints shell24$
        fgets(a,sizeof(a),stdin); //takes the input
        
        size_t len = strlen(a);
        if (len > 0 && a[len - 1] == '\n') // if input contains new line at the end .. it will replaced by null
            a[len - 1] = '\0';

        if(strstr(a,"newt")!=NULL)  // if the input is newt 
        {
            execute_newt(a); 
        }
        else if(strstr(a,"fg")!=NULL) //if the input is fg
        {
            execute_fg();
            
        }
        else if(strstr(a,"&&")!=NULL  || strstr(a,"||")!=NULL)  //if the input contains && or ||
        {
            //printf("start onlyspace = %d \n",onlyspace);
            maxNumberOfCommands=6;
            expectedNumberOfCommands=count_and_and(a)+1;
            tokenize_double_and(a,"&&");  //tokenize the command and store it in the array
            if(checkerrorincommands() == 1)  //checks the errors in the command
            {
                continue;
            }
            else
            {
                execute_conditions(); //execute the function
            }
            
        }
        else if(strstr(a,"|")!=NULL)
        {
            maxNumberOfCommands=7;
            expectedNumberOfCommands=count_characters(a,'|')+1;
            tokenizecommands(a,"|"); //tokenize the command and store it in the array
            if(checkerrorincommands() == 1) //checks the errors in the command
            {
                continue;
            }
            else
            {
                execute_all_commands(); //execute the function
            }        
        }
        else if(a[strlen(a)-1]=='&')
        {
            execute_background(a); //execute the function
        }
        else if(strstr(a,"#")!=NULL)
        {
            maxNumberOfCommands=6;
            expectedNumberOfCommands=count_characters(a,'#')+1;
            tokenizecommands(a,"#");  //tokenize the command and store it in the array
            if(checkerrorincommands() == 1) //checks the errors in the command
            {
                continue;
            }
            else
            {
                execute_concat(); //execute the function
            }
        }
        else if(strstr(a,">>")!=NULL)
        {
            tokenizecommands(a,">>"); //tokenize the command and store it in the array
            doubleArrow=1;
            execute_arrows(); //execute the function
        }
        else if(strstr(a,">")!=NULL)
        {
            tokenizecommands(a,">"); //tokenize the command and store it in the array
            doubleArrow=0;
            execute_arrows(); //execute the function
        }
        else if(strstr(a,"<")!=NULL)
        {
 
            if(checkreversearrow(a)==0) //checks for any errors in the command
            {
                printf("error in command\n");
                continue;
            }
          
            tokenizecommands(a,"<"); //tokenize the command and store it in the array
            execute_reversearrow(); //execute the function
        }
        else if(strstr(a,";")!=NULL)
        {
            maxNumberOfCommands=6;
            expectedNumberOfCommands=count_characters(a,';')+1;
            tokenizecommands(a,";");  //tokenize the command and store it in the array
            if(checkerrorincommands() == 1) //checks the errors in the command
            {
                continue;
            }
            else
            {
                execute_sequential(); //execute the function
            }
        }
        else
        {
            storecommand(a,0);  //stores the command
            numberOfCommands=0; 
            expectedNumberOfCommands=0;
            maxNumberOfCommands=1; 
            if(checkerrorincommands() == 1)  //checks the errors in the command
            {
                continue; //if command contains error then skips next steps
            }
            else
            {
                if(fork()==0)
                {
                    execute_single_command(0); //execute the function
                } 
                else
                {
                    wait(NULL);  //wait the child id to complete
                }
            }
            
        }


    }
}
