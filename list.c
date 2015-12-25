/*
 * list.c - implementation of the integer list functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "list.h"


list_t* lst_new()
{
    list_t *list;
    list = (list_t*) malloc(sizeof(list_t));
    list->first = NULL;
    return list;
}

list_p* lst_new_p()
{
    list_p *list;
    list = (list_p*) malloc(sizeof(list_p));
    list->first = NULL;
    return list;
}


void lst_destroy(list_t *list)
{
    struct lst_iitem *item, *nextitem;
    item = list->first;
    while (item != NULL) {
        nextitem = item->next;
        free(item);
        item = nextitem;
    }
    free(list);
}


void lst_destroypid(list_p *list)
{
    struct lst_pid_item *item, *nextitem;
    item = list->first;
    while (item != NULL) {
        nextitem = item->next;
        free(item);
        item = nextitem;
    }
    free(list);
}

void insert_new_process(list_t *list, int pid, int status, time_t starttime)
{
    lst_iitem_t *item;

    item = (lst_iitem_t *) malloc (sizeof(lst_iitem_t));
    item->pid = pid;
    item->status = status;

    // tempos processos
    item->starttime = starttime;
    item->endtime = 0;

    item->next = list->first;
    list->first = item;
}

void insert_new_pid(list_p *list, int pid)
{
    lst_item_p *item;

    item = (lst_item_p *) malloc (sizeof(lst_item_p));
    if(list->first == NULL){
        list->last = item;
    }

    item->pid = pid;

    item->next = list->first;
    list->first = item;
}

void update_terminated_process(list_t *list, int pid, int status, time_t endtime)
{
    lst_iitem_t *item;
    item = list->first;

    while(item != NULL) {
        if(pid == item->pid) {
            item->endtime = endtime;
            item->status = status;
            break;
        }
        item = item->next;
    }
}

void lst_print(list_t *list)
{
    lst_iitem_t *item;
    item = list->first;

    while (item != NULL) {
        long diff = (item->endtime) - (item->starttime);
        printf("O processo com o pid %d foi terminado com status %d, apos %ld segundos.\n", item->pid, item->status, diff);

        item = item->next;
    }
    printf("-- end of list.\n");

  }  

void lst_matapid(list_p *list)
{
    lst_item_p *item;
    item = list->first;
    
    while (item != NULL) {
        
        kill(item->pid, SIGKILL);
        item = item->next;
    }
    printf("Nao ha terminais activos.\n");

}

int lst_removepid(list_p *list, int pid)
{
    struct lst_pid_item *item, *previtem, *nextitem;

    /* Procura na lista o elemento com o pid */
    for (item = list->first, previtem = NULL; item != NULL && item->pid != pid; previtem = item, item = item->next);
    
    /* Se a lista nao tiver nenhum elemento */
    if (item == NULL)
    {
        return -1;
        printf("Nao ha terminais activos\n");
    }

    /* Caso o elemento que procuramos seja o primeiro da lista */
    if(list->first == item){
        list->first = item->next;   // O primeiro elemento da lista passa a ser o seguinte
        free(item);
    }

    else if(item->next == NULL){ // queremos eliminar o ultimo da lista
        previtem->next = NULL;
        list->last = previtem;
        free(item);
    }

    /* Se nao for, actualizamos os ponteiros antes de remover */
    else{
        nextitem = item->next;
        free(item);
        item = nextitem;
    }
    return 0;
}

long lst_tempodeexecucao(list_t *list, int pid){
    long diferenca;
	lst_iitem_t *item;

	//printf("Process list with start and end time:\n");
	
	item = list->first;

	    while(item != NULL) {

    		if(pid == item->pid){
    		    diferenca = (item->endtime) - (item->starttime);
    		    break;
    		  }
    		item = item->next;
    	}

	return  diferenca;
}
