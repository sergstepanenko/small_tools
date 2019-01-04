#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

int run_cmd(char *command)
{
        pid_t pid;
        char bashstr[] = "bash";
        char bashparamstr[] = "-c";
        char *const argv[] = {bashstr, bashparamstr, command, (char *) 0};
        int status;

        status = posix_spawn(&pid, "/bin/bash", NULL, NULL, argv, environ);
        if(status == 0)
        {
            if(waitpid(pid, &status, 0) == -1)
            {
                status = -1;
                printf("waitpid error\n");
            }
            else if(status != 0)
                printf("system cmd returns with %d:%s (%s)\n", status>>8, strerror(status>>8), command);
        }
        else
            printf("posix_spawn: %s\n", strerror(status));

        return status;
}

int main(int argc, char* argv[])
{
    run_cmd(argv[1]);
    return 0;
}
