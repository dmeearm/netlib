/*
 * Copyright (c) 2013 Jason ding <dmeearm@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

#include "net.h"
#include <stdio.h>

char buffer[BUFSIZE];

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
                    ret = data->readcb(data->fd, &length, buffer, BUFSIZE);
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

