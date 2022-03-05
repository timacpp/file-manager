/** @file
 * Hashmap storing universal pointers.
 * @date 2022
*/

#pragma once

#include <stdbool.h>
#include <sys/types.h>

typedef struct HashMap HashMap;

/**
 * Creates empty hash map.
 * @return allocated map
 */
HashMap* hmap_new();

/**
 * Clear the map and free its memory. This frees the map and the keys
 * copied by hmap_insert, but does not free any values.
 * @param map map to free
 */
void hmap_free(HashMap* map);

/**
 * Get the value stored under `key`, or NULL if not present.
 * @return value under given key
 */
void* hmap_get(HashMap* map, const char* key);

/**
 * Insert a `value` under `key` and return true, or do nothing
 * and return false if `key` already exists in the map. The caller
 * can free `key` at any time - the map internally uses a copy of it.
 * @return is @p key unused in @p map?
 */
bool hmap_insert(HashMap* map, const char* key, void* value);

/**
 * Removes the value under `key` and return true (the value is not free'd),
 * or do nothing and return false if `key` was not present.
 * @return
 */
bool hmap_remove(HashMap* map, const char* key);

size_t hmap_size(HashMap* map);

typedef struct HashMapIterator HashMapIterator;

/**
 * Return an iterator to the map. See `hmap_next`.
 * @return begin iterator
 */
HashMapIterator hmap_iterator(HashMap* map);

/**
 * Set `*key` and `*value` to the current element pointed by iterator and
 * move the iterator to the next element. If there are no more elements,
 * leaves `*key` and `*value` unchanged and returns false.
 * @param map map to perform search
 * @param it current entry iterator
 * @param key where to store key
 * @param value where to store value
 * @return is @p it not representing @p map end?
 */
bool hmap_next(HashMap* map, HashMapIterator* it, const char** key, void** value);

struct HashMapIterator {
    int bucket;
    void* pair;
};
