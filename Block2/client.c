/*
** client.c -- a stream socket client
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAXDATASIZE 512 // max number of bytes we can get at once

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 3) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM; //specifies Sock type to sequenced, reliable connection-based stream

    //print error message if connection not possible
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); //gai_strerror converts error int to error message
        return 1;
    }

    /*helper function to display different IP addresses:
    for(p = servinfo; p != NULL; p = p->ai_next) {
        char buffer[1024];
        getnameinfo(p->ai_addr, p->ai_addrlen, buffer, sizeof(buffer), NULL, 0, NI_NUMERICHOST);
        printf("Addr:%s\n",buffer);
    }
     */

    // loop through all the results and connect to the first we can (DNS server returns multiple IP adresses in different order to do load balancing)
    for (p = servinfo; p != NULL; p = p->ai_next) {
        //build sockfd
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        //connect to sockfd
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    //read bytes from socket into buf - deletes from socket after read to buf?
    while ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) > 0) {
        buf[numbytes] = '\0';
        //print out the quote that was read into buf
        printf("%s", buf);
    }
    if (numbytes == -1) {
        perror("recv");
        exit(1);
    }


    close(sockfd);

    return 0;
}