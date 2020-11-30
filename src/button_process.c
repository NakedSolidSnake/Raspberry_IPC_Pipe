#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <button.h>

#define _1ms    1000

static Button_t button7 = {
        .gpio.pin = 7,
        .gpio.eMode = eModeInput,
        .ePullMode = ePullModePullUp,
        .eIntEdge = eIntEdgeFalling,
        .cb = NULL
    };

int main(int argc, char const *argv[])
{
    char data[64];
    char buffer[BUFSIZ + 1];
    int fd;
    int state = 0;
    
    if(Button_init(&button7))
        return EXIT_FAILURE;
    

    memset(buffer, 0, sizeof(buffer));
    sscanf(argv[1], "%d", &fd);
    while(1)
    {
        while(1)
        {
            if(!Button_read(&button7)){
                usleep(_1ms * 40);
                while(!Button_read(&button7));
                usleep(_1ms * 40);
                state ^= 0x01;
                break;
            }else{
                usleep( _1ms );
            }
        }   

        memset(data, 0, sizeof(data));
        snprintf(data, sizeof(data), "state = %d\n", state);
        write(fd, data, strlen(data));                
        usleep(500 * _1ms);
    }    
    
    close(fd);
        
    return 0;
}
