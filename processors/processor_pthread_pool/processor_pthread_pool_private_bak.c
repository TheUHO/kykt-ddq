#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include	"ddq_plugin.h"
#include	"processor_pthread_pool.h"
#include 	"error.h"
#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#define MAX_LOCAL_TASKS 6

// 处理本地任务队列的函数
static inline int process_local_tasks(struct processor_pthread_pool_t** local_tasks, int* local_task_count) {
    int res = 0;
    for (int i = 0; i < *local_task_count; ) {
        struct processor_pthread_pool_t* p = local_tasks[i];
        p->ret = ((task_f*)p->head.p_f)(p->head.p_inputs, p->head.p_outputs, p->head.p_attributes);

        if (p->ret == task_ret_again) {
            // 如果任务仍需再次执行，保留在本地队列
            i++;
        } else {
            // 任务完成，从本地队列中移除
            p->is_done = 1;
            local_tasks[i] = local_tasks[--(*local_task_count)];
            res = 1; // 标记有任务完成
        }
    }
    return res;
}

// void* thread_run(void* arg) {
//     ThreadPool* pool = (ThreadPool*)arg;

//     // 线程绑核
//     int thread_id = pthread_self() % pool->thread_count;
//     cpu_set_t cpuset;
//     CPU_ZERO(&cpuset);
//     CPU_SET(thread_id, &cpuset);
//     pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

//     // 本地任务队列
//     struct processor_pthread_pool_t* local_tasks[MAX_LOCAL_TASKS];
//     int local_task_count = 0;
//     // int local_task_signal = 0;
//     struct processor_pthread_pool_t* p = NULL;

//     while (1) {
//         // 遍历本地任务队列，重新执行任务
//         if (local_task_count > 0) {
//             //如果有任务可以执行，则优先执行本地队列任务
//             if(process_local_tasks(local_tasks, &local_task_count)){
//                 continue; // 跳过全局任务队列的处理逻辑
//             }
//         }
        
//         if (local_task_count < MAX_LOCAL_TASKS){
//             pthread_mutex_lock(&pool->lock);

//             // 如果全局任务队列为空且本地任务队列为空，才进入等待
//             while (pool->task_count == 0 && local_task_count == 0 && !pool->stop) {
//                 pthread_cond_wait(&pool->cond, &pool->lock);
//             }

//             if(pool->task_count == 0 && local_task_count > 0){
//                 pthread_mutex_unlock(&pool->lock);
//                 continue; // 继续执行本地任务
//             }

//             if (pool->stop) {
//                 pthread_mutex_unlock(&pool->lock);
//                 break;
//             }

//             if (pool->task_count > 0) {
//                 p = pool->tasks[--pool->task_count];
//             }
//             pthread_mutex_unlock(&pool->lock);
//             // 执行任务
//             if (p) {
//                 p->ret = ((task_f*)p->head.p_f)(p->head.p_inputs, p->head.p_outputs, p->head.p_attributes);
//                 if (p->ret == task_ret_again) {
//                     // 如果任务需要再次执行，将其保存到本地队列
//                     local_tasks[local_task_count++] = p;
//                 } else {
//                     // 任务执行完成
//                     p->is_done = 1;
//                 }
//             }
//         }
//     }

//     return NULL;
// }

void* thread_run(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;

    // 线程绑核
    int thread_id = pthread_self() % pool->thread_count;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (1) {
        pthread_mutex_lock(&pool->lock);

        while (pool->task_count == 0 && !pool->stop) {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }

        if (pool->stop) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        struct processor_pthread_pool_t* p = pool->tasks[--pool->task_count];
        pthread_mutex_unlock(&pool->lock);

		// 执行任务
        p->ret = ((task_f*)p->head.p_f) (p->head.p_inputs, p->head.p_outputs, p->head.p_attributes);
		p->is_done = 1;
    }

    return NULL;
}


static int get_physical_core_count() {
    return sysconf(_SC_NPROCESSORS_ONLN) - 1; // 获取物理核心数
}

ThreadPool* thread_pool_init() {
    ThreadPool* pool = malloc(sizeof(ThreadPool));
	pool->task_count = 0;
    pool->stop = 0;
	pool->n_ref = 0;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);

    // 动态获取物理核心数
    pool->thread_count = get_physical_core_count();
    pool->threads = malloc(pool->thread_count * sizeof(pthread_t));

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_create(&pool->threads[i], NULL, thread_run, pool);
    }
	return pool;
}

int thread_pool_submit(ThreadPool* pool,  struct processor_pthread_pool_t* p) {
	int ret = 0;

    pthread_mutex_lock(&pool->lock);

    // 检查任务队列是否已满
    if (pool->task_count >= MAX_TASKS) {
        ret = -1; // 任务队列已满，返回 -1
    } else {
        // 将任务添加到任务队列
        pool->tasks[pool->task_count++] = p;
        pthread_cond_signal(&pool->cond); // 唤醒等待任务的线程
    }

    pthread_mutex_unlock(&pool->lock);

    return ret; // 返回任务提交结果
}

void thread_pool_destroy(ThreadPool* pool) {
    pthread_mutex_lock(&pool->lock);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads); // 释放线程数组
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
}