#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>



int main(int argc, char const *argv[])
{
    /*
        file_pipes[0] - read
        file_pipes[1] - write
    */
    int file_pipes[2];
    int fd_read;
    int fd_write;
    char args[BUFSIZ + 1];

    int ret = pipe(file_pipes);

    fd_read = file_pipes[0];
    fd_write = file_pipes[1];

    if(ret == 0)
    {
        int fork_res = fork();
        if(fork_res == 0)
        {
            close(fd_read);
            memset(args, 0, sizeof(args));
            sprintf(args, "%d", fd_write);
            (void)execl("button_process", "button_process", args, (char *)0);
            exit(EXIT_FAILURE);
        }
        else if(fork_res > 0)
        {
            fork_res = fork();
            if(fork_res == 0)
            {
                close(fd_write);
                memset(args, 0, sizeof(args));
                sprintf(args, "%d", fd_read);
                (void)execl("led_process", "led_process", args, (char *)0);
                exit(EXIT_FAILURE);
            }
        }
    }

    close(file_pipes[0]);
    close(file_pipes[1]);  
    
    return 0;
}
