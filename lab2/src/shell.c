#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>

/******************************************pipe********************************************/

int IsPipe(char *args[]){
	/*判断是否为管道命令*/
	for(int i = 0; args[i] != NULL; i++)
		if(strcmp(args[i], "|") == 0)
			return (i+1);
	return 0;
}

void PipeExe(char *args[]){
	/* 
	 * 执行管道命令
	 */
	int i, j;
	int pipe_num = 0;
	for (i = 0; args[i]; i++)
    	if (*args[i] == '|')
        	pipe_num++;
	char *new_args[128][128];
    int a = 0;
    pid_t pid;
	
	for(i = 0; args[a] != NULL; i++){
		for(j = 0; args[a] != NULL; j++){
			if(strcmp(args[a], "|") != 0){
				new_args[i][j] = args[a];
				a++;
			} else {
				new_args[i][j] = NULL;
				a++;
				break;
			}
		}
		if(args[a] == NULL)
			new_args[i][j] = NULL;
	}
	new_args[i][0] = NULL;
	
    int fd[128][2];
    for(i=0;i<pipe_num;i++)
        if((pipe(fd[i]))==-1){
            printf("pipe error!\n");
            exit(-1);
        }
    for(i=0;i<pipe_num+1;i++){
        if((pid=fork())==0)
            break;
        else if(pid==-1){
            printf("fork error!\n");
            exit(-1);
        }
    }

    if(i==0){
        for(j=0;j<pipe_num;j++){
            close(fd[j][0]);
            if(j!=i)
                close(fd[j][1]);
        }
        dup2(fd[i][1],STDOUT_FILENO);
        execvp(new_args[i][0],new_args[i]); 
    }
    else if(i>0&&i<pipe_num){
        for(j=0;j<pipe_num;j++){
            if(j!=i-1)
                close(fd[j][0]);
            if(j!=i)
                close(fd[j][1]);
        }
        dup2(fd[i-1][0],STDIN_FILENO);
        dup2(fd[i][1],STDOUT_FILENO);
        execvp(new_args[i][0],new_args[i]);
    }
    else if(i==pipe_num){
        for(j=0;j<pipe_num;j++){
            close(fd[j][1]);
            if(j!=i-1)
                close(fd[j][0]);
        }
        dup2(fd[i-1][0],STDIN_FILENO);
        execvp(new_args[i][0],new_args[i]);
    }
    else{
		for(j=0;j<pipe_num;j++){
            close(fd[j][0]);
            close(fd[j][1]);
		}
        for(j=0;j<pipe_num+1;j++)
            wait(NULL);
    }
}

/*****************************************redirection*******************************************/

int IsRedirection(char *args[]){
	/*判断是否为重定向命令*/
	/* type = 1, ">"  *
	 * type = 2, ">>" *
	 * type = 3, "<"ls | grep she  */
	for(int i = 0; args[i] != NULL;i++){
		if(strcmp(args[i], ">") == 0){
			return 1;
		} else if(strcmp(args[i], ">>") == 0){
			return 2;
		} else if(strcmp(args[i], "<") == 0){
			return 3;
		}
	}
	return 0;
}

void RedirectionSeparate(char *args[], char *args1[], char *args2[]){
	/*将重定向命令拆开*/
	int i;
	for(i = 0; ((strcmp(args[i], ">") != 0) && (strcmp(args[i], ">>") != 0) && (strcmp(args[i], "<") != 0)); i++){
		args1[i] = args[i];
	}
	args1[i] = NULL;
	i++;
	int j = 0;
	for(; args[i] != NULL; i++, j++){
		args2[j] = args[i];
	}
	args2[j] = NULL;
}

void RedirectionExe(char *args1[], char *args2[], int type){
	/*执行重定向命令*/
	/* type = 1, ">"
	 * type = 2, ">>"
	 * type = 3, "<" */
	if(type == 1){
		pid_t pid = fork();
		if(pid == 0){
			/*子进程*/
			int fd = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			dup2(fd, 1);
			execvp(args1[0], args1);
			/*execvp失败*/
			printf("Redirection1 execvp error!\n");
		} else{
			wait(NULL);
		}
	} else if(type == 2){
		pid_t pid = fork();
		if(pid == 0){
			/*子进程*/
			int fd = open(args2[0], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			dup2(fd, 1);
			execvp(args1[0], args1);
			/*execvp失败*/
			printf("Redirection2 execvp error!\n");
		} else{
			wait(NULL);
		}
	} else if(type == 3){
		pid_t pid = fork();
		if(pid == 0){
			/*子进程*/
			int fd = open(args2[0], O_RDONLY);
			dup2(fd, 0);
			execvp(args1[0], args1);
			/*execvp失败*/
			printf("Redirection3 execvp error!\n");
		} else{
			wait(NULL);
		}

	} else {
		printf("Redirection type error!\n");
	}
}

/******************************************main********************************************/

int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];					//拆解后的命令
    while (1) {
        /* 提示符 */
        printf("# ");
        fgets(cmd, 256, stdin);
        fflush(stdin);
    	//char *cmd;						//整行的命令
		//cmd = readline("# ");
		//add_history(cmd);
        /* 清理结尾的换行符 */
        int i, j;
        for (i = 0; cmd[i] != '\n'; i++)
            ;
        cmd[i] = '\0';
        /* 拆解命令行 */
        args[0] = cmd;
        for (i = 0; *args[i]; i++)
            for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
                if (*args[i+1] == ' ') {
                    *args[i+1] = '\0';
                    args[i+1]++;
                    break;
                }
        args[i] = NULL;
		/*删除空格*/
		for(i = 0; args[i] != NULL; i++){
			while(args[i][0] == ' '){
				for(j = 0; args[i][j] != '\0'; j++){
					args[i][j] = args[i][j+1];
				}
			}
		}
		for(i = 0; args[i] != NULL; i++)
			//while((args[i][0] == '\0') && (args[i] != NULL)){
			while((args[i] != NULL) && (args[i][0] == '\0')){
				for(j = i; args[j] != NULL; j++){
					args[j] = args[j+1];
				}
			}

        /* 没有输入命令 */
        if (!args[0])
            continue;

        /* 内建命令 */
		/*pipe*/
		/*管道优先级最高*/
		if(IsPipe(args) != 0){
			/*是管道*/
			PipeExe(args);
			continue;
		}

		/*redirection*/
		int redir = IsRedirection(args);
		if(redir != 0){
			/*是重定向命令*/
			char *args1[128];
			char *args2[128];
			RedirectionSeparate(args, args1, args2);
			RedirectionExe(args1, args2, redir);
			continue;
		}

		/*cd*/
        if (strcmp(args[0], "cd") == 0) {
            if (args[1])
                if(chdir(args[1]) != 0)		//执行失败
					printf("chdir error!\n");
            continue;
        }

		/*pwd*/
        if (strcmp(args[0], "pwd") == 0) {
            char wd[4096];
			char *cwd = getcwd(wd, 4096);
			if(cwd == NULL)
				printf("getcwd error!\n");
			else
            	puts(cwd);
            continue;
        }

		/*export*/
		if (strcmp(args[0], "export")==0){
            char *value;
            for (value = args[1]; (*value != '=') && (*value != '\0'); value++);
            if (*value == '='){
                *value = '\0';
                value++;
                if(setenv(args[1],value,1)==-1)
                    printf("setenv error\n");
			} else {
                printf("format error!\n");
			}
            continue;
        }
		
		/*exit*/
        if (strcmp(args[0], "exit") == 0)
            return 0;

        /* 外部命令 */
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程 */
            execvp(args[0], args);
            /* execvp失败 */
            return 255;
        }
        /* 父进程 */
        wait(NULL);
	}
}
