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
int main (int argc, char **argv)
{
    int socket_descriptor;
    struct sockaddr_in address;
    
    int index;
    int c;
    
    char msg = 0;
    // 0 - no action
    // 1 - reboot system
    // 2 - shutdown system
    // 3 - kill application

    opterr = 0;

    while ((c = getopt (argc, argv, "rsk")) != -1)
        switch (c)
        {
        case 'r':
            msg = 1;
            break;
        case 's':
            msg = 2;
            break;
        case 'k':
            msg = 3;
            break;
        case '?':
            return 1;
        default:
            abort ();
        }

    //for (index = optind; index < argc; index++)
        //printf ("Non-option argument %s\n", argv[index]);
    
    socket_descriptor = socket (AF_INET, SOCK_DGRAM, 0);
    if (socket_descriptor == -1) 
    {
        perror ("socket()");
        exit (EXIT_FAILURE);
    }
    memset (&address, 0, sizeof (address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr ("224.0.33.33");
    address.sin_port = htons (port);
    while (1) 
    {
        if (sendto(socket_descriptor, &msg, sizeof (msg),
                    0, (struct sockaddr *) &address, sizeof (address)) < 0) 
        {
        perror ("sendto()");
        exit (EXIT_FAILURE);
        }
        sleep (10);
    }
    return EXIT_SUCCESS;
}
