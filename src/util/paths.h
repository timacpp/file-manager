/** @file
 * Utilities for parsing folder names.
 * @date 2022
*/

#pragma once

#include <stdbool.h>

#include "../hash.h"

/** Max length of path (excluding terminating null character) */
#define MAX_PATH_LENGTH 4095

/** Max length of folder name (excluding terminating null character) */
#define MAX_FOLDER_NAME_LENGTH 255

/**
 * Checks if given a sequence represents a valid path.
 * Valid paths are '/'-separated sequences of folder names, always starting and ending with '/'.
 * Valid paths have length at most MAX_PATH_LENGTH (and at least 1). Valid folder names are are
 * sequences of 'a'-'z' ASCII characters, of length from 1 to MAX_FOLDER_NAME_LENGTH.
 * @param path path to validate
 * @return is path valid?
 */
bool is_path_valid(const char* path);

/**
 * Checks whether a path is a subpath.
 * @param subpath valid candidate for subpath
 * @param path valid source paths
 * @return is @p subpath a subpath of @p path?
 */
bool is_subpath(const char* subpath, const char* path);

/**
 * Gives the subpath obtained by removing the first component.
 * @param path valid path to split
 * @param component buffer to store removed component
 * @return @p path without first component
 */
const char* split_path(const char* path, char* component);

/**
 * Gives a copy of the subpath obtained by removing the last component.
 * The caller should free the result, unless it is NULL.
 * @param path path valid path to split
 * @param component component buffer to store removed component
 * @return @p path without last component
 */
char* make_path_to_parent(const char* path, char* component);

/**
 * Return an array containing all keys, lexicographically sorted.
 * The result is null-terminated. *Keys are not copied,
 * they are only valid as long as the map. The caller should free the result.
 * @param map source of keys
 * @return buffer with sorted keys of @p map
 */
const char** make_map_contents_array(HashMap* map);

/**
 * Gives a string containing all keys in map, sorted, comma-separated.
 * The result has no trailing comma. An empty map yields an empty string.
 * The caller should free the result.
 * @param map source of keys
 * @return sequence of formatted keys
 */
char* make_map_contents_string(HashMap* map);