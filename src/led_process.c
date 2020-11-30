#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <led.h>

#define _1ms    1000

int main(int argc, char const *argv[])
{    
    char buffer[BUFSIZ + 1];
    int fd;   
    int state_cur;
    int state_old;

    LED_t led =
    {
        .gpio.pin = 0,
        .gpio.eMode = eModeOutput
    };

    if (LED_init(&led))
        return EXIT_FAILURE;
    
    sscanf(argv[1], "%d", &fd);
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        read(fd, buffer, BUFSIZ);
        sscanf(buffer, "state = %d", &state_cur);        

        if (state_cur != state_old)
        {

            state_old = state_cur;
            LED_set(&led, (eState_t)state_cur);
        }
        usleep(1);
    }    
    
    close(fd);
    return 0;
}
