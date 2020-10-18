/**************************************************
  >File Name: test.c
  >Author:silence
  >Create Time: 2018年02月06日 星期二 20时23分34秒
**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <proxyd/fixed_pool.h>
//##include "fixed_pool.h"

struct Buffer {
    char buf[1024];
};

static struct Pool *buffer_pool = NULL;


static void *pthread_func()
{
	// 申请内存
    struct Buffer *buffer = (struct Buffer *)pool_alloc(buffer_pool);
    if (buffer == NULL) {
        return NULL;
    }
	check_pool(buffer_pool, buffer);

    strcpy(buffer->buf, "qwertyuiop[]");

    printf("pthread buffer: %s\n", buffer->buf);

	// 释放内存
	pool_free(buffer_pool, buffer);
    
    return NULL;
}



int main()
{
    int i = 0;
	int err;
    pthread_t pid[10];

	// 初始化
    buffer_pool = create_pool_debug("buffer", sizeof(struct Buffer)); 
    if (buffer_pool == NULL) {
        return -1;
    } 

    for (i = 0; i < 32; i++) {
        err = pthread_create(&pid[i], 
				NULL, 
				(void *)&pthread_func, 
				NULL);
		if (err) {
			printf("pthread create error\n");
		} else {
			printf("pthread_func create sucess!\n");
		}
        usleep(100);
    }

	// 申请内存
    struct Buffer *buffer = (struct Buffer *)pool_alloc(buffer_pool);
    if (buffer == NULL) {
        return -1;
    }

	check_pool(buffer_pool, buffer);

    strcpy(buffer->buf, "qwertyuiop[]");

    printf("buffer: %s\n", buffer->buf);

	// 释放内存
	pool_free(buffer_pool, buffer);

	// 销毁内存池
	pool_destroy(buffer_pool);	

    return 0;
}
