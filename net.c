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

int handle_read(int fd, int *length, char *buffer, int size)
{
    int   ret;
    int   total;
    char *ptr;

    ptr = buffer;
    total = 0;

    while (1) {
        ret = recv(fd, ptr, size - 1, 0);
        if (ret > 0) {
            total += ret;
            if (total % (size - 1) == 0) {
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


