#define _XOPEN_SOURCE 500  // Required for FTW_DEPTH on some systems
#include <stdio.h>
#include <stdlib.h> 
#include<string.h>
#include <unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/signal.h>
#include <ctype.h>

const char *process_id=NULL;  
const char *root_process=NULL;
const char *option=NULL;
int found=0;
int hasnondirectDescendatants=0;
int hasGrandChildren=0;
int defunctChilds=0;


long getFileSize(const char *filename) { //it will return file size of given file 
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    } else {
        perror("Error getting file size");
        return -1;
    }
}


int isValidNumber(char *str) {  // if will return whether passed string is valid number of not
    int i;
    const int len = strlen(str);

    for (i = 0; i < len; i++)
    {
        if (!isdigit(str[i])) 
        {
            return 0;
        }
    }
    return 1;
}
void nonDirectDescendants(const char* parent_id,int level)  // it will print non direct descendants of parent
{
    level++;
    char command[50];
    snprintf(command, sizeof(command), "cat /proc/%s/task/%s/children", parent_id, parent_id); // it will create the command and store it in command variable
    char* childProcessIds=malloc(128);  // it will create the space of the pointer
    FILE *fp = popen(command, "r");  // it will execute the command and store the output in a file and return the filepointer
    fgets(childProcessIds, 128, fp ); // it will read the content of file and store it in chilProcessIds buffer
    char *saveptr1;
    char *child_process_id = strtok_r(childProcessIds, " ",&saveptr1); // it will tokenize the buffer using demiliter as space
    while(child_process_id != NULL && isValidNumber(child_process_id)==1) // iterate for each child id
    {
        if(level>1)  // if level ==1 then it is direct child and leven ==2 is grand child . to print non direct descandants I used level > 1
        {
            hasnondirectDescendatants=1; // it acts as a flag
            printf("%s\n",child_process_id); //prints the child id
        }
        nonDirectDescendants(child_process_id,level);  // it recursively call the function to iterate for every child in the root proces tree
        if ((child_process_id = strtok_r(NULL," ",&saveptr1)) == NULL ) {  // it take the next token as child id
            return;
        }
    }
}

void siblingIds(const char *parent_id,const char *child_id)  // prints the siblings of process id( child id)
{
    char command[50];
    int hasSibling=0;
    snprintf(command, sizeof(command), "cat /proc/%s/task/%s/children", parent_id, parent_id); // it will create the command and store it in command variable
    char childProcessIds[128]; // it will create the space of the pointer
    FILE *fp = popen(command, "r"); // it will execute the command and store the output in a file and return the filepointer
    fgets(childProcessIds, sizeof(childProcessIds), fp );  // it will read the content of file and store it in chilProcessIds buffer
    char *child_process_id = strtok(childProcessIds, " "); // it will tokenize the buffer using demiliter as space
    while(child_process_id != NULL && isValidNumber(child_process_id)==1) // iterate for each child id
    {
        if(strcmp(child_process_id,child_id)!=0) // it will compare each child id with pass process id and prints if both are not equal
        {
            hasSibling=1; // it acts as a flag
            printf("%s\n",child_process_id); //prints the siblingid
        }
        if ((child_process_id = strtok(NULL, " ")) == NULL) { // it take the next token as child id
                break;
        }
        
    }
    if(hasSibling==0)  //if process id has no siblings prints no siblings
    {
        printf("No sibling/s\n");
    }
}

void immediateDescendants(const char *parent_id)  // prints the immediate descendants of the process id
{
    char command[50];
    snprintf(command, sizeof(command), "cat /proc/%s/task/%s/children", parent_id, parent_id); // it will create the command and store it in command variable
    char childProcessIds[128]; // it will create the space of the pointer
    FILE *fp = popen(command, "r"); // it will execute the command and store the output in a file and return the filepointer
    fgets(childProcessIds, sizeof(childProcessIds), fp ); // it will read the content of file and store it in chilProcessIds buffer
    char *child_process_id = strtok(childProcessIds, " ");  // it will tokenize the buffer using demiliter as space
    if(isValidNumber(child_process_id)==0)
    {
        printf("No direct Descendants\n");
        return;
    }
    while(child_process_id != NULL && isValidNumber(child_process_id)==1)
    {
        printf("%s\n",child_process_id);
        if ((child_process_id = strtok(NULL, " ")) == NULL) {
            break;
        }
    }
}

void grandChildren(const char* parent_id,int level)
{
    level++;
    if(level>2)
    {
        return;
    }
    char command[50];
    snprintf(command, sizeof(command), "cat /proc/%s/task/%s/children", parent_id, parent_id); // it will create the command and store it in command variable
    //printf("command= %s\n",command);
    char *childProcessIds=malloc(128); // it will create the space of the pointer
    FILE *fp = popen(command, "r"); // it will execute the command and store the output in a file and return the filepointer
     if (fp == NULL) {
        perror("Error opening file");
        return;
    }
    fgets(childProcessIds, 128, fp ); // it will read the content of file and store it in chilProcessIds buffer
    //printf("Descendants immediate child of parent id = %s is child ids = %s\n",parent_id,childProcessIds);
    char *saveptr1;
    char *child_process_id = strtok_r(childProcessIds, " ",&saveptr1);    // it will tokenize the buffer using demiliter as space
    if(child_process_id==NULL || (parent_id,child_process_id)==0)
    {
        return;
    }
    while(child_process_id != NULL && isValidNumber(child_process_id)==1)
    {
        if(level==2)
        {
            hasGrandChildren=1;
            printf("%s\n",child_process_id);
        }
        
        grandChildren(child_process_id,level);
        if ((child_process_id = strtok_r(NULL," ",&saveptr1)) == NULL ) {
            return;
        }
    }
}

void killprocessid(const char* child_id)
{
    int i=kill(atoi(child_id),SIGKILL);
    if(i==0)
    {
        printf("kill signal is sent to process id %s successfully\n",child_id);
    }
    else{
        printf("error in sending a kill signal to %s id\n",child_id);
    }
}


int isDefunctProcessId(const char* child_id)
{
    char command[100];
    snprintf(command, sizeof(command), "cat /proc/%s/status | grep State | awk '{print $2}'", child_id); // it will create the command and store it in command variable
    int len=strlen(child_id);
    char file[128];
    FILE *fp = popen(command, "r"); // it will execute the command and store the output in a file and return the filepointer
    fgets(file,2, fp );
    //printf("file = %s\n",file);
    if(strcmp(file,"Z")==0)
    {
        return 1;
    }
    else{
        return 0;
    }
}

void defunctChildIds(const char *parent_id)
{
    char command[50];
    int i=0;
    snprintf(command, sizeof(command), "cat /proc/%s/task/%s/children", parent_id, parent_id); // it will create the command and store it in command variable
    char childProcessIds[128]; // it will create the space of the pointer
    FILE *fp = popen(command, "r"); // it will execute the command and store the output in a file and return the filepointer
    fgets(childProcessIds, sizeof(childProcessIds), fp ); // it will read the content of file and store it in chilProcessIds buffer
    //printf("child of parent id = %s is child ids = %s\n",parent_id,childProcessIds);
    char *saveptr1;
    char *child_process_id = strtok_r(childProcessIds, " ",&saveptr1);  // it will tokenize the buffer using demiliter as space
    if(child_process_id==NULL || (parent_id,child_process_id)==0)
    {
        return;
    }
    while(child_process_id!= NULL && isValidNumber(child_process_id)==1) {
        i=isDefunctProcessId(child_process_id);
        if(i==1)
        {
            defunctChilds=1;
            printf("%s\n",child_process_id);
        }
        else{
            defunctChildIds(child_process_id);
        }
        if ((child_process_id = strtok_r(NULL," ",&saveptr1)) == NULL) {
            break;
        }
    }
}




int childExists(const char* child_id)
{
    int fd1=open("temp.txt",O_RDONLY);
    char buff[50];
    long n=read(fd1,buff,getFileSize("temp.txt"));
    char* id=strtok(buff," ");
    while(id!=NULL)
    {
        if(strcmp(id,child_id)==0)
        {
            return 0;
        }
        if ((id= strtok(NULL, " ")) == NULL) {
                break;
        }
    }
    return -1;
}



void storeInFile(const char* child_id) // stores the id in the file
{
    int fd1=open("temp.txt",O_CREAT|O_RDWR|O_APPEND,0777);
    long n=write(fd1,child_id,strlen(child_id));
    n=write(fd1," ",1);
    close(fd1);
}
int fileExists(const char* file) //checks whether file exists or not
{
    int fd1=open("temp.txt",O_RDONLY);
    return fd1;
}

int pauseProcess(const char* child_id)  // stop the process
{
    int i=kill(atoi(child_id),SIGSTOP); // sends SIGSTOP signal to child id 
    if(i==0)
    {
        printf("SIGSTOP signal is sent to %s process successfully\n",child_id);
        if(fileExists("temp.txt")>0 && childExists(child_id)!=0) // if child id doesn't exist in file then it stores the id in the file
        {
            storeInFile(child_id);
        } 
        else if(fileExists("temp.txt")<0) // if file doesn't exist  then it  creates and stores the id in the file
        {
            storeInFile(child_id);
        }
    }
    else{
        printf("error in stopping %s process \n",child_id);
    }
}

int isValidProcess(const char *process_id) //returns process is valid or not
{
    if (kill(atoi(process_id), 0) == 0) {
        return 1;
    } else {
        return 0;
    }
}

void continueAllProcesses()
{
    if(fileExists("temp.txt")<0) //if files doesn't exist then prints error
    {
        printf("No processes are paused previously by SIGSTOP\n");
        return;
    }
    int fd1=open("temp.txt",O_RDONLY); //open the file
    long len=getFileSize("temp.txt");
    char buff[len];
    long n=read(fd1,buff,len); //read the content of the file
    buff[len-1]='\0';
    //printf("buff= %s\n",buff);
    char* id=strtok(buff," "); // tokenize the string to iterate each child
    while(id!=NULL)
    {
        int i=kill(atoi(id),SIGCONT); // sends SIGCONT signal to each child id
        if(i==0)  //if successful prints success or error
        {
            printf("%s process resumed successfully\n",id);
        }
        else{ 
            printf("error in resuming %s process or process doesn't exist \n",id);
        }
        if ((id= strtok(NULL, " ")) == NULL) { //takes next child id
                break;
        }
    }
    remove("temp.txt");
}



void options(const char *parent_id,const char *child_id)
{
    if(strcmp(option,"-xn")==0)  //if option is -xn calls nonDirectDescendants function
    {
        nonDirectDescendants(child_id,0);
        if(hasnondirectDescendatants==0)
        {
            printf("No non-direct descendants\n");
        }
    }
    else if(strcmp(option,"-xd")==0) //if option is -xd calls immediateDescendants function
    {
        immediateDescendants(child_id);
    }
    else if(strcmp(option,"-xs")==0)  //if option is -xs calls siblingIds function
    {
        siblingIds(parent_id,child_id);
    } 
    else if(strcmp(option,"-xg")==0) //if option is -xg calls grandChildren function
    {
        grandChildren(child_id,0);
        if(hasGrandChildren==0)
        {
            printf("No grand children\n");
        }
    }
    else if(strcmp(option,"-rp")==0)  //if option is -rp calls killprocessid function
    {
        killprocessid(child_id);
    }
    else if(strcmp(option,"-zs")==0) //if option is -zs calls isDefunctProcessId function
    {
        int i=isDefunctProcessId(child_id);
        if(i==1)
        {
            printf("Defunct\n");
        }
        else{
            printf("Not Defunct\n");
        }
    }
    else if(strcmp(option,"-xz")==0) //if option is -xz calls defunctChildIds function
    {
        defunctChildIds(child_id);
        if(defunctChilds==0)
        {
            printf("No descendant zombie process/es \n");
        }
    }
    else if(strcmp(option,"-xt")==0)  //if option is -xt calls pauseProcess function
    {
        pauseProcess(child_id);
    }
    else if(strcmp(option,"-xc")==0)  //if option is -xc calls continueAllProcesses function
    {
        continueAllProcesses();
    }
    else{
        printf("enter valid option as argument\n");
    }
}



void search(const char *parent_id,const char *child_id)
{
    
    char command[50];
    snprintf(command, sizeof(command), "cat /proc/%s/task/%s/children", parent_id, parent_id); // it will create the command and store it in command variable
    char childProcessIds[128]; // it will create the space of the pointer
    FILE *fp = popen(command, "r"); // it will execute the command and store the output in a file and return the filepointer
    
    fgets(childProcessIds, sizeof(childProcessIds), fp ); // it will read the content of file and store it in chilProcessIds buffer
    char *saveptr1;
    char *child_process_id = strtok_r(childProcessIds, " ",&saveptr1);  // it will tokenize the buffer using demiliter as space
    while(child_process_id != NULL && isValidNumber(child_process_id)==1) { 
        if(strcmp(child_process_id,child_id)==0)  //if child id and process id are same then it will print child id and parent id or calls options function
        { 
            found=1;
            if(option==NULL)
                printf("%s %s\n",child_process_id,parent_id);
            
            else
            {
                options(parent_id,child_id); // if option is not null .. calls options function
            }
            return;
        }
        search(child_process_id,child_id);// recursively calls the fucnton
        if(found==1)
        {
            return;
        }
        if ((child_process_id = strtok_r(NULL, " ",&saveptr1)) == NULL) {
            break;
        }
    }
    
}



int main(int argc, char *argv[]){
    if(argc==2)   // if number of arguments are 2 then it should be -xc or asks for valid arguments
    {
        option=argv[1];
        if(strcmp(option,"-xc")==0)
        {
            continueAllProcesses();
        }
        else{
            printf("pass valid argument/s\n");
        }
        return 0;
    }
    else if(argc==3) //if number of args is 3 then command without options or -pr option
    {
        if(strstr(argv[2],"-")) //checks for - , if - presents then it is -pr or else without options
        {
            root_process=argv[1]; // loads arg1 into root process
            option=argv[2]; // loads arg2 into root option
            if (isValidProcess(root_process)) { //checks whether root process is valid or not
                if(strcmp(option,"-pr")==0) // checks for the option is -pr or not
                    killprocessid(root_process); // kills the root process
                else{
                    printf("enter valid arguments\n");
                }
            } 
            else{
                printf("root process doesn't exist\n");
            }
            return 0;
        }
        else{
            process_id=argv[1]; // loads arg1 into process id
            root_process=argv[2]; // loads arg2 into root process
        }
    }
    else if(argc==4)
    {
        process_id=argv[1];   // loads arg1 into process id
        root_process=argv[2]; // loads arg2 into root process
        if (!isValidProcess(root_process)) {
            printf("root process doesn't exist\n");
            return 0;
        } 
        option=argv[3]; // loads arg3 into option
    }
    else
    {
        printf("enter valid number of arguments\n"); 
    }

    search(root_process,process_id);  //calls search function to search for the process id in root process tree

    if(found==0) //if process tree doesn't found prints error msg
    {
        printf("Does not belong to the process tree\n");
    }
    
    return 0;
    
}