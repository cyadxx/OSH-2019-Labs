#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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
#define HTTP_STATUS_404 "404 Not Found"
#define HTTP_STATUS_500 "500 Internal Server Error"

char path[MAX_CONN][MAX_PATH_LEN];

void parse_request(char* request, ssize_t req_len, char* path, ssize_t* path_len)
{
    char* req = request;

    // 一个粗糙的解析方法，可能有 BUG！
    // 获取第一个空格(s1)和第二个空格(s2)之间的内容，为 PATH
    ssize_t s1 = 0;
    while(s1 < req_len && req[s1] != ' ') s1++;
    ssize_t s2 = s1 + 1;
    while(s2 < req_len && req[s2] != ' ') s2++;

    memcpy(path, req + s1 + 1, (s2 - s1 - 1) * sizeof(char));
    path[s2 - s1 - 1] = '\0';
    *path_len = (s2 - s1 - 1);
}

void handle_clnt(int clnt_sock)
{
    // 一个粗糙的读取方法，可能有 BUG！
    // 读取客户端发送来的数据，并解析
    char* req_buf = (char*) malloc(MAX_RECV_LEN * sizeof(char));
    // 将 clnt_sock 作为一个文件描述符，读取最多 MAX_RECV_LEN 个字符
    // 但一次读取并不保证已经将整个请求读取完整
    ssize_t req_len = read(clnt_sock, req_buf, MAX_RECV_LEN);

    // 根据 HTTP 请求的内容，解析资源路径和 Host 头
    char* path = (char*) malloc(MAX_PATH_LEN * sizeof(char));
    ssize_t path_len;
    parse_request(req_buf, req_len, path, &path_len);
    
    // 构造要返回的数据
    // 这里没有去读取文件内容，而是以返回请求资源路径作为示例，并且永远返回 200
    // 注意，响应头部后需要有一个多余换行（\r\n\r\n），然后才是响应内容
    char* response = (char*) malloc(MAX_SEND_LEN * sizeof(char)) ;
    sprintf(response, 
        "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n%s", 
        HTTP_STATUS_200, path_len, path);
    size_t response_len = strlen(response);

    // 通过 clnt_sock 向客户端发送信息
    // 将 clnt_sock 作为文件描述符写内容
    write(clnt_sock, response, response_len);

    // 关闭客户端套接字
    close(clnt_sock);
    
    // 释放内存
    free(req_buf);
    free(path);
    free(response);
}

int main(){
    // 创建套接字，参数说明：
    //   AF_INET: 使用 IPv4
    //   SOCK_STREAM: 面向连接的数据传输方式
    //   IPPROTO_TCP: 使用 TCP 协议
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(serv_sock == -1){
		perror("socker: ");
		exit(EXIT_FAILURE);
	}
    // 将套接字和指定的 IP、端口绑定
    //   用 0 填充 serv_addr （它是一个 sockaddr_in 结构体）
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    //   设置 IPv4
    //   设置 IP 地址
    //   设置端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(BIND_IP_ADDR);
    serv_addr.sin_port = htons(BIND_PORT);
    //   绑定
	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
		perror("bind: ");
		exit(EXIT_FAILURE);
	}
    // 使得 serv_sock 套接字进入监听状态，开始等待客户端发起请求
	if(listen(serv_sock, MAX_CONN) == -1){
		perror("listen: ");
		exit(EXIT_FAILURE);
	}
	//创建epoll
	int efd;
	if(efd = epoll_create1 (0) == -1){
		perror("epoll_create1: ");
		exit(EXIT_FAILURE);
	}
	struct epoll_event event;
	struct epoll_event *events;
	event.data.fd = serv_sock;
	event.events = EPOLLIN;
	if (epoll_ctl (efd, EPOLL_CTL_ADD, serv_sock, &event) == -1){
		perror ("epoll_ctl: ");
		exit (EXIT_FAILURE);
	}
	//创建event数组
	events = calloc (MAXEVENTS, sizeof (event));
	while (1) {
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
					//handle_clnt(clnt_sock);
					//将新的socket加入epoll
					event.data.fd = clnt_sock;
					event.events = EPOLLIN;
					if (epoll_ctl(efd, EPOLL_CTL_ADD, clnt_sock, &event) == -1) {
						perror ("epoll_ctl: ");
						exit(EXIT_FAILURE);
					}
				} else {
					//have something to read
					//handle request
					handle_clnt(events[i].data.fd);
					//修改socket为EPOLLOUT，因为要xie hui数据了
					event.events = EPOLLOUT;
					epoll_ctl (efd, EPOLL_CTL_MOD, events[i].data.fd, &event);
				}
			} else if((events[i].events & EPOLLOUT) && (events[i].data.fd != sfd)) {
				//向socket中写数据
				handle_clnt (events[i].data.fd);
				//这里该函数与上面的不同，该函数中最后需要关闭fd
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
