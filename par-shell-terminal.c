#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h> 
#include <sys/wait.h>
#include <signal.h>

#define INPUT_MAX 80
#define STR_BUF 100
#define MAX_BUF 1024
#define MAX_PID 20

int fd;
int fdstats;

char buf[MAX_BUF];  
char pathstats[STR_BUF];
char pidprocesso[MAX_PID];
char pipeserver[MAX_PID];
char str[INPUT_MAX];
char strstats[STR_BUF];


void faz_exit() {			// função que envia o aviso para o servidor a avisar que o cliente fez exit para que este seja removido da lista
	if(sprintf(pidprocesso, "exit %d\n", getpid()) < 0){ 		// cria nome do ficheiro do processo com o respetivo PID
		perror("Falhou a imprimir a string");
	}
	if((write(fd, pidprocesso , strlen(pidprocesso))) == -1){   	// volta a enviar o pid para o servidor para este fazer o remove da lista
    	perror("Falhou a escrita no pipe");
    }
    if(printf("\nExit\n") < 0){
    	perror("Falhou a imprimir a string");
    }
    close(fd);
    exit(EXIT_SUCCESS);							    	// Termina o processo 
}

void INT_handler(int sign){  // Faz o tratamento do Ctrl-c
    signal(sign, SIG_IGN);
    faz_exit();
    exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[]) {

	if(signal(SIGINT, INT_handler) == SIG_ERR){ // Detecta o Ctrl-c
        if((printf("\ncan't catch SIGINT\n")) < 0) {
        	perror("Falhou a imprimir a string");
        }   
    }

    sprintf(pipeserver, "/tmp/%s", argv[1]); 	// liga-se ao pipe do servidor que é dado no argv[1] (par-shell-in)

    char * myfifo = pipeserver;					// Cria o named pipe que recebe comandos

    if((fd = open(myfifo, O_WRONLY)) < 0 ) {
        perror("Falhou a abertura do pipe");
    }

    else {
    	if(printf("Ligado à par-shell\n") < 0){
    		perror("Falhou a imprimir a string");
    	}
    }   

	if (sprintf(pidprocesso, "PID: %d\n", getpid()) < 0) {  	//string para envio do pid ao par-shell
		exit(EXIT_FAILURE);
	}

	if (write(fd, pidprocesso , strlen(pidprocesso)) < 0) { 	// Envio do PID para o servidor
		perror("Falhou a escrita no pipe");
		exit(EXIT_FAILURE);
	}

	// Ciclo principal

	while(1) {  

		if(printf( "Instrucao: ") < 0){
			perror("Falhou a imprimir a string");
		}

	   	if((fgets( str , INPUT_MAX - 1 , stdin)) == NULL) {	// guarda cada linha em str ate chegar ao fim do stdin 
	   		perror("Atingiu o EOF sem ler nenhum caracter");
	   	}

		if (strncmp(str,"exit\n", 5) == 0) { 				// so compara os primeiros 4 bytes entre o str e exit
			faz_exit();   
        }

        else if (strncmp(str,"stats", 5) == 0) { 			// so compara os primeiros 5 bytes entre o str e o stats

			if (sprintf(pathstats, "/tmp/%d", getpid()) < 0) {  //cria o path com o nome do pid, o que faz com que seja um path unico
				perror("Falhou a imprimir a string");
				exit(EXIT_FAILURE);
			}

			unlink(pathstats);

			if (mkfifo(pathstats, 0666) == -1) {
				perror("Falhou a criar o pipe");
				exit(EXIT_FAILURE);
			}
			
			if (sprintf(strstats, "stats %d\n", getpid()) == -1) {
				perror("Falhou a imprimir a string");
				exit(EXIT_FAILURE);
			}
			if (write(fd, strstats, strlen(strstats)) < 0) {
				perror("Falhou a imprimir a string");
			    exit(EXIT_FAILURE);
			}

			// Leitura

			if((fdstats = open(pathstats, O_RDONLY)) < 0) {
				perror("Falhou a abrir o pipe");
				exit(EXIT_FAILURE);
			}
			
			if (read(fdstats, buf, MAX_BUF) < 0) {
			    perror("Falhou a leitura do pipe");
			    exit(EXIT_FAILURE);
            }

            if(printf("%s\n", buf) < 0) {
            	perror("Falhou a imprimir a string");
            }

            close(fdstats);
			unlink(pathstats);
        }

        else if (strlen(str) == 0) {
            printf("No arguments\n");
        }

        else if (strncmp(str,"PID:", 4) != 0) { // Nao deixa enviar ao servidor mais nenhuma autenticação de processo par shell terminal 

            int tamanhowrite = write(fd, str , strlen(str)); // escreve o numero de caract (arg 3) do buffer str, associado ao filedescriptor fd
            if(tamanhowrite < 0) {   
                perror("Falhou a escrita no pipe"); 
            }
        }    
	}
}


