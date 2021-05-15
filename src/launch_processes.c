#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>



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
    pid_t led_process;
    pid_t button_process;
    int state;

    int ret = pipe(file_pipes);

    fd_read = file_pipes[0];
    fd_write = file_pipes[1];

    if(ret == 0)
    {
        button_process = fork();
        if(button_process == 0)
        {
            close(fd_read);
            memset(args, 0, sizeof(args));
            sprintf(args, "%d", fd_write);
            (void)execl("button_process", "button_process", args, (char *)0);
            exit(EXIT_FAILURE);
        }
        else if(button_process > 0)
        {
            led_process = fork();
            if(led_process == 0)
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

    wait(&state);

    kill(led_process, SIGKILL);
    kill(button_process, SIGKILL);
    
    return 0;
}
