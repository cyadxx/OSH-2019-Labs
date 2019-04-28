#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

/******************************************pipe********************************************/

int IsPipe(char *args[]){
	/*判断是否为管道命令*/
	for(int i = 0; args[i] != NULL; i++)
		if(strcmp(args[i], "|") == 0)
			return (i+1);
	return 0;
}

void PipeSeparate(char *args[], char *args1[], char *args2[]){
	/*将管道命令拆开*/
	int i;
	for(i = 0; strcmp(args[i], "|") != 0; i++){
		args1[i] = args[i];
	}
	i++;
	args1[i] = NULL;
	int j = 0;
	for(; args[i] != NULL; i++, j++){
		args2[j] = args[i];
	}
	args2[++j] = NULL;
}

void PipeExe(char *args1[], char *args2[]){
	/*执行管道命令*/
	pid_t cpid = fork();
	if(cpid == -1){
		printf("fork error!\n");
		return;
	}else if(cpid > 0){
		//父进程
		wait(NULL);
	}else if(cpid == 0){
		//子进程
		int fd[2];
		if(pipe(fd) == -1){
			printf("pipe create error!\n");
			return;
		}
		pid_t ccpid = fork();	//创建子进程的子进程
		if(ccpid == -1){
			printf("fork error!\n");
			return;
		}else if(ccpid == 0){
			//子进程
			//close(fd[0]);		//子进程关闭读端
			dup2(fd[1], 1);		//管道写端与标准输出关联
			execvp(args1[0], args1);	
			/*execvp失败*/
			printf("execvp error!\n");
		}else{
			//父进程
			wait(NULL);
			close(fd[1]);
			dup2(fd[0], 0);		//管道读端与标准输入关联
			if(IsPipe(args2) == 0){
				/*不是管道*/
				execvp(args2[0], args2);
				/*execvp失败*/
				printf("execvp error!\n");
			}
			//}else{
				/*是管道*/
			//	char *args3[128];
			//	char *args4[128];
			//	PipeSeparate(args2, args3, args4);
			//	PipeExe(args3, args4);
			//}
		}
	}
}

/*****************************************redirection*******************************************/

int IsRedirection(char *args[]){
	/*判断是否为重定向命令*/
	/* type = 1, ">"  *
	 * type = 2, ">>" *
	 * type = 3, "<"  */
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
	i++;
	args1[i] = NULL;
	int j = 0;
	for(; args[i] != NULL; i++, j++){
		args2[j] = args[i];
	}
	args2[++j] = NULL;
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
			int fd = open(args2[0], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			dup2(fd, 1);
			execvp(args1[0], args1);
			/*execvp失败*/
			printf("Redirection execvp error!\n");
		}
		wait(NULL);
	} else if(type == 2){

	} else if(type == 3){

	} else {
		printf("Redirection type error!\n");
	}
}

/******************************************pipe********************************************/

int main() {
    /* 输入的命令行 */
    //char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];					//拆解后的命令
    while (1) {
        /* 提示符 */
        //printf("# ");
        //fgets(cmd, 256, stdin);
        fflush(stdin);
    	char *cmd;						//整行的命令
		cmd = readline("# ");
		//add_history(cmd);
        /* 清理结尾的换行符 */
        int i;
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

        /* 没有输入命令 */
        if (!args[0])
            continue;

        /* 内建命令 */
		/*pipe*/
		/*注意，管道优先级最高*/
		if(IsPipe(args) != 0){
			/*是管道*/
			char *args1[128];
			char *args2[128];
			PipeSeparate(args, args1, args2);
			PipeExe(args1, args2);
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
		if(strcmp(args[0], "export") == 0){
			if(putenv(args[1]) == -1){
				printf("putenv error!\n");
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
    	free(cmd);
	}
}
