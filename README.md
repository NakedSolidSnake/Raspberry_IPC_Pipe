<p align="center">
  <img src="https://64.media.tumblr.com/tumblr_mbpqrwH1NK1qb8i73o1_500.gif"/>
</p>


# _Pipe_
## Introdução
_Pipe_ é um recurso IPC empregado para conectar a saída de um processo a entrada de um outro processo. _Pipe_ é largamente utilizado pelo CLI(_Command Line Interface_), como por exemplo em uma consulta simples listando todos os arquivos de um respectivo diretório e filtrando por arquivos de extensão \*.txt, normalmente usamos _ls_ seguido de _grep_, como demonstrado no comando abaixo:
```bash
$ ls | grep *.txt
```
O comando descrito transfere os dados de saída gerado pelo comando _ls_ que seriam apresentados no _stdout_  e são passados como argumentos de entrada(_stdin_) para o comando _grep_ que filtra o resultado e apresenta os arquivos que possuem a extensão. Em outras palavras o _Pipe_ permite que o _stdout_ do [Processo A] se conecte com o _stdin_ do [Processo B].

<p align="center">
  <img src="docs/images/pipe.png"/>
</p>

O fluxo da conexão obedece a seguinte ordem o _stdout_ se conecta com o _stdin_, porém fluxo contrário não é permitido, para isso é necessário estabelecer um conexão do processo B para o A.

### Criando um _Pipe_

Para criar um _pipe_ é utilizado a _system all_ 
```c
#include <unistd.h>

int pipe(int filedes[2]);
```

### _Pipe_ após um _fork_
O _fork_ é tem a característica de clonar todo o processo, e devido a isso tudo que for aberto pelo processo pai vai ser refletido no processo filho, sendo assim, se um _pipe_ for aberto no processo pai o processo filho também irá herdar o _pipe_, a figura a seguir pode representar essa situação:

<p align="center">
  <img src="docs/images/pipe-apos_fork.png"/>
</p>

Para garantir que o processo pai se comunicará com o processo filho na forma correta, é necessário fechar os descritores, de modo que o _stdout_ do pai fique conectado ao _stdin_ do filho, a imagem a seguir representa essa configuração:

<p align="center">
  <img src="docs/images/pipe-close_descriptors.png"/>
</p>

## Implementação
Para exemplificar o _Pipe_ foi criado uma aplicação no estilo cliente-servidor, onde o processo que controla o botão que é responsável pela requisição para a troca de estado do _LED_ se comporta como o cliente, e o processo que controla o _LED_ que é responsável pelo o controle do pino físico, ou seja, a alteração do estado efetivo do _LED_ se comporta como o server da aplicação; A aplicação é composta por três executáveis descritos a seguir:
* _launch_processes_ - processo responsável por lançar os processos _button_process_ e _led_process_ através da combinação _fork_ e _exec_
* _button_process_ - processo responsável por ler o GPIO em modo de leitura da Raspberry Pi e enviar o estado via _Pipe_ para o processo _led_process_
* _led_process_ - processo reponsável por controlar o GPIO em modo de escrita da Rasoberry e alterar o estado do pino de acordo com o _input_ fornecido pelo processo _button_process_.
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
