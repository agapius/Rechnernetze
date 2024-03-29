/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include<time.h>
#define BACKLOG 10   // how many pending connections queue will hold

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//check if file is present and if its not empty
check_file(const char *filename){
    FILE *f;
    int lines=0;
    char ch;
    //check if file is found
    f = fopen(filename, "r");
    if(f==NULL){
        perror("Error while reading file!");
        exit(1);
    }
    //count number of lines in the text file to check if file is empty
    while((ch=fgetc(f))!=EOF)
    {
        if (ch=='\n') { lines++; }
    }
    if(lines==0){
        perror("Empty file!");
        exit(1);
    }
    fclose(f);
}


//get random quote
/* Returns a random line (w/o newline) from the file provided */
char* get_random_quote(const char *filename, size_t *len) {
    FILE *f;
    int lines=0;
    char ch;

    //count number of lines in the text file
    f = fopen(filename, "r");
    while((ch=fgetc(f))!=EOF)
    {
        if (ch=='\n') { lines++; }
    }
    fclose(f);

    //get random line from the number of lines
    srand(time(0));
    int randomline = rand() % (lines);

    size_t lineno = 0;
    size_t selectlen;
    char selected[256]; /* Arbitrary, make it whatever size makes sense */
    char current[256];
    selected[0] = '\0'; /* Don't crash if file is empty */

    //choose selected quote
    f = fopen(filename, "r");
    int i = 0;
    while(i <= randomline){
        fgets(current, sizeof(current), f);
        strcpy(selected, current);
        i++;
    }
    fclose(f);

    //'cuts' the array by replacing the \n announcing the end of the line/quote with a null termination
    selectlen = strlen(selected);
    if (selectlen > 0 && selected[selectlen-1] == '\n') {
        selected[selectlen-1] = '\0';
    }
    *len = selectlen;

    return strdup(selected);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    check_file(argv[2]);
    size_t len;
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // hier wichtig ai passive zu setzen, damit server nicht nur localhost entgegennimmt (ermittelt welche ip adresse ich benutzen möchte (für bind))

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) { //getaddrinfo gibt uns eigene ip und port
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
    //bind: mit bind belege ich einen port und eine ip (server hat mögl. 20 netzwerkkarten) funtioniert nicht, wenn port schon belegt ist
    //server läuft auf maschine mit ip adressen und ports - das programm server möchte sich jetzt an eine ip und einen port verbinden
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    //Im Backlog steht die zahl, wie viele Adressen mein Server annehmen kann.
    // Es wird zwar immer nur eine angenommen, aber ggf werden die anderen in eine cue gepackt.
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, get_random_quote(argv[2], &len), len, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}
