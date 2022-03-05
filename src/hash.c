#include <stdlib.h>
#include <string.h>

#include "hash.h"

/* We fix the number of hash buckets for simplicity. */
#define BUCKETS_COUNT 8

typedef struct Pair Pair;

struct Pair {
    char* key;
    void* value;
    Pair* next; // Next item in a single-linked list.
};

struct HashMap {
    Pair* buckets[BUCKETS_COUNT]; // Linked lists of key-value pairs.
    size_t size; // total number of entries in map.
};

static unsigned int get_hash(const char* key);

HashMap* hmap_new() {
    HashMap* map = malloc(sizeof(HashMap));

    if (!map)
        return NULL;

    memset(map, 0, sizeof(HashMap));
    return map;
}

void hmap_free(HashMap* map) {
    for (int h = 0; h < BUCKETS_COUNT; ++h) {
        for (Pair* p = map->buckets[h]; p;) {
            Pair* q = p;
            p = p->next;
            free(q->key);
            free(q);
        }
    }

    free(map);
}

static Pair* hmap_find(HashMap* map, int h, const char* key) {
    for (Pair* p = map->buckets[h]; p; p = p->next) {
        if (strcmp(key, p->key) == 0)
            return p;
    }

    return NULL;
}

void* hmap_get(HashMap* map, const char* key) {
    int h = get_hash(key);
    Pair* p = hmap_find(map, h, key);

    return p ? p->value : NULL;
}

bool hmap_insert(HashMap* map, const char* key, void* value) {
    if (!value)
        return false;

    int h = get_hash(key);
    Pair* p = hmap_find(map, h, key);

    if (p)
        return false; // Already exists.

    Pair* new_p = malloc(sizeof(Pair));
    new_p->key = strdup(key);
    new_p->value = value;
    new_p->next = map->buckets[h];
    map->buckets[h] = new_p;
    map->size++;

    return true;
}

bool hmap_remove(HashMap* map, const char* key) {
    int h = get_hash(key);
    Pair** pp = &(map->buckets[h]);

    while (*pp) {
        Pair* p = *pp;

        if (strcmp(key, p->key) == 0) {
            *pp = p->next;
            free(p->key);
            free(p);
            map->size--;
            return true;
        }

        pp = &(p->next);
    }

    return false;
}

size_t hmap_size(HashMap* map) {
    return map->size;
}

HashMapIterator hmap_iterator(HashMap* map) {
    return (HashMapIterator){0, map->buckets[0]};
}

bool hmap_next(HashMap* map, HashMapIterator* it, const char** key, void** value) {
    Pair* p = it->pair;

    while (!p && it->bucket < BUCKETS_COUNT - 1) {
        p = map->buckets[++it->bucket];
    }

    if (!p)
        return false;

    *key = p->key;
    *value = p->value;
    it->pair = p->next;

    return true;
}

static unsigned int get_hash(const char* key) {
    unsigned int hash = 17;

    while (*key) {
        hash += (hash << 3) + *key;
        ++key;
    }

    return hash % BUCKETS_COUNT;
}
