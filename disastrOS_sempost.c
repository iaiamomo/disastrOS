#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"

void internal_semPost(){
  // do stuff :)
    //salvo l'argomento della funzione
    int fd = running->syscall_args[0];
    
    //controllo che il descrittore del semaforo sia nel processo running
    SemDescriptor* sem_des = SemDescriptorList_byFd(&running->sem_descriptors, fd);
    if(!sem_des){
        //se non trovo il descrittore nel processo, setto il valore di ritorno della semPost
        //come DSOS_ERESOURCEOPEN e termino funzione
        running->syscall_retvalue = DSOS_ERESOURCEOPEN;
        return;
    }
    
    //salvo il semaforo e incremento il contatore
    Semaphore* sem = sem_des->semaphore;
    sem->count++;
    
    //controllo se il valore del contatore è <=0
    if(sem->count <= 0){
        //se lo rimuovo il primo SemDescriptorPtr che si trova in waiting_descriptors del semaforo
        SemDescriptorPtr* sem_des_ptr = (SemDescriptorPtr*)List_detach(&sem->waiting_descriptors,
                                                                       sem->waiting_descriptors.first);
        if(!sem_des_ptr){
            //se in waiting_descriptors non c'è nessuno, setto il valore di ritorno della semPost
            //e termino funzione
            running-syscall_retvalue = 0;
            return;
        }
        
        //salvo il semaforo per trovare il processo da mettere in ready
        SemDescriptor* sem_des = sem_des_ptr->descriptor;
        PCB* ready_process = sem_des->pcb;
        
        //rimuovo dalla waiting_list del sistema il processo da mettere in ready
        PCB* ret = List_detach(&waiting_list, (ListItem*)ready_process)
        if(!ret){
            //se non trovo il processo da mettere in ready nella waiting_list del sistema
            //ho un grave errore
            running->syscall_retvalue = DSOS_ERESOURCEOPEN;
            return;
        }
        
        //modifico lo stato del processo
        ready_process->status = Ready;
        
        //inserisco il processo nella ready_list del sistema
        ret = List_insert(&ready_list, ready_list.last, (ListItem*)ready_process);
        if(!ret){
            //ho avuto problemi nell'inserire il processo nella ready_process del sistema
            //ciò può essere dovuto al fatto che si trova in un altra coda
            running->syscall_retvalue = DSOS_ERESOURCEINUSE;
            return;
        }
    }
    
    //imposto il valore di ritorno della semPost
    running->syscall_retvalue = 0;
}
