#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <button_interface.h>

#define _1ms    1000

static void wait_press(void *object, Button_Interface *button)
{
    while (true)
    {
        if (!button->Read(object))
        {
            usleep(_1ms * 100);
            break;
        }
        else
        {
            usleep(_1ms);
        }
    }
}

bool Button_Run(void *object, char **argv, Button_Interface *button)
{
    char data[64];
    char buffer[BUFSIZ + 1];
    int fd_temp;
    int state = 0;
    
    if(button->Init(object) == false)
        return EXIT_FAILURE;
    

    memset(buffer, 0, sizeof(buffer));
    sscanf(argv[1], "%d", &fd_temp);
    const int fd = fd_temp;
    while(true)
    {
        wait_press(object, button);  

        state ^= 0x01;
        memset(data, 0, sizeof(data));
        snprintf(data, sizeof(data), "state = %d\n", state);
        write(fd, data, strlen(data));                
        usleep(500 * _1ms);
    }    
    
    close(fd);
        
    return 0;
}