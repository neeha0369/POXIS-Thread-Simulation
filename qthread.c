/*
 * file:        qthread.c
 * description: assignment - simple emulation of POSIX threads
 * class:       CS 5600, Fall 2019
 */

/* a bunch of includes which will be useful */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <errno.h>
#include "qthread.h"

#define STACK_SIZE 8*1024

/* prototypes for stack.c and switch.s
 * see source files for additional details
 */
extern void switch_to(void **location_for_old_sp, void *new_value);
extern void *setup_stack(int *stack, void *func, void *arg1, void *arg2);

int thread_id = 0;
queue_list active_list = NULL;
queue_list sleeping_list = NULL;
qthread_t current = NULL;

// checks if queue that holds the threads are empty or not.
int is_empty(queue_list *queue) {
    if (queue && *queue == NULL) 
        return 1;
    
    return 0;
}

// adds thread to the end queue that holds the threads.
void add_to_tail(queue_list *queue,  qthread_t thread) {
    if (thread != NULL) {
        if (is_empty(queue)) {
            // initialize the queue
            *queue = (queue_list) calloc(1, sizeof(struct threadq));
            (*queue)->head = thread;
            (*queue)->tail = thread;
        }
        else {
            // append to the tail of queue
            qthread_t tail_node = (*queue)->tail;
            tail_node->next = thread;
            (*queue)->tail = thread;
        }
    }
}

// pops the thread that is at the first position in the queue of the threads.
qthread_t pop_from_head(queue_list *queue) {
    qthread_t poped_element = NULL;
    if (!is_empty(queue)) {
        if ((*queue)->head == (*queue)->tail) {
            // only one element in the queue
            poped_element = (*queue)->head;
            free(*queue);
            *queue = NULL;
        }
        else {
            poped_element = (*queue)->head;
            (*queue)->head = poped_element->next;
        }
        poped_element->next = NULL;
    }

    return poped_element;
}

qthread_t create_new_thread() {
    qthread_t thread = (qthread_t) calloc(1, sizeof(struct qthread));
    thread->id = ++thread_id;
    thread->next = NULL;
    thread->join = NULL;
    thread->stack_sp = NULL;
    thread->tt_wake_up = 0;
    thread->is_active = 1;
    thread->is_exited = 0;
    thread->return_value = (void *) 0;
    return thread;
}

void *create_stack(f_2arg_t wrap, void *arg1, void *arg2) {
    void *stack = (void *) calloc(1, STACK_SIZE);
    int *stack_end = stack + STACK_SIZE;
    void *arg_1 = arg1? arg1 : NULL;
    void *arg_2 = arg2? arg2 : NULL;
    void *adjusted_stack = setup_stack(stack_end, wrap, arg_1, arg_2);
    return adjusted_stack;
}

void wrapper(void *arg1, void *arg2) {
    f_1arg_t f = arg1;
    void *tmp = f(arg2);
    qthread_exit(tmp);
}

void add_all_to_active_queue(queue_list *queue) {
    qthread_t thread = NULL;
    while (!is_empty(queue)) {
        thread = pop_from_head(queue);
        thread->is_active = 1;
        add_to_tail(&active_list, thread);
    }
}

qthread_t create_2arg_thread(f_2arg_t wrap, void *arg1, void *arg2) {
  qthread_t thread = create_new_thread();
  thread->stack_sp = create_stack(wrap, arg1, arg2); 
  return thread; 
}

/* qthread_create - see hints @for how to implement it, especially the
 * reference to a "wrapper" function
 */
qthread_t qthread_create(f_1arg_t f, void *arg1) {
    qthread_t thread = create_2arg_thread(wrapper, f, arg1);
    add_to_tail(&active_list, thread);
    return thread;
}

/* Helper function for the POSIX replacement API - you'll need to tell
 * time in order to implement qthread_usleep. WARNING - store the
 * return value in 'unsigned long' (64 bits), not 'unsigned' (32 bits)
 */
static unsigned long get_usecs(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000000 + tv.tv_usec;
}

/**
 * Filter woken threads from sleeping list 
 * and add them to active list
 * */
void add_woken_threads_to_active() {
    if (!is_empty(&sleeping_list)) {
        qthread_t thread = sleeping_list->head;
        qthread_t prev = NULL;
        qthread_t next = NULL;
        while(thread != NULL) {
            next = thread->next;
            if (thread->tt_wake_up < get_usecs()) {
                thread->is_active = 1;
                thread->tt_wake_up = 0;
                add_to_tail(&active_list, thread);

                // remove thread from queue
                thread->next = NULL;
                if (prev == NULL)
                    sleeping_list->head = next;
                else
                    prev->next = next; 
            }
            else {
              prev = thread;
            }
            thread = next;
        }
    }
}

/**
 * If there are no runnable threads, scheduler needs to block
 * waiting for a thread blocked in 'qthread_usleep' to wake up
 * */
int ctx_switch() {
    qthread_t next_thread = NULL;
    qthread_t prev_current = NULL;
    while (!is_empty(&active_list) || !is_empty(&sleeping_list)) {
        // check if any threads woke up
        if (is_empty(&active_list))
            add_woken_threads_to_active();

        // schedule
        next_thread = pop_from_head(&active_list);
        if (next_thread != NULL && 
            next_thread->is_active == 1 && 
            next_thread->is_exited == 0) {
                if (current != next_thread) {
                    prev_current = current;
                    current = next_thread;
                    if (prev_current == NULL || prev_current->is_exited){
                        switch_to(NULL, next_thread->stack_sp);}
                    else
                        switch_to(&prev_current->stack_sp, next_thread->stack_sp);
                    break;
                }
                else {
                    if (!is_empty(&active_list) || !is_empty(&sleeping_list)) {
                        add_to_tail(&active_list, next_thread);
                    }
                }
        }
    }

    return 0;
}

/* I suggest factoring your code so that you have a 'schedule'
 * function which selects the next thread to run and @switches to it,
 * or goes to sleep if there aren't any threads left to run.
 *
 * NOTE - if you end up switching back to the same thread, do *NOT*
 * use do_switch - check for this case and return from schedule(), 
 * or else @you'll crash.
 */
void schedule(void *save_location) {
    ctx_switch();
}

/* qthread_init - set up a thread structure for the main (OS-provided) thread
 */
void qthread_init(void) {
    current = NULL;
    current = create_new_thread();
}

/* qthread_yield - yield to the next @runnable thread.
 */
void qthread_yield(void) {
    add_to_tail(&active_list, current);
    ctx_switch();
}

/* qthread_exit, qthread_join - exit argument is returned by
 * qthread_join. Note that join blocks if the thread hasn't exited
 * yet, and is allowed to crash @if the thread doesn't exist.
 */
void qthread_exit(void *val){
    current->is_exited = 1;
    current->return_value = val;
    
    // wait until qthread_join to free the thread structure
    while (current->join == NULL) {
        qthread_yield();
    }
    
    add_to_tail(&active_list, current->join);
    ctx_switch();
}

void *qthread_join(qthread_t thread) {
    if (thread != NULL && thread->join == NULL) {
        thread->join = current;
        ctx_switch();

        return thread->return_value;
    }
    else {
        return NULL;
    }
}

/* Mutex functions
 */
qthread_mutex_t *qthread_mutex_create(void) {
    qthread_mutex_t *mutex = (qthread_mutex_t*) calloc(1, sizeof(qthread_mutex_t));
    mutex->available = 1;
    mutex->wait_queue_list = NULL;
    return mutex;
}

void qthread_mutex_destroy(qthread_mutex_t *mutex) {
    if (!is_empty(&mutex->wait_queue_list)) {
        add_all_to_active_queue(&mutex->wait_queue_list);
        free(mutex->wait_queue_list);
        free(mutex);
    }
}

void qthread_mutex_lock(qthread_mutex_t *mutex) {
    while(!(mutex->available)) {
        if (current->is_active == 1) {
            current->is_active = 0;
            add_to_tail(&mutex->wait_queue_list, current);
            ctx_switch();
        }
        else {
            return;   
        }
    }   
    mutex->available = 0;
}

void qthread_mutex_unlock(qthread_mutex_t *mutex) {
        mutex->available = 1;
        add_all_to_active_queue(&mutex->wait_queue_list);
}

/* Condition variable functions
 */
qthread_cond_t *qthread_cond_create(void) {
    qthread_cond_t *cond = (qthread_cond_t *) calloc(1, sizeof(struct qthread_cond));
    cond->queue = NULL;
    return cond;
}

void qthread_cond_destroy(qthread_cond_t *cond) {
    free(cond->queue);
}

void qthread_cond_wait(qthread_cond_t *cond, qthread_mutex_t *mutex) {
    qthread_mutex_unlock(mutex);
    current->is_active = 0;
    add_to_tail(&cond->queue, current);
    ctx_switch();
}

void qthread_cond_signal(qthread_cond_t *cond) {
    qthread_t thread = pop_from_head(&cond->queue);
    if (thread != NULL) {
        thread->is_active = 1;
        add_to_tail(&active_list, thread);
    }
}

void qthread_cond_broadcast(qthread_cond_t *cond) {
    add_all_to_active_queue(&cond->queue);
}

/* POSIX replacement API. This semester we're only implementing 'usleep'
 *
 * If there are no runnable threads, your scheduler needs to block
 * waiting for a thread blocked in 'qthread_usleep' to wake up. 
 */

/* qthread_usleep - yield to next runnable thread, making arrangements
 * to be put back on the active list after 'usecs' timeout. 
 */
void qthread_usleep(long int usecs) {
    if (current->is_exited == 0) {
        current->tt_wake_up = get_usecs() + usecs;
        add_to_tail(&sleeping_list, current);
        current->is_active = 0;
        ctx_switch();
    }
}
