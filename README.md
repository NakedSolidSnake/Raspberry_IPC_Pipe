<p align="center">
  <img src="https://64.media.tumblr.com/tumblr_mbpqrwH1NK1qb8i73o1_500.gif"/>
</p>


# _Pipe_
## Introdução
_Pipe_ é um recurso empregado para conectar a saída de um processo a entrada de um outro processo. _Pipe_ é largamente utilizado pelo CLI(_Command Line Interface_), como por exemplo em uma consulta simples listando todos os arquivos do respectivo diretório e filtrando por arquivos com extensão \*.txt, normalmente usamos _ls_ seguido de _grep_, como demonstrado no comando abaixo:
```bash
$ ls | grep *.txt
```
O comando descrito transfere os dados de saída gerado pelo comando _ls_ que seriam apresentados no _stdout_  e são passados como argumentos de entrada(_stdin_) para o comando _grep_ que filtra o resultado e apresenta os arquivos que possuem a extensão. Em outras palavras o _Pipe_ permite que o _stdout_ do processo A se conecte com o _stdin_ do processo B.

<p align="center">
  <img src="docs/images/pipe.png"/>
</p>

## Implementação
### launch_processes.c
```c
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

    if(ret == 0){
        int fork_res = fork();
        if(fork_res == 0){
            close(fd_read);
            memset(args, 0, sizeof(args));
            sprintf(args, "%d", fd_write);
            (void)execl("button_process", "button_process", args, (char *)0);
            exit(EXIT_FAILURE);
        }else if(fork_res > 0){
            fork_res = fork();
            if(fork_res == 0){
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

```
### button_process.c
```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <button.h>

#define _1ms    1000

static Button_t button = {
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
    
    if(Button_init(&button))
        return EXIT_FAILURE;
    

    memset(buffer, 0, sizeof(buffer));
    sscanf(argv[1], "%d", &fd);
    while(1)
    {
        while(1)
        {
            if(!Button_read(&button)){
                usleep(_1ms * 40);
                while(!Button_read(&button));
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

```
### led_process.c
```c
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

```

## Conclusão
