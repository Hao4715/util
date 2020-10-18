#ifndef _POOL_H_
#define _POOL_H_

//数据池中用来将数据连起来
struct List
{
	struct List *next;	//下一个节点
};


/*
 *由于数据池中存放的是回收的数据，因此数据已经没有实际的意义，我们可以将它强转成list结构，来构成链表
 */

//数据池
struct Pool
{
	struct List *free_list;	//空闲数据链表
	char name[32];	//池名称
	int used;	//使用了多少数据单元
	int left;	//可用的数据单元
	unsigned int size;	//每个数据单元的大小
};

void *pool_alloc(struct Pool *pool);

int pool_free(struct Pool *pool, void *element);

int init_pools();

void destroy_pools();

#endif
