/* server.c */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static int port = 33333;
int main(int argc, char **argv)
{
    int socket_descriptor;
    struct sockaddr_in address;
        
    if(argv[1])
    {
        printf("%s\n", argv[1]);
    
        socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
        if(socket_descriptor == -1) 
        {
            perror("socket()");
            exit(EXIT_FAILURE);
        }
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("224.0.33.33");
        address.sin_port = htons(port);

        if (sendto(socket_descriptor, argv[1], strlen(argv[1]),
                    0, (struct sockaddr *) &address, sizeof(address)) < 0) 
        {
        perror("sendto()");
        exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}
