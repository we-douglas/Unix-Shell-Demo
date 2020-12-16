#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

char* cmdBuffer;
int p[2];

int childEXE(char *command);
char history[200];
char *history1;

/**
myShellFinal.c 
William Douglas

This file holds all the methods needed to build a simple unix shell
that will execute on a given terminal. the program launches with main()
however most of the functions are defined either in there own method 
or within the executor method, childEXE() Please refer to README file
for more info. 
**/


/**
Method is used to read the user input from the command line.
Void,
Returns char *ln, this represents one line from the command line.
**/
char *readCmdLine(){
  char *ln = "";
  char *endln="\n";
  ssize_t bufsize = 0;
  getline(&ln, &bufsize, stdin);
  ln[strcspn(ln,endln)]=0; // this just trims "\n" from the input because we dont need it.
  return ln;
}

/**
This Method is used to parse commands and command arguments from an input String.
Note we do not parse mulitpule commands here, we only use the deliminatior " ". This is because 
the ";" deliminator is taken care of later in Main where its easier to fork for execution.

char *input,  The input string to be parsed.
Returns char **cmd, This array holds the command name at  cmd[0] and its arguments in cmd[1 -> ..],  
**/
char **parseCmds(char *input) {
   char **cmd = malloc(64 * sizeof(char *));
   char *temp; int  i=0;

   temp = strtok(input, " ");       // this is used to split the inputline at each " " char.
   while (temp != NULL) {
      cmd[i] =temp;
      temp = strtok(NULL, " ");
      i++;
    }
    cmd[i] = NULL;
    return cmd;
}

/**
This method handles the cd command by just calling chdir() to do all the work for us.

char *arg, this is the normal path argument for cd and can even be ".."
return, we dont care about.  
 **/
int cd(char *arg) {if(arg !=NULL){ return chdir(arg);}}


/**
This is just a helper Method to split strings cleanly, use case is alot like String.split() in java.

char *input, the input string to be split up
char *delim, The char* we use to delim the string
Returns char **out, this is just an array of input split at each index of the delim
**/
char **strSplit(char *input,char *delim){
  char *ch;
  int idx=0;
  char **out=malloc(64 * sizeof(char *));
  ch = strtok(input,delim);
  while(ch !=NULL){
    out[idx]=ch;
    ch = strtok(NULL,delim);
    idx++;
	}
  return(out);
}


/**
readFile handles most of the file function we need for the program.

char *fileName, this is the name or path or the file we need
int print, this determines if the file contents will be printed(we only use for help cmd)  
Return out, this is and array of each line pulled from the file.
**/
char **readFile(char *fileName, int print)
{
    char fileIn[1000][1000];
    char c[1000];
    int idx =0;
    FILE *fptr;
    char temp[1000];
    char in[1000];
    char *input;

    char **out=malloc(64 * sizeof(char *));
    if ((fptr = fopen(fileName, "r")) == NULL)
    {
        printf("Error! cant find file!");
        exit(1);         
    }
    while (fgets(fileIn[idx],sizeof(c),fptr)!=NULL){  //this block is populates a char** with each line onfile//
      if(print==1){printf("%s",fileIn[idx]);} 
      out[idx]=fileIn[idx];
      idx++;
     }
    return out;
}
/**
redirectOutput splits the input at index i or the index of the ">" char and 
uses dup to change the output location to the file name to the right of i
the method then uses childEXE to evaluate the command  to the left
after it is done we return the output to STDOUT
 **/

void redirectOutput(char **args, int i,char *input){// for >
  printf("Input: %d  %s\n",i,input);
  char newInput[sizeof(input)];
  for(int a=0;a<i;a++){
  strcat(newInput,args[a]);
  strcat(newInput," ");
  }

  int term;
  term =dup(1);
  printf("Output Directed to File: %s\n",args[i+1]);
  FILE *fptr;
  char **out[1000];
  int fw = open(args[i+1], O_WRONLY| O_APPEND);
  dup2(fw,STDOUT_FILENO);
  childEXE(newInput);
  printf("\n");
  dup2(term,STDOUT_FILENO);
  close(term);
  close(fw);
}

/**
   this command is used to setup the < command
   it splits the input at the index of < then executed everything on the left with data
   read from the file on the right.
 **/
void redirectInput(char **args, int i,char* input){// for <
  printf("Input: %d  %s\n",i,input);
  char newInput[sizeof(input)+100];
  for(int a=0;a<i;a++){
  strcat(newInput,args[a]);
  strcat(newInput," ");
  }
   strcat(newInput,args[i+1]);
   // printf("Input: %s\n",newInput);

  int term;
  term =dup(1);
  int fw = open(args[i+1], O_RDONLY);
  dup2(fw,0);
  childEXE(newInput);
  printf("\n");
  dup2(term,STDOUT_FILENO);
  close(term);
  close(fw);
}




/**
childEXE takes a given command and will parse arguments and 
then execute the command on execvp or a custom defined method
like for cd etc. This process is only called on a fork so it
never runs in the main threaad.

 **/
int childEXE(char *command){
  char **args;
  int tempIdx;
  char x [100];
  strcpy(x,command);
  args = parseCmds(command); 
  if(args ==NULL){exit(1);}
  if(args[2] != NULL){
    for(int i=0;i<sizeof(args);i++){
      printf("args: %s",args[i]);
      if(strcmp(args[i], ">")==0){redirectOutput(args,i,x);break;}
      if(strcmp(args[i], "<")==0){redirectInput(args,i,x);break;}
    }
  }
  else if(strcmp(args[0], "cd") ==0){cd(args[1]);}
  else if(strcmp(args[0], "exit") ==0){exit(0);}
  else if(strcmp(args[0], "help") ==0){readFile("README",1);}
  else{
    execvp(args[0], args);
  }
  return 1;
}




/**
   This is the main function, it directs all action on the program.
   I decided to change the main loop provided to make it look better and
   To be more functional for my use here. 
  
   A basic manifest:
   1.) Get input
   2.) Check if its a pipe command
   2.) fork()
   3.) childEXE command
   4.) return to parent hold or kill the loop depending.

   return, no return info.
 **/
int main() {
  printf("starting myShell...\n \n");
  readFile("intro.txt",1);
  char *input;
  char path[1000];
  char **command;
  char **commands;
  pid_t child;
  int stat_loc;
  char **pipes;
  char h1[200];
  int pipeId[2];
  char inMsg[1000];
  strcat(history,"No History");


  while (1) {
    getcwd(path,sizeof(path));    // this gets our path
    printf("\n%s>",path);
    input = readCmdLine();

    // exit case //
    if(strcmp(input, "exit")==0){ printf("\n Thank you for using the Magic Conch Shell Shell! \n");break;}
    




  // here we check if the input is a pipe command or not
  pipes=strSplit(input,"|"); 
  if(pipes[1]!=NULL){


    // this breaks the | command into 2 redirect commands
  char newInputA[200];
  memset(newInputA,0,200);
  strcat(newInputA,pipes[0]);
  strcat(newInputA," > pipe.txt");
  char newInputB[200];
  memset(newInputB,0,200);
  strcat(newInputB,pipes[1]);
  strcat(newInputB," < pipe.txt");

  fclose(fopen("pipe.txt","w")); // wipe data on pipe file

  // now we run the two commands     
  for(int i=0;i<sizeof(pipes);i++){
      if(pipes[i]!=NULL){       
      child = fork();
      if(child==0){
	if(i==0){waitpid(0, &stat_loc, WUNTRACED);childEXE(newInputA);}
	else{waitpid(0, &stat_loc, WUNTRACED);childEXE(newInputB);}
      }
      else{waitpid(0, &stat_loc, WUNTRACED);}
      }
      }
  }

  // if the input is not a pipe command we exe as normal
  else{
  commands=strSplit(input,";");                  // this block takes the input,splits it with ';' as a delim //	
  for(int i=0;i<sizeof(commands);i++){           // for each command we exexute eveything below //
    if(commands[i]!=NULL){
      char *his1=history;
      strcpy(h1,his1);
      memset(history,0,200);
      strcat(history,commands[i]);
      pipe(pipeId);
      child = fork();


      // I decided to just pipe here in main to handle the history function.
      if(child>0){
	close(pipeId[0]);
	write(pipeId[1],h1,strlen(h1)+1);
	close(pipeId[1]);
      }	
      else {	
	read(pipeId[0],inMsg,1000);
	if(strcmp(commands[i], "!!") ==0){childEXE(inMsg);}
	else{childEXE(commands[i]);}
	close(pipeId[0]);
	
      }
      if(child==0){}
      else{
	waitpid(0, &stat_loc, WUNTRACED);
      }
  }
      
  }
  }
  }
  return 0;
  
  
}
