#include "hash.h"

typedef struct hash_node
{
	void *key;
	void *value;
	struct hash_node *prev;
	struct hash_node *next;
}hash_node_t;

struct hash
{
	unsigned int buckets;//桶的个数
	hashfunc_t hash_func;//哈希函数
	hash_node_t **nodes;//哈希表中存放的一些链表的地址
};

hash_node_t** hash_get_bucket(hash_t *hash, void *key);
hash_node_t* hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size);

hash_t* hash_alloc(unsigned int buckets, hashfunc_t hash_func)//创建hash表
{
	hash_t *hash = (hash_t *)malloc(sizeof(hash_t));
	assert(hash != NULL);
	hash->buckets = buckets;
	hash->hash_func = hash_func;
	int size = buckets * sizeof(hash_node_t*);
	hash->nodes = (hash_node_t **)malloc(size);
	memset(hash->nodes, 0, size);
	return hash;
}

void* hash_lookup_entry(hash_t *hash, void* key, unsigned int key_size)//在hash表中查找
{
	hash_node_t *node = hash_get_node_by_key(hash, key, key_size);
	if(node == NULL)
		return NULL;

	return node->value;
}

void hash_add_entry(hash_t *hash, void *key, unsigned int key_size,
	void *value, unsigned int value_size)//往hash表中添加一项
{
	if(hash_lookup_entry(hash, key, key_size))
	{
		fprintf(stderr, "duplicate hash key.\n");
		return;
	}

	hash_node_t *node = malloc(sizeof(hash_node_t));
	node->prev = NULL;
	node->next = NULL;

	node->key = malloc(key_size);
	memcpy(node->value, value, value_size);

	hash_node_t **bucket = hash_get_bucket(hash, key);
	if(*bucket == NULL)
		*bucket = node;
	else
	{
		node->next = *bucket;
		(*bucket)->prev = node;
		*bucket = node;
	}
}

void hash_free_entry(hash_t *hash, void *key, unsigned int key_size)
{
	hash_node_t *node = hash_get_node_by_key(hash, key, key_size);
	if(node == NULL)
		return;

	free(node->key);
	free(node->value);

	if(node->prev)
		node->prev->next = node->next;
	else
	{
		hash_node_t **bucket = hash_get_bucket(hash, key);
		*bucket = node->next;
	}
	if(node->next)
		node->next->prev = node->prev;

	free(node);
}

//获取桶地址
hash_node_t** hash_get_bucket(hash_t *hash, void *key)//桶号根据哈希函数得到
{
	unsigned int bucket = hash->hash_func(hash->buckets, key);
	if (bucket >= hash->buckets)//桶号不能超过桶的个数的大小
	{
		fprintf(stderr, "bad bucket lookup\n");
		exit(EXIT_FAILURE);
	}

	return &(hash->nodes[bucket]);
}

//根据key获取hash表中一个结点
hash_node_t* hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size)
{
	hash_node_t **bucket = hash_get_bucket(hash, key);//根据关键码获取链表头指针的地址
	hash_node_t *node = *bucket;//*bucket这个是链表头指针
	if (node == NULL)
	{
		return NULL;
	}

	while (node != NULL && memcmp(node->key, key, key_size) != 0)//将关键码与链表中的结点的关键码进行比较
	{
		node = node->next;
	}

	return node;
}