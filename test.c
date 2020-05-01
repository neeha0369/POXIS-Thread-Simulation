#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "qthread.h"

START_TEST(thread_init) {
    qthread_init();
    ck_assert(current != NULL);
    ck_assert(current->tt_wake_up == 0.0);
}
END_TEST

START_TEST(empty_queue_pop) {
    queue_list queue = NULL;
    ck_assert(queue == NULL);
    qthread_t thread = pop_from_head(&queue);
    ck_assert(thread == NULL);
}
END_TEST

START_TEST(add_to_queue) {
    queue_list queue = NULL;
    qthread_t thread = pop_from_head(&queue);
    ck_assert(thread == NULL);
    
    // insert elements
    add_to_tail(&queue, create_new_thread());
    ck_assert(queue != NULL);
    add_to_tail(&queue, create_new_thread());
    add_to_tail(&queue, create_new_thread());

    // pop elements
    thread = pop_from_head(&queue);
    ck_assert(thread != NULL);
    ck_assert(thread->id == 2);
    thread = pop_from_head(&queue);
    ck_assert(thread != NULL);
    ck_assert(thread->id == 3);
    thread = pop_from_head(&queue);
    ck_assert(thread != NULL);
    ck_assert(thread->id == 4);
    ck_assert(queue == NULL);
    thread = pop_from_head(&queue);
    ck_assert(thread == NULL);
    ck_assert(queue == NULL);
}
END_TEST
   
int create_yield_flag = 0;

void* create_yield_th1(void *arg) {
    create_yield_flag = 1;
    return NULL;
}

START_TEST(create_yield) {
    qthread_t thread2 = qthread_create(create_yield_th1, NULL);
    ck_assert(create_yield_flag == 0);
    
    qthread_yield();

    ck_assert(create_yield_flag == 1);
}
END_TEST

int mutex_lock_unlock_flag = 0;
qthread_mutex_t *lock_unlock_mutex_var;

void *mutex_lock_unlock_th1(void *arg) { 
    qthread_mutex_lock(lock_unlock_mutex_var);
    mutex_lock_unlock_flag = 1;
    qthread_yield();
    qthread_yield();
    qthread_yield();
    ck_assert(mutex_lock_unlock_flag == 1);
    qthread_mutex_unlock(lock_unlock_mutex_var);
}

void *mutex_lock_unlock_th2(void *arg) { 
    qthread_mutex_lock(lock_unlock_mutex_var);
    mutex_lock_unlock_flag = 2;
    qthread_yield();
    mutex_lock_unlock_flag = 2;
    qthread_yield();
    mutex_lock_unlock_flag = 2;
    qthread_mutex_unlock(lock_unlock_mutex_var);
}

START_TEST(mutex_lock_unlock) {
    lock_unlock_mutex_var = qthread_mutex_create();
    ck_assert(lock_unlock_mutex_var != NULL);
    ck_assert(lock_unlock_mutex_var->available == 1);

    qthread_t t1 = qthread_create(mutex_lock_unlock_th1, NULL);
    qthread_t t2 = qthread_create(mutex_lock_unlock_th2, NULL);

    qthread_join(t1);
    qthread_join(t2);

    ck_assert(mutex_lock_unlock_flag == 2);
}
END_TEST

int thread_usleep_flag = 0;

void *thread_usleep_th1(void *arg) { 
    thread_usleep_flag = 1;
    qthread_usleep(10);
    ck_assert(thread_usleep_flag == 2);
    thread_usleep_flag = 3;
}

void *thread_usleep_th2(void *arg) { 
    ck_assert(thread_usleep_flag == 1);
    thread_usleep_flag = 2;
    qthread_usleep(10);
    ck_assert(thread_usleep_flag == 3);
    thread_usleep_flag = 4;
}

START_TEST(thread_usleep) {
    qthread_t t1 = qthread_create(thread_usleep_th1, NULL);
    qthread_t t2 = qthread_create(thread_usleep_th2, NULL);
    
    qthread_join(t1);
    qthread_join(t2);
    
    ck_assert(thread_usleep_flag == 4);
}
END_TEST

int cond_wait_flag = 0;
qthread_mutex_t *cond_wait_mutex_var;
qthread_cond_t *cond_wait_cond_var;

void *cond_wait_th1(void *arg) { 
    cond_wait_flag = 1;
    qthread_cond_wait(cond_wait_cond_var, cond_wait_mutex_var);
    cond_wait_flag = 3;
}

void *cond_wait_th2(void *arg) { 
    ck_assert(cond_wait_flag == 1);
    ck_assert(is_empty(&active_list) == 1);
    cond_wait_flag = 2;
    qthread_cond_signal(cond_wait_cond_var);
    ck_assert(cond_wait_flag == 2);
}

START_TEST(cond_wait) {
    qthread_t t1 = qthread_create(cond_wait_th1, NULL);
    qthread_t t2 = qthread_create(cond_wait_th2, NULL);
    
    cond_wait_cond_var = qthread_cond_create();
    cond_wait_mutex_var = qthread_mutex_create();
    
    qthread_join(t1);
    ck_assert(cond_wait_flag == 3);
}
END_TEST


void *thread_join_return_value_th1(void *arg) {
    qthread_yield();
    return arg;
}

void *thread_join_return_value_th2(void *arg) { 
    return arg; 
}

START_TEST(thread_join_return_value) {
    qthread_t t1 = qthread_create(thread_join_return_value_th1, "THREAD_1");
    qthread_t t2 = qthread_create(thread_join_return_value_th2, "THREAD_2");

    char *r_value1 = (char *) qthread_join(t1);
    char *r_value2 = (char *) qthread_join(t2);
    ck_assert(r_value1 == "THREAD_1");
    ck_assert(r_value2 == "THREAD_2");
}
END_TEST

START_TEST(empty_ctx_switch) {
    int op = 1;
    op = ctx_switch();
    ck_assert(op == 0);
}
END_TEST

Suite *create_suite(void) {
    Suite *s;
	TCase *tc;

	s = suite_create("Homework 2");
	tc = tcase_create("Queue");

    // Queue
	tcase_add_test(tc, empty_queue_pop);
    tcase_add_test(tc, add_to_queue);

    // Thread
    tcase_add_test(tc, thread_init);
    tcase_add_test(tc, create_yield);
    tcase_add_test(tc, thread_usleep);
    tcase_add_test(tc, thread_join_return_value);

    // Mutex
    tcase_add_test(tc, mutex_lock_unlock);

    // Cond
    tcase_add_test(tc, cond_wait);

    // ctx switch
    tcase_add_test(tc, empty_ctx_switch);

	suite_add_tcase(s, tc);

	return s;
}

void main() {
    int n_failed;
	Suite *s;
	SRunner *sr;
	s = create_suite();
	sr = srunner_create(s);

    qthread_init();

	srunner_run_all(sr, CK_VERBOSE);
	n_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	printf("%d tests failed\n", n_failed);
}
