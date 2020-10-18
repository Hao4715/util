#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>

#include "Common.h"
#include "Pool.h"
#include "Connection.h"
#include "Buffer.h"
#include "Session.h"
#include "Inspect.h"
#include "Log.h"
#include "ip_tree.h"
#include "certs_tree.h"
#include "Global.h"

pthread_mutex_t buffers_pool_mutex;

/*
** 创建一个数据池,为了防止数据碎片，数据单元大小是16的倍数
*/
struct Pool *create_pool(char *name, unsigned int size)
{
	struct Pool* pool = (struct Pool*)malloc(sizeof(struct Pool));
	if (pool == NULL)
	{
		print_log("[%s %d %u] create pool: malloc pool error(%s)", __FILE__, __LINE__, self_pid, strerror(errno));	
		return NULL;
	}

	strncpy(pool->name, name, sizeof(pool->name)-1);
	pool->free_list = NULL;
	pool->used = pool->left = 0;

	pool->size = size;
	//syslog(LOG_INFO, "create pool: name = %s, size = %d, realsize = %d", name, size, pool->size);
	return pool;
}

/*
** 从数据池pool中获得一个数据单元
*/
void *pool_alloc(struct Pool *pool)
{	
	void *__p;
	
	if(pool == buffers_pool)
	{
		pthread_mutex_lock(&buffers_pool_mutex);
	}

	if ((__p = pool->free_list) == NULL)//数据池已空,从系统分配
	{
		__p = malloc(pool->size);
		if (__p != NULL)
			pool->used++;
		//syslog(LOG_INFO, "pool_alloc: name = %s, size = %d, used = %d, left = %d(malloc from system)", pool->name, pool->size, pool->used, pool->left);
	}
	else	//数据池中有空闲的数据单元
	{
		__p = pool->free_list;
		pool->free_list = pool->free_list->next;
		pool->left--;
		pool->used++;
		//syslog(LOG_INFO, "pool_alloc: name = %s, size = %d, used = %d, left = %d(malloc from pool)", pool->name, pool->size, pool->used, pool->left);
	}

	if(pool == buffers_pool)
	{
		pthread_mutex_unlock(&buffers_pool_mutex);
	}

	return __p;
}

/*
** 释放一个数据结构，并没有真正释放，而是将其放入pool中
*/
int pool_free(struct Pool *pool, void *element)
{
	struct List *list = (struct List*)element;

	if(pool == buffers_pool)
	{
		pthread_mutex_lock(&buffers_pool_mutex);
	}

	list->next = pool->free_list;
	pool->free_list = list;
	pool->left++;
	pool->used--;

	if(pool == buffers_pool)
	{
		pthread_mutex_unlock(&buffers_pool_mutex);
	}
	
	//syslog(LOG_INFO, "pool_free: name = %s, size = %d, used = %d, left = %d(mem put in pool)", pool->name, pool->size, pool->used, pool->left);

	return OK;
}

/*
** 释放数据池中的空闲数据单元和数据池本身，这里是真正的free
*/
void pool_destroy(struct Pool *pool)
{
	struct List *list = pool->free_list;
	while (list)
	{
		struct List *tmp = list;
		list = list->next;
		free(tmp);
	}

	free(pool);
}

/*
** 初始化系统中的数据池
*/
int init_pools()
{
	connections_pool = create_pool("connection", sizeof(struct Connection));
	if (connections_pool == NULL)
	{
		syslog(LOG_ERR, "init pools: connections_pool error");
		return ERROR;
	}

	sessions_pool = create_pool("session", sizeof(struct Session));
	if (sessions_pool == NULL)
	{
		syslog(LOG_ERR, "init pools: sessions_pool error");
		return ERROR;
	}
	
	buffers_pool = create_pool("buffer", sizeof(struct Buffer));
	pthread_mutex_init(&buffers_pool_mutex, NULL);
	if (buffers_pool == NULL)
	{
		syslog(LOG_ERR, "init pools: buffers_pool error");
		return ERROR;
	}
		
	i_pool = create_pool("ip_node", sizeof(struct Ip_node));
	if (i_pool == NULL)
	{
		syslog(LOG_ERR, "init pools: ip_node_pool error");
		return ERROR;
	}
	
#ifdef FILTER
	tcp_stream_pool = create_pool("tcp_stream", sizeof(struct tcp_stream));
	if (tcp_stream_pool == NULL)
	{
		syslog(LOG_ERR, "init pools: tcp_stream_pool error");
		return ERROR;
	}
	
	lurker_node_pool = create_pool("lurker", sizeof(struct lurker_node));
	if (lurker_node_pool == NULL)
	{
		syslog(LOG_ERR, "init pools: lurker_node_pool error");
		return ERROR;
	}
#endif
	return OK;
}

/*
** 销毁系统中的数据池
*/
void destroy_pools()
{
	pool_destroy(connections_pool);
	pool_destroy(sessions_pool);
	pool_destroy(buffers_pool);
	pool_destroy(i_pool);
	pool_destroy(c_pool);
#ifdef FILTER
	pool_destroy(tcp_stream_pool);
	pool_destroy(lurker_node_pool);	
#endif
}
