/*
 * list.h - definitions and declarations of the integer list   -pthread
 */

#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>


/* lst_iitem - each element of the list points to the next element */
typedef struct lst_iitem {
    int pid;
    time_t starttime;
    time_t endtime;
    int status;
    struct lst_iitem *next;
} lst_iitem_t;

/* list_t */
typedef struct {
    lst_iitem_t * first;
} list_t;


/* lst_item_pid - each element of the list points to the previous and the next element */
typedef struct lst_pid_item {
	int pid;
	struct lst_pid_item *prev, *next;
} lst_item_p;

/* list_p */
typedef struct {
	lst_item_p *first, *last;
} list_p;


/* lst_new - allocates memory for list_t and initializes it */
list_t* lst_new();

/* lst_new_pid - allocates memory for list_p and initializes it */
list_p* lst_new_p();

/* lst_destroy - free memory of list_t and all its items */
void lst_destroy(list_t *list);

/* lst_destroypid - free memory of list_t and all its items */
void lst_destroypid(list_p *list);

/* insert_new_process - insert a new item with process id and its start time in list 'list' */
void insert_new_process(list_t *list, int pid, int status, time_t starttime);

/* insert_new_process - insert a new item with process id and its start time in list 'list' */
void insert_new_pid(list_p *list, int pid);

/* lst_remove - remove first item of value 'value' from list 'list' */
void update_terminated_process(list_t *list, int pid, int status, time_t endtime);

/* lst_print - print the content of list 'list' to standard output */
void lst_print(list_t *list);

/* mata todos os par-shell-terminais */
void lst_matapid(list_p *list);

/* da o tempo de execucao de cada processo */
long lst_tempodeexecucao(list_t *list, int pid);

/* remove um dado pid da lista dos pids */
int lst_removepid(list_p *list, int pid);

#endif
