#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BIND_IP_ADDR "127.0.0.1"
#define BIND_PORT 8000
#define MAX_RECV_LEN 1048576
#define MAX_SEND_LEN 1048576
#define MAX_PATH_LEN 1024
#define MAX_HOST_LEN 1024
#define MAX_CONN SOMAXCONN
#define MAXEVENTS 1024

#define HTTP_STATUS_200 "200 OK"
#define HTTP_STATUS_404 "404 File Not Found"
#define HTTP_STATUS_500 "500 Internal Server Error"

char path[MAX_CONN][MAX_PATH_LEN];
ssize_t path_len[MAX_CONN];

void write_everything (int clnt_sock, char *response, int write_len) {
	int write_count = 0;
	//while (write_count < write_len) {
		int count = write (clnt_sock, response, MAX_SEND_LEN);
		write_count += count;
		//debug
		printf ("write_everything\n");
	//}
}

int file_size (char *filename) {
	struct stat statbuf;
	stat (filename, &statbuf);
	int size = statbuf.st_size;
	return size;
}

void parse_request (char* request, ssize_t req_len, int i) {
    char* req = request;

    // 一个粗糙的解析方法，可能有 BUG！
    // 获取第一个空格(s1)和第二个空格(s2)之间的内容，为 PATH
    ssize_t s1 = 0;
    while(s1 < req_len && req[s1] != ' ') s1++;
    ssize_t s2 = s1 + 1;
    while(s2 < req_len && req[s2] != ' ') s2++;

    memcpy(path[i], req + s1 + 1, (s2 - s1 - 1) * sizeof(char));
    path[i][s2 - s1 - 1] = '\0';
    path_len[i] = (s2 - s1 - 1);
}

void handle_clnt_read (int clnt_sock, int i) {
    // 一个粗糙的读取方法，可能有 BUG！
    // 读取客户端发送来的数据，并解析
	// 将解析得到的路径送入全局变量path
	char end_of_req[] = "/r/n/r/n";

	//debug
	printf("here1\n");

    char* req_buf = (char*) malloc(MAX_RECV_LEN * sizeof(char));

	//debug
	printf("here2\n");

	char* req_buf_ptr = req_buf;
	ssize_t req_len = 0;
    // 将 clnt_sock 作为一个文件描述符，读取最多 MAX_RECV_LEN 个字符
    // 但一次读取并不保证已经将整个请求读取完整
	//debug
	//while (strstr (req_buf, end_of_req) == NULL) {
		//只要没有“/r/n/r/n”就继续读取
		ssize_t n = read (clnt_sock, req_buf_ptr, MAX_RECV_LEN);
		req_buf_ptr += n;
		req_len += n;

		//debug
		printf("here3,	req_len: %zd\n", req_len);
		printf("req_buf: %s\n", req_buf);
		char *debug = strstr (req_buf, end_of_req);
		printf ("strstr return value: %s\n", debug);
	//}

    //ssize_t req_len = read(clnt_sock, req_buf, MAX_RECV_LEN);
    // 根据 HTTP 请求的内容，解析资源路径和 Host 头
    parse_request(req_buf, req_len, i);
	free(req_buf);
}

void handle_clnt_write (int clnt_sock, int i) {
    // 构造要返回的数据
    // 这里没有去读取文件内容，而是以返回请求资源路径作为示例，并且永远返回 200
    // 注意，响应头部后需要有一个多余换行（\r\n\r\n），然后才是响应内容
	// 一次不一定写的完
    char* response = (char*) malloc(MAX_SEND_LEN * sizeof(char)) ;
	int len = strlen (path[i]);
	if (path[i][len - 1] == '/') {
		//请求的资源为目录
    	sprintf(response, 
        	"HTTP/1.0 %s\r\nContent-Length: %d\r\n\r\n", 
        	HTTP_STATUS_500, 0);
    	size_t response_len = strlen(response);
    	write(clnt_sock, response, response_len);
	} else {
		//不是目录，则看是否存在该文件
		char relative_path[MAX_PATH_LEN] = ".";
		strcat (relative_path, path[i]);
		int fd = open (relative_path, O_RDONLY);
		if (fd == -1) {
			//不存在
    		sprintf(response, 
        		"HTTP/1.0 %s\r\nContent-Length: %d\r\n\r\n", 
        		HTTP_STATUS_404, 0);
    		size_t response_len = strlen(response);
    		write(clnt_sock, response, response_len);
		} else {
			//该文件存在
			//先获取文件长度
			//再读取并发送
			int content_len = file_size (relative_path);
    		sprintf(response, 
        		"HTTP/1.0 %s\r\nContent-Length: %d\r\n\r\n", 
        		HTTP_STATUS_200, content_len);
    		size_t response_len = strlen(response);
    		write(clnt_sock, response, response_len);
			//读取并发送文件内容
			int write_len = 0, read_len = 0;
			//while (read_len < content_len) {
				int read_count = read (fd, response, MAX_SEND_LEN);

				//debug
				printf("here6\tread_count: %d\n", read_count);

				write_everything (clnt_sock, response, read_count);
				read_len += read_count;

				//debug
				printf("here7\tread_len: %d\tcontent_len: %d\n", read_len, content_len);
			//}
		}
	}

    // 关闭客户端套接字
    close(clnt_sock);
    
    // 释放内存
    free(response);
}

int main(){
    // 创建套接字，参数说明：
    //   AF_INET: 使用 IPv4
    //   SOCK_STREAM: 面向连接的数据传输方式
    //   IPPROTO_TCP: 使用 TCP 协议
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serv_sock == -1){
		perror("socker: ");
		exit(EXIT_FAILURE);
	}
    // 将套接字和指定的 IP、端口绑定
    //   用 0 填充 serv_addr （它是一个 sockaddr_in 结构体）
    struct sockaddr_in serv_addr;
    memset (&serv_addr, 0, sizeof(serv_addr));
    //   设置 IPv4
    //   设置 IP 地址
    //   设置端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(BIND_IP_ADDR);
    serv_addr.sin_port = htons(BIND_PORT);
    //   绑定
	if (bind (serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
		perror ("bind: ");
		exit (EXIT_FAILURE);
	}
    // 使得 serv_sock 套接字进入监听状态，开始等待客户端发起请求
	if (listen (serv_sock, MAX_CONN) == -1){
		perror ("listen: ");
		exit (EXIT_FAILURE);
	}
	//创建epoll
	int efd;
	if((efd = epoll_create1 (0)) == -1){
	//if(efd = epoll_create (1024) == -1){
		perror("epoll_create1: ");
		exit(EXIT_FAILURE);
	}

	struct epoll_event event;
	struct epoll_event *events;
	event.data.fd = serv_sock;
	event.events = EPOLLIN;
	if (epoll_ctl (efd, EPOLL_CTL_ADD, serv_sock, &event) == -1){
		printf("errno: %d\n", errno);
		printf("serv_sock: %d\n", serv_sock);
		printf("efd: %d\n", efd);
		perror ("epoll_ctl1: ");
		exit (EXIT_FAILURE);
	}
	//创建event数组
	events = calloc (MAXEVENTS, sizeof (event));
	while (1) {

		//debug
		printf("a new cycle\n");

		int ready_ev_num, i;
		ready_ev_num = epoll_wait (efd, events, MAXEVENTS, -1);		//-1为阻塞
		for (i = 0; i < ready_ev_num; i++) {
			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                // 监控到错误或者挂起
                perror ("epoll error: ");
                close (events[i].data.fd);
                continue;
            }
			if (events[i].events & EPOLLIN) {
				//recieve new connection
				//or connected socket has data in
				if (serv_sock == events[i].data.fd) {
					//have new connection
    				// 接收客户端请求，获得一个可以与客户端通信的新生成的套接字 clnt_sock
				    struct sockaddr_in clnt_addr;
				    socklen_t clnt_addr_size = sizeof(clnt_addr);
        			int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
					//don't need to handle request this time, we just need to add it to epoll
					//将新的socket加入epoll
					event.data.fd = clnt_sock;
					event.events = EPOLLIN;
					if (epoll_ctl(efd, EPOLL_CTL_ADD, clnt_sock, &event) == -1) {
						perror ("epoll_ctl2: ");
						exit(EXIT_FAILURE);
					}
				} else {
					//have something to read
					//handle request
					handle_clnt_read (events[i].data.fd, i);
					//修改socket为EPOLLOUT，因为要写回数据了
					event.events = EPOLLOUT;
					epoll_ctl (efd, EPOLL_CTL_MOD, events[i].data.fd, &event);
				}
			} else if ((events[i].events & EPOLLOUT) && (events[i].data.fd != serv_sock)) {
				//向socket中写数据
	
				// debug
				printf("here4\tready_ev_num: %d\n", ready_ev_num);

				handle_clnt_write (events[i].data.fd, i);
				//这里该函数与上面的不同，该函数中最后需要关闭fd
	
				// debug
				printf("here5\n");

			}
		}
	}
    // 实际上这里的代码不可到达，可以在 while 循环中收到 SIGINT 信号时主动 break
    // 关闭套接字
    close (serv_sock);
    close (efd);
	free (events);
    return 0;
}
