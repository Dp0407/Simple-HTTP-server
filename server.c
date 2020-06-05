#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include "http.h"
#include "file_lib.h"

#define MAX_CONN 100

extern int signal;
int opt = 1;
int server_fd, client_fd[30];
struct sockaddr_in addr;
socklen_t ad_length = sizeof(addr);
int server_init() {
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket initialiazation failed\n");
        exit(1);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 4) < 0) {
        perror("Set socket option falied\n");
        exit(1);
    }
    //set the server_fd to NON_BLOCK
    int fl = fcntl(server_fd, F_GETFL,0);
    fl |= O_NONBLOCK;
    fcntl(server_fd, F_SETFL, fl);


    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8784);

    if (bind(server_fd, (struct sockaddr *)&addr, (socklen_t) sizeof(addr)) < 0) {
        perror("Can't bind on port 80\n");
        exit(1);
    }
    
    if (listen(server_fd, 0) == -1) {
        perror("Can't listen on port 80\n");
        exit(1);
    }
    
    printf("Listening on port 8784...\n");
    return server_fd;
}

void* client_connect() {
    
    char buff[32565];
    //accept 
    struct sockaddr_in addr;
    socklen_t addr_length = sizeof(addr);
    
    int client_fd = accept(server_fd, (struct sockaddr *)&addr,(&addr_length));
    signal = 0;
    if (client_fd == -1) {
        perror("Oops! Something wrong\n");
    } else {
         // Receive client request 
        if (recv(client_fd, buff, sizeof(buff),0) > 0) {
    	    struct http_header req_h;
    	    req_h = parse_http_req(buff);
    	    
            FILE *fp;
    	    char path[80] = ".";
    	    strcat(path, req_h.path);
            // Get requested file
    	    fp = fopen(path,"rb");
    	    if (fp == NULL) {
                fp = fopen("./404.html", "rb");
    	        int size = get_file_size(fp);
                char *data = get_file_content(fp, size);
                char *header = http_respone_404(req_h, NULL, size);
                strcat(header, data);
                send(client_fd, header, strlen(header), 0);
                free(data);
                free(header);
            } else {
    	        int size = get_file_size(fp);
                char *data = get_file_content(fp, size);
                char *header = http_respone_200(req_h, NULL, size);
    	        strcat(header, data);
	            send(client_fd, header, strlen(header), 0);
                free(data);
                free(header);
	         }
    	     shutdown(client_fd, SHUT_RDWR);
    	     close(client_fd); 
        }
    }  
}
