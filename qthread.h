/*
 * file:        qthread.h
 * description: assignment - simple emulation of POSIX threads
 * class:       CS 5600, Fall 2019
 */
#ifndef __QTHREAD_H__
#define __QTHREAD_H__

/* Note that for each structure we need a forward reference, but you'll
 * define the actual contents of the structure in qthread.c 
 * Also qthread_t is a pointer, while qthread_mutex_t (and qthread_cond_t)
 * isn't a pointer, but an alias for the structure itself.
 */
struct qthread {
    /* your code here */;
    int id;
    int is_active;
    struct qthread *next;
    struct qthread *join;
    void *stack_sp;
    unsigned long tt_wake_up;
    void *return_value;
    int is_exited;
};     

typedef struct qthread *qthread_t;

struct threadq {
    /* your code here */
    qthread_t head;
    qthread_t tail;
};

typedef struct threadq *queue_list;

/* These are global variables that are used in the qthread.c and test.c file.
*/
qthread_t current;  // current  is  current thread that is in execution.
int thread_id;
queue_list active_list;  // active_list is queue of threads active that are waiting to be run.
queue_list sleeping_list;  // sleeping_list is queue of inactive threads.

struct qthread_mutex {
    int available;
    queue_list wait_queue_list;
};

typedef struct qthread_mutex qthread_mutex_t;

struct qthread_cond {
    queue_list queue;
};

typedef struct qthread_cond qthread_cond_t;

typedef void (*f_2arg_t)(void*, void*);
typedef void * (*f_1arg_t)(void*);

/* prototypes - see qthread.c for function descriptions
 */

void qthread_init(void);
qthread_t qthread_create(f_1arg_t f, void *arg1);
void qthread_yield(void);
void qthread_exit(void *val);
void *qthread_join(qthread_t thread);
qthread_mutex_t *qthread_mutex_create(void);
void qthread_mutex_lock(qthread_mutex_t *mutex);
void qthread_mutex_unlock(qthread_mutex_t *mutex);
void qthread_mutex_destroy(qthread_mutex_t *mutex);
qthread_cond_t *qthread_cond_create(void);
void qthread_cond_wait(qthread_cond_t *cond, qthread_mutex_t *mutex);
void qthread_cond_signal(qthread_cond_t *cond);
void qthread_cond_broadcast(qthread_cond_t *cond);
void qthread_cond_destroy(qthread_cond_t *cond);
void qthread_usleep(long int usecs);
qthread_t pop_from_head(queue_list *queue);
qthread_t create_new_thread();
void add_to_tail(queue_list *queue,  qthread_t thread);
int is_empty(queue_list *queue);
static unsigned long get_usecs(void);
int ctx_switch();

#endif
