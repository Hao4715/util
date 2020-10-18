#ifndef __FIXED_POOL_H__
#define __FIXED_POOL_H__

struct Pool;

/*
 * @brief 		创建一个内存池
 * @details		创建一个数据池 --- 调试时使用
 * @param[in]	name	内存池名称
 * @param[in]	size	内存池大小
 * @return		成功返回指向该内存池指针，失败返回NULL
 */
struct Pool *create_pool_debug(char *name, unsigned int size);

/*
 * @brief 		检查内存
 * @details		检查内存 --- 调试时使用
 * @param[in]	element	从内存池中拿出的之后需要检查的内存首地址
 * @param[in]	size	内存池大小
 * @return		成功返回0，失败返回-1 -2 -3 -4
 */
int tmp_check_pool(struct Pool *pool, void *element, char *file, int line);
#define check_pool(pool, element)								\
	tmp_check_pool(pool, element, __FILE__, __LINE__)

/*
 * @brief 		创建一个内存池 
 * @details		创建一个数据池 --- 正常使用
 * @param[in]	name	内存池名称
 * @param[in]	size	内存池大小
 * @return		成功返回指向该内存池指针，失败返回NULL
 */
struct Pool *create_pool(char *name, unsigned int size);

/*
 * @brief 		从数据池pool中获得一个数据单元
 * @details		从数据池pool中获得一个数据单元
 * @param[in]	pool	初始化时返回的内存池首地址
 * @return		成功返回从该内存池中申请的内存首地址指针，失败返回NULL
 */
void *__pool_alloc(struct Pool *pool, const char *file, int line);
#define pool_alloc(pool)										\
	__pool_alloc(pool, __FILE__, __LINE__)


/*
 * @brief 		释放内存
 * @details		释放一个数据结构，并没有真正释放，而是将其放入pool中	
 * @param[in]	pool	初始化时返回的内存池首地址
 * @param[in]	element	从内存池中拿出的之后需要释放的内存首地址
 * @return		成功返回0，失败返回-1 -2 -3 -4
 */
int __pool_free(struct Pool *pool, void *element, const char *file, int line);
#define pool_free(pool, element)								\
	__pool_free(pool, element, __FILE__, __LINE__)


/*
 * @brief 		创建一个内存池
 * @details		创建一个数据池 --- 调试时使用
 * @param[in]	name	内存池名称
 * @param[in]	size	内存池大小
 * @return		成功返回指向该内存池指针，失败返回NULL
 */
// 释放数据池中的空闲数据单元和数据池本身，这里是真正的free
void __pool_destroy(struct Pool *pool, const char *file, int line);
#define pool_destroy(pool)										\
	__pool_destroy(pool, __FILE__, __LINE__)

#endif
