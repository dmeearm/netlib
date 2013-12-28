/*
 * Copyright (c) 2013 Jason ding <dmeearm@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_EVENTS 128
#define BUFSIZE    10 

char buffer[BUFSIZE];

struct event {
    int fd;
    int read;
    int write;
    int (*readcb)(int fd, int *len);
    void (*writecb)(int fd);
};

struct epoll_event  events[MAX_EVENTS];

int listenfd_init()
{
    int ret;
    int fd;
    int reuseaddr = 1;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
        (const void *)&reuseaddr,  sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        return -1;
    }

    return fd;

}

int epoll_init()
{
    int epfd;

    epfd = epoll_create(1);
    if (epfd < 0) {
        perror("epoll_create");
        return -1;
    }

    return epfd;
}

int handle_read(int fd, int *length)
{
    int   ret;
    int   total;
    char *ptr;

    ptr = buffer;
    total = 0;

    while (1) {
        ret = recv(fd, ptr, BUFSIZE - 1, 0);
        if (ret > 0) {
            total += ret;
            if (total % (BUFSIZE - 1) == 0) {
                printf("%s", buffer);
                ptr = buffer;
            } else {
                ptr += ret;
            }
        }
        if (ret == 0) {
            printf("client close\n");
            *length = total;
            return -1;
        }

        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                *length = total;
                return EAGAIN;    
            }
        }
    }
}

void handle_write(int fd)
{
    char *info = "server received your data\n";
    write(fd, info, strlen(info));
    return;
}

int init_conn_sock(int listenfd)
{
    struct sockaddr_in cliaddr;
    socklen_t addrlen = sizeof(cliaddr);
    int    conn_sock;

    conn_sock = accept(listenfd, (struct sockaddr *)&cliaddr, 
            &addrlen);
    if (conn_sock < 0) {
        perror("accept");
        return -1;
    }

    fcntl(conn_sock, F_SETFL, 
            fcntl(conn_sock, F_GETFL) | O_NONBLOCK);
    
    return conn_sock;
}


int main()
{
    int ret;
    int i;
    int nfds;
    int conn_sock;
    int epfd;
    int listenfd;
    struct sockaddr_in sockaddr;
    struct epoll_event ev;
    struct event private_data;

    epfd = epoll_init();
    if (epfd < 0) {
        return -1;
    }
    
    listenfd = listenfd_init();

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(80);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(listenfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if (ret < 0) {
        perror("bind");
        return -1;
    }
    
    ret = listen(listenfd, 125);
    if (ret < 0) {
        perror("listen");
        return -1;
    }
    
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    if (ret < 0) {
        perror("epoll_ctl");
        return -1;
    }    

    while (1) {
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            perror("epoll_wait");
            return -1;
        }
         
        for (i = 0; i < nfds; i++) {
            if (events[i].data.fd == listenfd) {
                conn_sock = init_conn_sock(listenfd);
                private_data.fd = conn_sock;
                private_data.readcb = handle_read;
                private_data.writecb = handle_write;

                ev.events = EPOLLIN;
                ev.data.ptr = &private_data;

                epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &ev);
                continue;
            }
            
            if ((events[i].events & (EPOLLERR | EPOLLHUP)) 
                && (events[i].events & (EPOLLIN | EPOLLOUT)) == 0) {
                events[i].events |= EPOLLIN | EPOLLOUT;
            }

            if (events[i].events & EPOLLIN) {
                struct event *data;
                data = events[i].data.ptr;
                int    length = 0;
                if (data->readcb) {
                    ret = data->readcb(data->fd, &length);
                    if (data->writecb) {
                        data->writecb(data->fd);
                    }
                    if (ret == -1) {
                        close(data->fd);
                    }
                }
            } else {
                struct event *data;
                data = events[i].data.ptr;
                if (data->writecb) {
                    data->writecb(data->fd);
                }
            }
        }
    }
}

