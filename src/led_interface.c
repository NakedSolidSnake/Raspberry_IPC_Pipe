#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <led_interface.h>

#define _1ms    1000



bool LED_Run(void *object, char **argv, LED_Interface *led)
{
    char buffer[BUFSIZ + 1];
    int fd_temp;    
    int state_cur;
    int state_old;
    
    if (led->Init(object) == false)
        return EXIT_FAILURE;
    
    sscanf(argv[1], "%d", &fd_temp);
    const int fd = fd_temp;

    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        read(fd, buffer, BUFSIZ);
        sscanf(buffer, "state = %d", &state_cur);
        if (state_cur != state_old)
        {
            state_old = state_cur;
            led->Set(object, (uint8_t)state_cur);
        }
        usleep(1);
    }    
    
    close(fd);
    return 0;
}

