/* 
 * File:   umap_uint64_ptuple.c
 * Author: marcusj6
 *
 * Created on May 30, 2018, 3:49 PM
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "umap_uint64_ptuple.h"
#include "misc.h"

static struct bucket{
    uint64_t key;
    struct tss_tuple * ptuple;
    struct bucket *next;
    struct bucket *previous;
};

struct umap_uint64_ptuple {
    int bucket_cap;
    struct bucket **buckets;
    int num_ptuples;
    struct bucket *_map_iter;
    bool reset;
};

static struct bucket * bucket_init();
static void bucket_free(struct bucket *b);
static inline int umap_up_size(int size);
static int umap_hash(umap_uint64_ptuple *map, uint64_t key);
static void umap_uint64_ptuple_insert_bucket(umap_uint64_ptuple *map, struct bucket *b);
static void umap_rehash(umap_uint64_ptuple *map, int new_size);
umap_uint64_ptuple * umap_uint64_ptuple_init();
void umap_uint64_ptuple_free(umap_uint64_ptuple *);
void umap_uint64_ptuple_insert(umap_uint64_ptuple *, uint64_t, struct tss_tuple *);
void umap_uint64_ptuple_erase(umap_uint64_ptuple *, uint64_t);
struct tss_tuple * umap_uint64_ptuple_find(umap_uint64_ptuple *, uint64_t);
struct tss_tuple * umap_uint64_ptuple_inext(umap_uint64_ptuple *);
void umap_uint64_ptuple_ireset(umap_uint64_ptuple *);
int umap_uint64_ptuple_size(umap_uint64_ptuple *);

static struct bucket * bucket_init(void){
    struct bucket *b = (struct bucket *)safe_malloc(sizeof(struct bucket));
    b->key = -1; 
    b->ptuple = NULL;
    b->next = NULL;
    b->previous = NULL;
    return b;
}

static void bucket_free(struct bucket *b){
    free(b);
}

int umap_uint64_ptuple_size(umap_uint64_ptuple *map){
    return map->num_ptuples;
}

//Finds the first prime number larger than 2 times the provided number
//Searches precomputed list if possible
//Otherwise, computes one with a method that takes on average sqrt(n)log(n)/6
static inline int umap_up_size(int size){
    const int k_num_preprime = 11;
    const static int k_primes[11] = {23, 53, 103, 223, 463, 983, 2003, 4093, 7919, 15859, 31721};
    if(size < k_primes[k_num_preprime-1] / 2){
        int i;
        for(i = 0; i < k_num_preprime; ++i){
            if(size < k_primes[i] / 2){
                return k_primes[i];
            }
        }
    }
    int new_size = 2 * size + 1;
    int prime = 0;
    int i;
    while(!prime){
        prime = 1;
        if (new_size % 2 == 0 || new_size % 3 == 0){
            prime = 0;
            break;
        }
        for(i = 5; i * i <= new_size ; i+=6){
            if(new_size % i == 0 || new_size % (i+2) == 0){
                prime = 0;
                break;
            }
        }
        new_size += 2;
    }
    return new_size;
}
static int umap_hash(umap_uint64_ptuple *map, uint64_t key){
    //Based on Splitmix64 found here http://xorshift.di.unimi.it/splitmix64.c
    //and found here https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
    long long hash = key; 
    hash = (hash ^ (hash >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    hash = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
    hash = hash ^ (hash >> 31);
    return (int)(hash % map->bucket_cap);
}

umap_uint64_ptuple * umap_uint64_ptuple_init(){
    umap_uint64_ptuple *map = (umap_uint64_ptuple *)safe_malloc(sizeof(umap_uint64_ptuple));
    map->bucket_cap = 23;
    map->buckets = (struct bucket **)calloc(map->bucket_cap, sizeof(struct bucket *));
    map->num_ptuples = 0;
    map->_map_iter = NULL;
    map->reset = false;
    return map;
}

void umap_uint64_ptuple_free(umap_uint64_ptuple *map){
    int i, count;
    for(i = 0, count = 0; i < map->bucket_cap; ++i){
        if (map->buckets[i] != NULL){
            struct bucket *current = map->buckets[i];
            while(current != NULL){
                struct bucket *temp = current->next;
                bucket_free(current);
                current = temp;
                count++;
            }
        }
        if(count == map->num_ptuples){//Deleted all ptuples
            break;
        }
    }
    free(map->buckets);
    free(map);
}

static void umap_uint64_ptuple_insert_bucket(umap_uint64_ptuple *map, struct bucket *b){
    int index = umap_hash(map, b->key);
    if(map->buckets[index] == NULL){
        map->buckets[index] = b; 
    }
    else{
        b->next = map->buckets[index];
        map->buckets[index]->previous = b;
        map->buckets[index] = b;
    }
    map->num_ptuples++;
}

static void umap_rehash(umap_uint64_ptuple *map, int new_size){
    int old_size = map->bucket_cap;
    struct bucket **old_map = map->buckets;
    map->bucket_cap = new_size;
    struct bucket **new_map = (struct bucket **)calloc(new_size, sizeof(struct bucket *));
    map->buckets = new_map;
    int i;
    for(i = 0; i < old_size; ++i){
        if(old_map[i] != NULL){
            struct bucket *current = old_map[i];
            struct bucket *temp = NULL;
            while(current != NULL){
                temp = current->next;
                current->previous = NULL;
                current->next = NULL;
                umap_uint64_ptuple_insert_bucket(map, current);
                current = temp;
            }
            old_map[i] = NULL;
        }
    }
    free(old_map);
}

void umap_uint64_ptuple_insert(umap_uint64_ptuple *map, uint64_t key, struct tss_tuple *ptuple){
    double loadfactor = (double)map->num_ptuples / (double)map->bucket_cap;
    double threshold = 0.5;
    if( loadfactor >= threshold ){
        umap_rehash(map, umap_up_size(map->bucket_cap));
    }
    int index = umap_hash(map, key);
    struct bucket *b = bucket_init();
    b->ptuple = ptuple;
    b->key = key;
    if(map->buckets[index] == NULL){
        map->buckets[index] = b; 
    }
    else{
        b->next = map->buckets[index];
        map->buckets[index]->previous = b;
        map->buckets[index] = b;
    }
    map->num_ptuples++;
    //umap_uint64_ptuple_ireset(map);
}

void umap_uint64_ptuple_erase(umap_uint64_ptuple *map, uint64_t key){
    int hash = umap_hash(map, key);
    struct bucket *current = map->buckets[hash];
    while(current != NULL){
        if(current->key == key){
            if(current->previous == NULL){
                map->buckets[hash] = current->next;
            }
            else{
                current->previous->next = current->next;
            }
            if(current->next != NULL){
                current->next->previous = current->previous;
            }
            bucket_free(current);
            current = NULL;
            map->num_ptuples--;
            //umap_uint64_ptuple_ireset(map);
            return;
        }
        current = current->next;
    }
}
struct tss_tuple * umap_uint64_ptuple_find(umap_uint64_ptuple *map, uint64_t key){
    int hash = umap_hash(map, key);
    struct bucket *current = map->buckets[hash];
    while(current != NULL){
        if(current->key == key){
            return current->ptuple;
        }
        current = current->next;
    }
    return NULL;
}

struct tss_tuple * umap_uint64_ptuple_inext(umap_uint64_ptuple *map){
    if(map->num_ptuples == 0 || map->_map_iter == NULL){
        map->_map_iter = NULL;
        map->reset = false;
        return NULL;
    }
    else if(map->_map_iter->next != NULL){
        if(!map->reset)
            map->_map_iter = map->_map_iter->next;
        map->reset = false;
        return map->_map_iter->ptuple;
    }
    else{
        if(map->reset){
            map->reset = false;
            return map->_map_iter->ptuple;
        }  
        int i;
        for(i = umap_hash(map, map->_map_iter->key)+1; i <map->bucket_cap; ++i)
        {
            if(map->buckets[i] != NULL){
                map->_map_iter = map->buckets[i];
                return map->_map_iter->ptuple;
            }
        }
        map->_map_iter = NULL;
        return NULL;
    }
}

void umap_uint64_ptuple_ireset(umap_uint64_ptuple *map){
    map->reset = true;
    if(map->num_ptuples == 0){
        map->_map_iter = NULL;
        return;
    }
    int i;
    for(i = 0; i < map->bucket_cap; ++i){
        if(map->buckets[i] != NULL){
            map->_map_iter = map->buckets[i];
            break;
        }
    }
}

void umap_uint64_ptuple_clear(umap_uint64_ptuple *map){
	int i;
	struct bucket *current, *previous;	
	for(i = 0; i < map->bucket_cap && map->num_ptuples > 0; ++i){
		current = map->buckets[i];
		map->buckets[i] = NULL;
		while(current != NULL){
			previous = current;
			current = current->next;
			bucket_free(previous);
			map->num_ptuples--;
		}
	}
}

