<p align="center">
  <img src="https://64.media.tumblr.com/tumblr_mbpqrwH1NK1qb8i73o1_500.gif"/>
</p>


# _Pipes_

## Tópicos
* [Introdução](#introdução)
* [Criando Pipes](#criando-pipes)
* [Pipes após um fork](#pipes-após-um-fork)
* [Implementação](#implementação)
* [launch_processes.c](#launch_processesc)
* [button_interface.c](#button_interfacec)
* [led_interface.c](#led_interfacec)
* [Compilando, Executando e Matando os processos](#compilando-executando-e-matando-os-processos)
* [Compilando](#compilando)
* [Clonando o projeto](#clonando-o-projeto)
* [Selecionando o modo](#selecionando-o-modo)
* [Executando](#executando)
* [Interagindo com o exemplo](#interagindo-com-o-exemplo)
* [MODO PC](#modo-pc-1)
* [MODO RASPBERRY](#modo-raspberry-1)
* [Matando os processos](#matando-os-processos)
* [Conclusão](#conclusão)
* [Referências](#referências)

Antes de seguir esse artigo é imprescindível a instalação da biblioteca [hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware) caso queria utilizar o hardware da Raspberry.


## Introdução
_Pipes_ é um recurso IPC empregado para conectar a saída de um processo a entrada de um outro processo. _Pipes_ é largamente utilizado pelo CLI(_Command Line Interface_), como por exemplo em uma consulta simples listando todos os arquivos de um respectivo diretório e filtrando por arquivos de extensão \*.txt, normalmente usamos _ls_ seguido de _grep_, como demonstrado no comando abaixo:
```bash
$ ls | grep *.txt
```
O comando descrito transfere os dados de saída gerado pelo comando _ls_ que seriam apresentados no _stdout_  e são passados como argumentos de entrada(_stdin_) para o comando _grep_ que filtra o resultado e apresenta os arquivos que possuem a extensão. Em outras palavras o _Pipes_ permite que o _stdout_ do [Processo A] se conecte com o _stdin_ do [Processo B].

<p align="center">
  <img src="docs/images/pipe.png"/>
</p>

O fluxo da conexão obedece a seguinte ordem o _stdout_ se conecta com o _stdin_, porém fluxo contrário não é permitido, para isso é necessário estabelecer um conexão do processo B para o A.

## Criando _Pipes_

Para criar _pipes_ é utilizado a _system call_ 
```c
#include <unistd.h>

int pipe(int filedes[2]);
```

## _Pipes_ após um _fork_
O _fork_ é tem a característica de clonar todo o processo, e devido a isso tudo que for aberto pelo processo pai vai ser refletido no processo filho, sendo assim, se um _pipe_ for aberto no processo pai o processo filho também irá herdar o _pipe_, a figura a seguir pode representar essa situação:

<p align="center">
  <img src="docs/images/pipe-apos_fork.png"/>
</p>

Para garantir que o processo pai se comunicará com o processo filho na forma correta, é necessário fechar os descritores, de modo que o _stdout_ do pai fique conectado ao _stdin_ do filho, a imagem a seguir representa essa configuração:

<p align="center">
  <img src="docs/images/pipe-close_descriptors.png"/>
</p>

## Implementação
Para exemplificar _Pipes_ foi criado uma aplicação no estilo cliente-servidor, onde o processo que controla o botão é responsável pela requisição da mudança de estado do _LED_ comportando-se como cliente, e o processo que controla o _LED_ é responsável pelo o controle do pino físico, ou seja, a alteração do estado efetivo do _LED_ comportando-se como servidor da aplicação; A aplicação é composta por três executáveis descritos a seguir:
* _launch_processes_ - processo responsável por lançar os processos _button_process_ e _led_process_ através da combinação _fork_ e _exec_
* _button_interface_ - processo responsável por ler o GPIO em modo de leitura do Raspberry Pi e enviar o estado via _Pipe_ para o processo _led_interface_
* _led_interface_ - processo reponsável por controlar o GPIO em modo de escrita do Raspberry e alterar o estado do pino de acordo com o _input_ fornecido pelo processo _button_interface_.
### launch_processes.c
Primeiro são declaradas variáveis para a criação do _pipe_ e duas variáveis para identificar quem é o _stdout_ e o _stdin_, e é chamada a _system call_ de criação do _pipe_
```c
int file_pipes[2];
int fd_read;
int fd_write;
char args[BUFSIZ + 1];

int ret = pipe(file_pipes);
```
Após a criação é feita a atribuição para as variáveis representando a função de cada índice, onde 0 representa o canal de leitura e 1 o canal de escrita
```c
fd_read = file_pipes[0];
fd_write = file_pipes[1];
```
Para que o _fork_ seja realizado é necessário verificar se o _pipe_ foi criado com sucesso, caso o retorno seja igual a zero, o fluxo continua
```c
if(ret == 0)
```
Com o _pipe_ criado, clonamos o processo e no processo filho fechamos o descritor referente ao de leitura, e passamos como argumento o descritor de escrita para o processo _button_interface_
```c
int fork_res = fork();
if(fork_res == 0)
{
    close(fd_read);
    memset(args, 0, sizeof(args));
    sprintf(args, "%d", fd_write);
    (void)execl("button_process", "button_process", args, (char *)0);            
    exit(EXIT_FAILURE);
}
```
De volta ao processo pai verificamos se o _fork_ anterior foi executado com sucesso, caso sim é iniciada uma nova cópia, porém dessa vez o descritor de escrita é fechado e passamos o descritor de leitura como argumento para o processo _led_interface_
```c
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
```
Por fim fechamos os descritores do processo pai
```c

close(file_pipes[0]);
close(file_pipes[1]);  
```

### button_interface.c

Declaramos um buffer para formatar a mensagem de envio, e declaramos uma variável para receber o descritor recebido via argumento
```c
char data[64];
char buffer[BUFSIZ + 1];
int fd;
int state = 0;
```
Configuramos o botão, passando o descritor como argumento
```c    
if(button->Init(object) == false)
        return EXIT_FAILURE;
```
Limpamos o buffer e copiamos o valor de descritor para o buffer, e recuperamos o descritor do pipe em fd
```c
memset(buffer, 0, sizeof(buffer));
sscanf(argv[1], "%d", &fd);
```
O loop a seguir aguarda que o botão seja pressionado para que a variável *state* seja alterada, se estiver em 0 vai para 1 e vice-versa
```c 
wait_press(object, button);
state ^= 0x01;
```
Assim que o botão for pressionado, inicia-se a escrita no _pipe_ como o novo estado, em seguida é chamado a _system call write_ para enviar a mensagem através do _pipe_
```c
memset(data, 0, sizeof(data));
snprintf(data, sizeof(data), "state = %d\n", state);
write(fd, data, strlen(data));                
usleep(500 * _1ms);
```
Fechamos o descritor de escrita no fim da aplicação
```c
close(fd);
```

### led_interface.c
O processo de configuração se assemelha ao do processo de botão
```c 
char buffer[BUFSIZ + 1];
int fd;   
int state_cur;
int state_old;

```
Aplica a inicialização do LED de acordo com o descritor, e recupera o valor do descritor de leitura do _pipe_
```c
if (led->Init(object) == false)
    return EXIT_FAILURE;

sscanf(argv[1], "%d", &fd);
```
Realiza a leitura do _pipe_ via _polling_ para verificar se houve alteração do estado, se houve atualiza a variável de estado e aplica o novo estado ao LED
```c
memset(buffer, 0, sizeof(buffer));
read(fd, buffer, BUFSIZ);
sscanf(buffer, "state = %d", &state_cur);        

if (state_cur != state_old)
{

    state_old = state_cur;
    led->Set(object, (uint8_t)state_cur);
}
usleep(1);    
```
Por fim fechamos o descritor de leitura ao fim da aplicação
```c    
close(fd);
```

Para o código fonte completo clique [aqui](https://github.com/NakedSolidSnake/Raspberry_IPC_Pipe)

## Compilando, Executando e Matando os processos
Para compilar e testar o projeto é necessário instalar a biblioteca de [hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware) necessária para resolver as dependências de configuração de GPIO da Raspberry Pi.

## Compilando
Para faciliar a execução do exemplo, o exemplo proposto foi criado baseado em uma interface, onde é possível selecionar se usará o hardware da Raspberry Pi 3, ou se a interação com o exemplo vai ser através de input feito por FIFO e o output visualizado através de LOG.

### Clonando o projeto
Pra obter uma cópia do projeto execute os comandos a seguir:

```bash
$ git clone https://github.com/NakedSolidSnake/Raspberry_IPC_Pipe
$ cd Raspberry_IPC_Pipe
$ mkdir build && cd build
```

### Selecionando o modo
Para selecionar o modo devemos passar para o cmake uma variável de ambiente chamada de ARCH, e pode-se passar os seguintes valores, PC ou RASPBERRY, para o caso de PC o exemplo terá sua interface preenchida com os sources presentes na pasta src/platform/pc, que permite a interação com o exemplo através de FIFO e LOG, caso seja RASPBERRY usará os GPIO's descritos no [artigo](https://github.com/NakedSolidSnake/Raspberry_lib_hardware#testando-a-instala%C3%A7%C3%A3o-e-as-conex%C3%B5es-de-hardware).

#### Modo PC
```bash
$ cmake -DARCH=PC ..
$ make
```

#### Modo RASPBERRY
```bash
$ cmake -DARCH=RASPBERRY ..
$ make
```

## Executando
Para executar a aplicação execute o processo _*launch_processes*_ para lançar os processos *button_process* e *led_process* que foram determinados de acordo com o modo selecionado.

```bash
$ cd bin
$ ./launch_processes
```

Uma vez executado podemos verificar se os processos estão rodando atráves do comando 
```bash
$ ps -ef | grep _process
```

O output 
```bash
pi        4725     1  1 23:53 pts/0    00:00:00 button_process 4
pi        4726     1  0 23:53 pts/0    00:00:00 led_process 3
```
Aqui é possível notar que o button_process possui um argumento com o valor 4, e o led_process possui também um argumento com o valor 3, esses valores representam os descritores gerados pelo _pipe system call_, onde o 4 representa o descritor de escrita e o 3 representa o descritor de leitura.

## Interagindo com o exemplo
Dependendo do modo de compilação selecionado a interação com o exemplo acontece de forma diferente

### MODO PC
Para o modo PC, precisamos abrir um terminal e monitorar os LOG's
```bash
$ sudo tail -f /var/log/syslog | grep LED
```

Dessa forma o terminal irá apresentar somente os LOG's referente ao exemplo.

Para simular o botão, o processo em modo PC cria uma FIFO para permitir enviar comandos para a aplicação, dessa forma todas as vezes que for enviado o número 0 irá logar no terminal onde foi configurado para o monitoramento, segue o exemplo
```bash
$ echo "0" > /tmp/pipe_file
```

Output do LOG quando enviado o comando algumas vezez
```bash
Apr  3 20:56:19 cssouza-Latitude-5490 LED PIPE[22810]: LED Status: On
Apr  3 20:56:20 cssouza-Latitude-5490 LED PIPE[22810]: LED Status: Off
Apr  3 20:56:21 cssouza-Latitude-5490 LED PIPE[22810]: LED Status: On
Apr  3 20:56:34 cssouza-Latitude-5490 LED PIPE[22810]: LED Status: Off
Apr  3 20:56:50 cssouza-Latitude-5490 LED PIPE[22810]: LED Status: On
Apr  3 20:56:51 cssouza-Latitude-5490 LED PIPE[22810]: LED Status: Off
Apr  3 20:56:51 cssouza-Latitude-5490 LED PIPE[22810]: LED Status: On
Apr  3 20:56:52 cssouza-Latitude-5490 LED PIPE[22810]: LED Status: Off
```

### MODO RASPBERRY
Para o modo RASPBERRY a cada vez que o botão for pressionado irá alternar o estado do LED.

## Matando os processos
Para matar os processos criados execute o script kill_process.sh
```bash
$ cd bin
$ ./kill_process.sh
```

## Conclusão
_Pipe_ é um IPC muito utilizado no shell, porém como opção de uso de comunicação entre processos que possuem tempo de vida indeterminado não muito viável, por não permitir o fluxo de dados de forma bidirecional, e pela dificuldade de manter os descritores de leitura e escrita.

## Referências
* [Mark Mitchell, Jeffrey Oldham, and Alex Samuel - Advanced Linux Programming](https://www.amazon.com.br/Advanced-Linux-Programming-CodeSourcery-LLC/dp/0735710430)
* [fork, exec e daemon](https://github.com/NakedSolidSnake/Raspberry_fork_exec_daemon)
* [biblioteca hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware)
