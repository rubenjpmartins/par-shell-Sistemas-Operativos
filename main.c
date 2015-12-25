/*******************************************************************************************
 * 	PROJETO SO - PARTE 5                                                			
 *                                                                     	    
 *	GRUPO 43							
 *									
 *	76832 - Duarte Clara 						
 *	78040 - Sofia Reis 						
 *	79532 - Rúben Martins 						
 *									
 *	Par-shell: Programa que simula uma shell que permite 		
 *			   executar e monitorizar lotes de programas  	
 *			   em paralelo numa	maquina multi-core.	
 * 
 ******************************************************************************************/

#include "list.h"
#include "commandlinereader.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define MAXARGS 6
#define MAXPAR 8
#define TAMANHOBUFFER 100

// Variaveis Globais

int num_child_proc = 0; 
int dadoexit = 0;       // variavel dadoexit para a outra tarefa saber que foi dado o exit; caso seja 1 foi dado exit
int iteracao, pid__,executiontime__;
int logvazio = 0;         // variável logvazio que indica para a outra tarefa se o ficheiro está vazio ou não
int fich_out; // Ficheiro output dos processos-filho 
int conta;
int num_symbols;
long executiontime = 0; 

char* cmd[MAXARGS];     // recebe ate MAXARGS argumentos
char buffer[TAMANHOBUFFER];   // Guarda os caracteres do standard input 

FILE *textlog; // ficheiro output

list_t *list;  
list_p *listpid;    

pthread_mutex_t trinco_dadoexit, trinco_num_child_proc , trinco_list;  
pthread_cond_t vazio,cheio;


// Signal handler

void INT_handler(int sign){

    signal(sign, SIG_IGN); // Vai ignorar se forem detectados mais contrl + c
    
    lst_matapid(listpid); // envia o signal para matar os terminais
    lst_destroypid(listpid); // faz free da lista dos pids

    if(printf("Exit!\n") < 0) {
    	perror("Falhou a imprimir a string");
    }
    exit(EXIT_SUCCESS);
}


// Funcao monitora

void *monitora(void *param) {

    int status;

    while(1) {

        pthread_mutex_lock(&trinco_num_child_proc);
        
        if ( num_child_proc > 0) {
            pthread_mutex_unlock(&trinco_num_child_proc);
            pid_t pid = wait(&status);      // Espera pela terminacao de 1 qualquer processo filho

            if (pid == -1) {
                perror("Falhou a espera pelos filhos");
            }
            
            else {
                
                pthread_mutex_lock(&trinco_num_child_proc);
                num_child_proc--;
                conta--;  
                pthread_cond_signal(&vazio);
                pthread_mutex_unlock(&trinco_num_child_proc);
                
                if (WIFEXITED(status)) {   // Verifica se o processo filho terminou normalmente

                    pthread_mutex_lock(&trinco_list);
                    update_terminated_process(list, pid, WEXITSTATUS(status), time(NULL));  // Insere o processo na lista

                    if ( WEXITSTATUS(status) == 0 )         // caso corra um fibonacci 
                    {
                        if (textlog == NULL){
                            perror("Nao foi possivel abrir o ficheiro de output log.txt"); 
                            exit(EXIT_FAILURE);
                        }

                        if (iteracao == 0){
                            if(fprintf(textlog, "iteracao %d", iteracao) == -1){
                                perror("Falhou a escrita");
                                exit(EXIT_FAILURE);
                            }
                            iteracao++;
                        }
                        else{
                            
                            if(fprintf(textlog, "\niteracao %d", iteracao) == -1){
                                perror("falhou a escrita");
                                exit(EXIT_FAILURE);
                            }
                            iteracao++;
                        }


                        if(fprintf(textlog, "\npid: %d execution time: %ld s", pid,  lst_tempodeexecucao(list, pid) ) == -1){
                            perror("Falhou a escrita");
                            exit(EXIT_FAILURE);
                        }

                        executiontime=executiontime + lst_tempodeexecucao(list, pid);

                        if(fprintf (textlog, "\ntotal execution time: %ld s", executiontime ) == -1){
                            perror("Falhou a escrita");
                            exit(EXIT_FAILURE);
                        }

                        if (fflush(textlog) != 0){
                        	perror("Falhou o flush do buffer");
                        }
                    }                    
                    pthread_mutex_unlock(&trinco_list);     
                }
            }
        }

        else {                      // Se não ha processos filho
            pthread_mutex_unlock(&trinco_num_child_proc);
            pthread_mutex_lock(&trinco_dadoexit);

            if (dadoexit == 1) {  // detecta quando é dado do exit na tarefa principal

                pthread_mutex_lock(&trinco_num_child_proc);
                if (num_child_proc > 0) {
                    pthread_mutex_unlock(&trinco_num_child_proc);
                    continue;
                }
                else {
                    pthread_mutex_unlock(&trinco_dadoexit);
                    pthread_mutex_unlock(&trinco_num_child_proc);
                    pthread_exit(NULL);
                }
            }
            else {
                pthread_mutex_unlock(&trinco_dadoexit);
            } 

            pthread_mutex_lock(&trinco_num_child_proc);
                while (conta == 0) {
                    pthread_cond_wait(&cheio, &trinco_num_child_proc);
                }
            pthread_mutex_unlock(&trinco_num_child_proc);
        }
    }
}


// MAIN ************************************************************

int main(int argc, char *argv[]) {

    char str_outputs[30]; // string que guarda cada nome dos ficheiros de output 
    char statsimprime[100]; // string que guarda os stats a enviar
    char statspid[20]; // string que guarda o directorio do pip no caso do stats
    
    list = lst_new();
    listpid = lst_new_p();
    
    if(signal(SIGINT, INT_handler) == SIG_ERR){
        if(printf("\nNao consegui detectar o SIGINT\n") < 0){
            perror ("Falhou a imprimir a string");
        }
    }

    textlog = fopen("log.txt","r");      // Abre log.txt em modo read
    
    if( textlog == NULL){ // n existe log.txt  
        if(fprintf(stderr, "Nao existe o ficheiro log.txt - Continua\n") < 0){
            perror ("Falhou a imprimir a string");
        }
    }

    else{
        if(printf("Existe log.txt - Ficheiro aberto\n") < 0){
            perror ("Falhou a imprimir a string");
        }

        logvazio = 1; // o ficheiro log nao esta vazio 

        int ordem = 0;

        while (fgets(buffer, TAMANHOBUFFER, textlog) != '\0') {      // guarda cada linha em buffer ate chegar ao fim do log.txt 
    
            if ((sscanf(buffer, "iteracao %d\n" , &iteracao)) == 1 && ordem == 0) {
                ordem = 1;
                iteracao++;
            }
            
            else if ((sscanf(buffer ,"pid: %d execution time: %d s\n", &pid__, &executiontime__)) == 2 && ordem ==1) {
                ordem = 2;
            }

            else if (sscanf(buffer ,"total execution time:%lds\n", &executiontime) == 1 && ordem ==2) {
                ordem = 0;
            }

            else {
                if(printf("Detectou linha errada\n") < 0){
                	perror ("Falhou a imprimir a string");
                }
                exit(EXIT_FAILURE);
            }   
        }
    }

    // Variaveis de condicao

    if(pthread_cond_init(&vazio, NULL) != 0) {
        perror("pthread_cond_init failed");
    }

    if(pthread_cond_init(&cheio, NULL) != 0) {
        perror("pthread_cond_init failed");
    }
    
    if(pthread_mutex_init(&trinco_dadoexit, NULL) != 0) {
        perror("pthread_mutex_init failed");
    }
    if(pthread_mutex_init(&trinco_num_child_proc, NULL) != 0) {
        perror("pthread_mutex_init failed");
    }
    if(pthread_mutex_init(&trinco_list, NULL) != 0) {
        perror("pthread_mutex_init failed");
    }

    pthread_t threadmonitora;

    if(printf("<<START>>\n") < 0) {
        perror ("Falhou a imprimir a string");
    };

    if (pthread_create(&threadmonitora, NULL, monitora, NULL)!=0) {
        perror("pthread_create failed"); // nao cria tarefa
        exit(EXIT_FAILURE);
    }

    textlog = fopen("log.txt", "a");       // Abre log.txt em modo append

    int fd;
    char * myfifo = "/tmp/par-shell-in";

    /* remove o pipe que foi passado como argumento */
    unlink(myfifo);

    /* create the FIFO (named pipe) */
    if(mkfifo(myfifo, 0666) != 0){       // recebe path e 0 - para ficar em octal(0 a 7) ; 666 for read and write user, group, others (?666 eh coisa do diabo)
    	perror("Falhou a criacao do pipe");
    }

    /* open, read, and display the message from the FIFO */
    if((fd = open(myfifo, O_RDONLY)) == -1) {    //faz open da fila, para leitura
    	perror("Falhou a abertura do pipe");
    }

    if(close(0) == -1) {   // fecha o stdin
    	perror("Falhou o fecho do stdin");
    }

   if(dup(fd) == -1) {   // make pipe go to stdin
  		perror("Falhou o redireccionamento do pipe");
  	}

    // CICLO Principal

    while(1) {
        num_symbols = readLineArguments( cmd, MAXARGS );

        if (num_symbols == -1) {
			if((fd = open(myfifo, O_RDONLY)) == -1){
				perror("Falhou a abertura do pipe");
			}
		}

        else if (num_symbols == 0) {
            if(printf("No arguments\n") < 0) {
                perror ("Falhou a imprimir a string");
            }
        }

        else if ((strcmp(cmd[0],"PID:")) == 0 ){            
  
            int val = atoi(cmd[1]); // transforma o pid em inteiro

            insert_new_pid(listpid, val); // insere na lista  
        }

         else if ((strcmp(cmd[0],"exit")) == 0 ){ 

            int val = atoi(cmd[1]); // transforma o pid em inteiro
            
            if(printf("O terminal cliente com o pid %d fez exit\n",val) < 0) {
            	perror ("Falhou a imprimir a string");
            }

            if((lst_removepid(listpid, val)) == -1) {		// A lista esta vazia, forca o pipe a manter-se aberto
               fd = open(myfifo, O_RDONLY);           
            }
        }

        else if ((strcmp(cmd[0],"stats")) == 0 ){ 
            
            int fdstats;
            int val = atoi(cmd[1]); // transforma o pid em inteiro

            if(sprintf(statspid, "/tmp/%d", val)< 0) {
            	perror("Falhou a escrita na string");
            }

            char * myfifostats = statspid;
                        
            if ((fdstats = open(myfifostats, O_WRONLY)) < 0) {
				perror("Falhou a abertura do pipe");
			}

            pthread_mutex_lock(&trinco_num_child_proc); // seccao critica onde é lida a variavel global conta e executiontime
            if (sprintf(statsimprime, "Number of processes runing: %d    Total execution time: %ld", conta, executiontime) < 0){ // cria nome do ficheiro do processo com o respetivo PID
            	perror("Falhou a escrita na string");
            }

            pthread_mutex_unlock(&trinco_num_child_proc);
             
            int tamanhowrite = write(fdstats,statsimprime, sizeof(statsimprime)); // escreve o numero de caract (arg 3) do buffer, associado ao filedescriptor fdstats

            if(tamanhowrite < 0) {         // sucesso: returns the number of bytes actually written to the file associated with fdstats (sempre menor que sizeof"Hi"). retorna -1 CC
               perror("Falhou a escrita"); 
            }
        }

        else if ((strcmp(cmd[0],"exit-global") == 0)) {          // Caso o argumento for exit, termina a par-shell de forma ordeira

            pthread_mutex_lock(&trinco_dadoexit);
            dadoexit=1;                                 // mete a variavel dadoexit para a outra tarefa saber que foi dado o exit
            pthread_mutex_unlock(&trinco_dadoexit);

            pthread_mutex_lock(&trinco_num_child_proc);
            conta++;                                    // Quando é dado o exit, desbloqueia a tarefa monitora
            pthread_cond_signal(&cheio);

            pthread_mutex_unlock(&trinco_num_child_proc);

            pthread_join(threadmonitora, NULL);     // fica a espera que a tarefa termine

            lst_print(list);                        
            lst_destroy(list);          

            // Destroi Trincos

            if(pthread_mutex_destroy(&trinco_dadoexit)!= 0) {
                perror("pthread_mutex_destroy failed");
            }
            if(pthread_mutex_destroy(&trinco_num_child_proc) != 0) {   
                perror("pthread_mutex_destroy failed"); 
            }

            if(pthread_mutex_destroy(&trinco_list) != 0) {       
                perror("pthread_mutex_destroy failed");
            }

            // Destroi Variaveis de condicao  
                    
            if(pthread_cond_destroy(&vazio) != 0) {   
                perror("pthread_cond_destroy failed"); 
            }

            if(pthread_cond_destroy(&cheio) != 0) {   
                perror("pthread_cond_destroy failed"); 
            }

            if(fclose(textlog) != 0) {
            	perror("Falhou o fecho do ficheiro");
            }
            
            if(close(fich_out) != 0) {
            	perror("Falhou o fecho do ficheiro");
            }

            lst_matapid(listpid); // mata todos os processos terminal que estejam a correr
			lst_destroypid(listpid); // liberta memoria associada 'a lista

            if(close(fd) != 0) {
            	perror("Falhou o fecho do pipe");
            }
    
            /* remove the FIFO */
            unlink(myfifo);

            exit(EXIT_SUCCESS);                     // Termina o processo pai
            return 0;
        }

        else {
            pthread_mutex_lock(&trinco_num_child_proc);

            while (conta == MAXPAR) {
                pthread_cond_wait(&vazio, &trinco_num_child_proc);
            } 
            
            pthread_mutex_unlock(&trinco_num_child_proc); 
            pid_t pid = fork();         // Cria um processo-filho

            if (pid < 0) {               // Quando o pid é negativo, falhou a criar um processo filho
                perror("FORK FAILED");
                exit(EXIT_FAILURE);
            }

            else if (pid == 0) {                // o fork retorna o pid do filho
	            if(sprintf(str_outputs, "par-shell-out-%d.txt", getpid()) < 0) { // cria nome do ficheiro do processo com o respetivo PID
	            	perror("Falhou a escrita na string");
	            }
	            if((fich_out = open(str_outputs, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR)) == -1) { // cria ficheiro com o nome do processo em questao. Os ultimos 2 args estavam na FAQ de SO, os profs deram
	                perror("Falhou a abertura do ficheiro");
	            }
	            if(close(1) == -1) {   // fecha o stdout para depois fazer dup
	            	perror("Falhou o fecho do stdout");
	            }
	            if(dup(fich_out) == -1) {   // make stdout go to file - agora esta vazio, pomos la o fich_out
	            	perror("Falhou o redireccionamento");
	            }
	            if(close(2) == -1) {   // fecha o stderr
	            	perror("Falhou o fecho do stderr");
	            }
	            if(dup(fich_out) == -1) {   // make stderr go to file
	            	perror("Falhou o redireccionamento");
	            }

	            if ((execv(cmd[0], cmd)) == -1) {     // O processo corre execv e caso dê errado devolve um erro	               
	                remove(str_outputs);
	                perror("INVALID PATHNAME");
	                exit(EXIT_FAILURE);
	            }
            }

            else {          // caso pid > 0, aumenta o contador de processos
                pthread_mutex_lock(&trinco_list);
                insert_new_process(list, pid, 0, time(NULL));
                pthread_mutex_unlock(&trinco_list);
                
                pthread_mutex_lock(&trinco_num_child_proc);
                num_child_proc++;
                conta++;
                pthread_cond_signal(&cheio);

                pthread_mutex_unlock(&trinco_num_child_proc);
            }
        }
    }
    return 0;
}

