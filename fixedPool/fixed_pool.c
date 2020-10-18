/**************************************************
  >File Name: fixed_pool.c
  >Author:silence
  >Create Time: 2018年02月06日 星期二 15时12分48秒
**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

void HexdumpPool(int level, void *data, int len);

//数据池中用来将数据连起来
struct List {        
    struct List *next;  //下一个节点
};

/*   
 * 数据池
 * 由于数据池中存放的是回收的数据
 * 因此数据已经没有实际的意义
 * 我们可以将它强转成list结构，来构成链表
 * */  
struct Pool {    
    struct List 	*free_list; 		//空闲数据链表
    char 			name[32];          	//池名称
	int 			tolal;
    int 			used;               //使用了多少数据单元
    int 			left;               //可用的数据单元
    unsigned int 	size;      			//每个数据单元的大小
    pthread_mutex_t mutex;
	unsigned int 	debug:1;
};

//#define MYPOOL_DEBUG					1

#define POOL_DEBUG_HALF_SIZE			64
#define POOL_DEBUG_SIZE					(2 * POOL_DEBUG_HALF_SIZE)

char pool_front[] = {0xff, 0xff, 0x69, 0x73, 0x72, 0x63, 0xff, 0xff};	/* 0xff 0xff isrc 0xff 0xff */
char pool_rear[]  = {0xff, 0xff, 0x49, 0x53, 0x52, 0x43, 0xff, 0xff};	/* 0xff 0xff ISRC 0xff 0xff */

static int __check_pool(void *element, unsigned int size);
static int __pack_pool (void *element, unsigned int size, const char *file, int line);


/*
 * 创建一个数据池
 * */
struct Pool *create_pool(char *name, unsigned int size)
{
	// printf("[create_pool]: entry create_pool \n");
	struct Pool* pool = (struct Pool*)malloc(sizeof(struct Pool));
	if (pool == NULL) {
		printf("[create pool]: malloc pool error\n");	
		return NULL;
	}

	int ret = pthread_mutex_init(&(pool -> mutex), NULL);
    if (ret != 0) {
		printf("[create pool]: pool mutex init error\n");	
        return NULL;
    }

	strcpy(pool->name, name);
	pool -> free_list = NULL;
	pool -> used = pool->left = 0;
	pool -> size = size;
	pool -> debug = 0;
	pool -> tolal = 0;

	// printf("[create_pool]: pool of %s init sucess!\n", pool -> name);

	return pool;
}

/*
 * 创建一个数据池
 * */
struct Pool *create_pool_debug(char *name, unsigned int size)
{
	// printf("[create_pool_debug]: entry create_pool_debug\n");
	struct Pool *pool = create_pool(name, size);
	if (pool != NULL) {
		pool -> debug = 1;					/* 与create_pool的差别 */
		pool -> size += POOL_DEBUG_SIZE; 
	}

#if 1
	printf("pool -> debug = %d\n", pool -> debug);
#endif

	return pool;
}



/*
** 从数据池pool中获得一个数据单元
*/
void *__pool_alloc(struct Pool *pool, char *file, int line)
{	
#if 0
	void *__p;

	__p = malloc(pool->size);

	if (__p != NULL && pool -> debug == 1) {
		__pack_pool(__p, pool -> size, file, line);		// 填充字节
		__p = __p + POOL_DEBUG_HALF_SIZE;
	}

	return __p;
#endif

#if 1
	// printf("entry __pool_alloc  : start mutex : %s\n", pool->name);
	pthread_mutex_lock(&pool -> mutex);
	// printf("entry __pool_alloc : haha get mutex : %s\n", pool->name);

	void *__p;

	if ((__p = pool->free_list) == NULL)		// 数据池已空,从系统分配
	{
		__p = malloc(pool->size);
		if (__p != NULL) {
			pool -> used ++;
			pool -> tolal ++;
		}
	}
	else										//数据池中有空闲的数据单元
	{
		__p = pool->free_list;
		pool->free_list = pool->free_list->next;
		pool->left--;
		pool->used++;
	}


	if (__p != NULL && pool -> debug == 1) {
		__pack_pool(__p, pool -> size, file, line);		// 填充字节
		__p = __p + POOL_DEBUG_HALF_SIZE;
	}

	pthread_mutex_unlock(&pool -> mutex);
	// printf("quit __pool_alloc : free mutex: %s\n", pool -> name);

	return __p;
#endif
}

/*
 ** 释放一个数据结构，并没有真正释放，而是将其放入pool中
 */
int __pool_free(struct Pool *pool, void *element, char *file, int line)
{
#if 0
	void *ptr = element;

	if (pool -> debug == 1) {
		ptr = ptr - POOL_DEBUG_HALF_SIZE;
		if (__check_pool(ptr, pool -> size) != 0) {
			printf("pool -> name: %s\n", pool -> name);
			HexdumpPool(0, ptr, pool -> size);
			// memset(ptr, '0', pool -> size);	/* 清空内存 */
			return -1;	
		}
		// memset(ptr, '0', pool -> size);	/* 清空内存 */
	}

	free(ptr);

	return 0;
#endif
#if 1
	pthread_mutex_lock(&pool -> mutex);

	void *ptr = element;

	if (pool -> debug == 1) {
		ptr = ptr - POOL_DEBUG_HALF_SIZE;
		if (__check_pool(ptr, pool -> size) != 0) {
			printf("[%s %d] pool -> name: %s\n", file, line, pool -> name);
			HexdumpPool(0, ptr, pool -> size);
			// memset(ptr, '0', pool -> size);	/* 清空内存 */
			pthread_mutex_unlock(&pool -> mutex);
			return -1;	
		}
		// memset(ptr, '0', pool -> size);	/* 清空内存 */
	}

	struct List *list = (struct List*)ptr;

	list->next = pool->free_list;
	pool->free_list = list;
	pool->left++;
	pool->used--;

	pthread_mutex_unlock(&pool -> mutex);


	return 0;
#endif 
}

/*
 ** 释放数据池中的空闲数据单元和数据池本身，这里是真正的free
 */
void __pool_destroy(struct Pool *pool, char *file, int line)
{
	pthread_mutex_lock(&pool -> mutex);

	struct List *list = pool->free_list;
	while (list) {
		struct List *tmp = list;
		list = list->next;

		if (pool -> debug == 1) {
			free(tmp);
		} else {
			// TODO  这里该怎么处理 现在处理方式是内存有问题提示错误 内问题就正常释放
			free(tmp - POOL_DEBUG_HALF_SIZE);
		}
	}

	pthread_mutex_unlock(&pool -> mutex);

	free(pool);

	return ;
}

int tmp_check_pool(struct Pool *pool, void *element, char *file, int line)
{
	int 	ret = 0;
	void *ptr = element;

	if (pool && pool -> debug == 0)		return ret;

	// if (file)   printf("[%s %d] call check_pool: \n", file, line);

	// return (__check_pool(ptr - POOL_DEBUG_HALF_SIZE, pool -> size));
	ret = __check_pool(ptr - POOL_DEBUG_HALF_SIZE, pool -> size);
	if (ret != 0) {
		HexdumpPool(0, ptr - POOL_DEBUG_HALF_SIZE, pool -> size);
		if (file)   { 
			printf("[%s %d] call check_pool(%s): \n", file, line, pool -> name); 
		}
	}

	return ret;
}


// 填充数据 POOL_DEBUG
static int __pack_pool(void *element, unsigned int size, const char *file, int line)
{
	//memset(element, '0', size);

	void *pptr = element;
	void *ptr  = element + 8;
	//void *ptr  = element + size - POOL_DEBUG_HALF_SIZE + 8;
	void *nptr = element + size - POOL_DEBUG_HALF_SIZE;

	memcpy(pptr, pool_front, 8);
	memcpy(pptr + POOL_DEBUG_HALF_SIZE - 8, pool_rear, 8);

	// 此处有字节可写，写入数据为文件名 行号
	if ((strlen(file) + 3 + sizeof(line)) < (POOL_DEBUG_HALF_SIZE - 17)) {
		/* strlen(file) + 3 + strlen(line) < 63 */
		memcpy(ptr, file, strlen(file));
		memcpy(ptr + strlen(file), " : ", 3);
		sprintf(ptr + strlen(file) + 3, "%d", line);
		*(char *)(ptr + strlen(file) + 3 + sizeof(line) + 1) = '\0';
	} else {
		/* TODO 文件名 + 行号长度大于POOL_DEBUG_HALF_SIZE - 17 就不填充这些信息 待解决 */
		printf("[pack_pool] strlen(file) + 3 + strlen(line) > %d\n", 
				POOL_DEBUG_HALF_SIZE - 1);
	}

#if 0
	// 用于调试填充内容长度 是否 大于496
	printf("[%s %d] \"%s\", The pack len is %ld\n", 
			file, line, 
			(char *)ptr,
			strlen(file) + 3 + sizeof(line));
#endif

	memcpy(nptr, pool_front, 8);
	memcpy(nptr + POOL_DEBUG_HALF_SIZE - 8, pool_rear, 8);

	return 0;
}


// 检查数据 POOL_DEBUG
static int __check_pool(void *element, unsigned int size) 
{

	void *pptr = element;
	void *ptr  = element + 8; 
	//void *ptr  = element + size - POOL_DEBUG_HALF_SIZE + 8;
	void *nptr = element + size - POOL_DEBUG_HALF_SIZE;

	if (memcmp(pptr, pool_front, 8) != 0) {
		printf("[%s] Application memory cross-border: memory 1 wrong\n", (char *)ptr);
		return 	-1;
	}

	if (memcmp(pptr + POOL_DEBUG_HALF_SIZE - 8, pool_rear, 8) != 0) {
		printf("[%s] Application memory cross-border: memory 2 wrong\n", (char *)ptr);
		return 	-2;
	}

	if (memcmp(nptr, pool_front, 8) != 0) {
		printf("[%s] Application memory cross-border: memory 3 wrong\n", (char *)ptr);
		return 	-3;
	}

	if (memcmp(nptr + POOL_DEBUG_HALF_SIZE - 8, pool_rear, 8) != 0) {
		printf("[%s] Application memory cross-border: memory 4 wrong\n", (char *)ptr);
		return 	-4;
	}


	//printf("[check_pool] \"%s\" \n", (char *)ptr);

	return 0;
}






void network_debug(int level, unsigned char *data, int len)
{   
	printf("----------------debug--------------\n");
	HexdumpPool(level, data, len);
	//OX(level, data, len);
	printf("\n");
}

/* 十六进制 */
static char dec2hex[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*
 * 打印形式类似于wireshark,这个更好.....
 * */
void HexdumpPool(int level, void *data, int len)
{
	printf("%p ----------------------------------------- data info ------------------------------------\n", data);

	// 每行显示的字符个数
	int showlen = 0;
	char str[16] = {' '};
	int line = ((uint32_t)len >> 4);
	//int line = ((unsigned int)len >> 4);

	if ((len & 15) != 0)
		line++;

	int i;
	for (i = 0; i < len; i++) {
		uint8_t byte = ((char*)data)[i];
		//unsigned short byte = ((char*)data)[i];

		// 第二个符号
		printf("%c", dec2hex[(byte & 0xf0) >> 4]);
		// 第一个符号
		printf("%c", dec2hex[(byte & 0xf)]);
		// 空格
		printf(" ");
		// 填充
		if ((i == (len - 1)) && (showlen < 15)) {
			printf(" ");
			int i;
			for (i = 0; i < (15 - showlen); i++)
				printf("   ");
		}
		// 输出值 
		// ZL: isprint()检查包含空格的任何可打印字符, 返回0表示不可打印
		str[showlen] = (isprint(byte)) ? byte : '.';
		showlen++;
		// 换行
		if (((i + 1) & 15) == 0) {
			printf(" %.*s", 8, str);
			printf(" %.*s", 8, str + 8);
			printf("\n");
			showlen = 0;
			if (i == (len - 1)) {
				printf("----------------------------------------------------------------------------------------\n");
				return;
			}
		}
	}

	// 对于不足16的进行换行
	for (i = showlen; i <= 15; i++)
		str[i] = ' ';
	printf("%.*s", 8, str);
	printf("%.*s", 8, str + 8);
	printf("\n");
	printf("----------------------------------------------------------------------------------------\n");
}

