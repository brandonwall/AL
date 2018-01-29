/*Brandon Wallerson
  114003737
  bwallers@umd.edu
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <err.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include "command.h"
#include "executor.h"

/*Prototype for print tree method*/
static void print_tree(struct tree *t);

int execute_helper(struct tree *t, int fd_in,int fd_out){
  pid_t pid;
  pid_t child_fd;
  pid_t child2_fd;
  pid_t sub_pid;
  int doleft = 1;
  int pipe_fd[2];
  /*Checks to see if t is NULL*/
  if(t){
    if(t->conjunction == NONE){
      /*if command is "cd", then, if there is another argument,
        go to that directory, else go home*/
      if(strcmp("cd", t->argv[0])==0){
	if(t->argv[1] != NULL){
	  chdir(t->argv[1]);
	  doleft = 0;/*Keeps track of status; 0 is success*/
	}else{
	  chdir(getenv("HOME"));
	  doleft = 0;
	}
	/*If command entered is exit, terminate program*/
      }else if(strcmp("exit",t->argv[0])==0){
	doleft = 0;
	exit(0);
      }else{
	/*First fork. Processes are exuted in the child, the parent
	  waits. */
	if((pid = fork())<0){
	  perror("fork");
	}
	/*Parent process*/
	if(pid!=0){
	  wait(&doleft);
	  /*Child process*/
	}else{
	  /*Checks for different input and adjusts*/
	  if(t->input != NULL){
	    if(fd_in == STDIN_FILENO){
	      if((fd_in = open(t->input,O_RDONLY))<0){
		perror("file openning fdin\n");
	      }
	      if(dup2(fd_in,STDIN_FILENO)<0){
		perror("dup2\n");
	      }
	      close(fd_in);
	    }else{
	      exit(0);
	    }
	  }
	  /*Checks for different output and adjusts*/
	  if(t->output != NULL){
	    if(fd_out == STDIN_FILENO){
	      if((fd_out = open(t->output,O_CREAT|O_WRONLY,0664))<0){
		perror("file openning fdout\n");
	      }
	      if(dup2(fd_out,STDIN_FILENO)<0){
		perror("dup2\n");
	      }
	      close(fd_out);
	    }else{
	      printf("Ambiguous output redirect.\n");
	      exit(1);
	    }
	  }
	  /*Executes command. Exits on error*/
	  doleft = execvp(t->argv[0],t->argv);
	  fprintf(stderr, "Failed to execute %s\n",t->argv[0]);
	  exit(1);	
	  
	}
      }
      /*Code for if the conjunction is AND*/
    }else if(t->conjunction == AND){
      /*Checks left side first to see if its null*/
      if(t->left!=NULL){
	/*Do left returns 0 on success*/
	doleft = execute_helper(t->left,fd_in,fd_out);
      }
      /*If code is executed correctly and the right side
        is not null, then execute right side*/
      if(doleft ==0&&t->right!=NULL){
	execute_helper(t->right,fd_in,fd_out);
      }
      /*Code for PIPE conjunction*/
    }else if(t->conjunction == PIPE){
      /*Forks to create pipeline between 2 processes */
      if (pipe(pipe_fd) < 0) {
	err(EX_OSERR, "pipe error");
      }
      if ((child_fd = fork()) < 0) {
      err(EX_OSERR, "fork error");
      }
      /*Child Process*/
      if(child_fd == 0){
	close(pipe_fd[0]);
	/* Redirecting standard output to pipe write end */
	if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
	  err(EX_OSERR, "dup2 error");
	}
	/* Releasing resource */
	close(pipe_fd[1]);
	doleft = execute_helper(t->left,STDIN_FILENO, STDOUT_FILENO);
	exit(0);
	/*Parent process*/
      }else{
	/*Create second child process for output*/
	if ((child2_fd = fork()) < 0) {
	  err(EX_OSERR, "fork error");
	}
	if(child2_fd == 0){
	  close(pipe_fd[1]);
	  if (dup2(pipe_fd[0], STDIN_FILENO) < 0) {
	    err(EX_OSERR, "dup2 error");
	  }
	  close(pipe_fd[0]);
	  doleft =  execute_helper(t->right,STDIN_FILENO, STDOUT_FILENO);
	  exit(0);
	  /*Parent of second child (closes pipes since it doesnt need it)*/
	}else{
	  close(pipe_fd[0]);
	  close(pipe_fd[1]);
	  wait(&doleft);
	  if(doleft == 0){
	    wait(&doleft);
	  }else{
	    wait(NULL);
	  }
	}
      }
      /*Code for subshel*/
    }else if(t->conjunction == SUBSHELL){
      /*Fork first to make sure that all proecesses occur
        inside its own environment */
      if((sub_pid = fork())<0){
	perror("fork");
      }
      /*Parent proecess*/
      if(sub_pid !=0){
	wait(&doleft);
	/*Child process*/
      }else{
	/*Perform input output check similiar to NONE*/
	if(t->input != NULL){
	    if(fd_in == STDIN_FILENO){
	      if((fd_in = open(t->input,O_RDONLY))<0){
		perror("file openning fdin\n");
	      }
	      if(dup2(fd_in,STDIN_FILENO)<0){
		perror("dup2\n");
	      }
	      close(fd_in);
	    }else{
	      exit(0);
	    }
	  }
	  
	  if(t->output != NULL){
	    if(fd_out == STDIN_FILENO){
	      if((fd_out = open(t->output,O_CREAT|O_WRONLY,0664))<0){
		perror("file openning fdout\n");
	      }
	      if(dup2(fd_out,STDIN_FILENO)<0){
		perror("dup2\n");
	      }
	      close(fd_out);
	    }else{
	      printf("Ambiguous output redirect.\n");
	      exit(1);
	    }
	  }
	  /*Execute node in tree by recursive call within own environement*/
      doleft =  execute_helper(t->left,STDIN_FILENO, STDOUT_FILENO);
      exit(0);
      }
       
    }
  }
  /*Always return status(helps with deciding whether or not 
    to execute right tree)*/
  return doleft;
}

/*Function to execute tree using helper method
  if tree is null, do nothing*/
int execute(struct tree *t) {
  if(t!=NULL){
  return execute_helper(t,STDIN_FILENO,STDOUT_FILENO);
  }
  print_tree(t);
  return 0;
}
      
/*Method to print out the current tree*/
static void print_tree(struct tree *t) {
   if (t != NULL) {
      print_tree(t->left);

      if (t->conjunction == NONE) {
         printf("NONE: %s, ", t->argv[0]);
      } else {
         printf("%s, ", conj[t->conjunction]);
      }
      printf("IR: %s, ", t->input);
      printf("OR: %s\n", t->output);

      print_tree(t->right);
   }
}

