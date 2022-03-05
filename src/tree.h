/** @file
 * Tree representation of file hierarchy.
 * @author Tymofii Vedmedenko <tv433559@students.mimuw.edu.pl>
 * @date 2022
*/

#pragma once

typedef struct Tree Tree;

Tree* tree_new();

void tree_free(Tree*);

/**
 * Creates an allocated sequence of characters representing the
 * lexicographically sorted content of a folder under a given path.
 * If @p path is NULL or invalid (see is_path_valid in src/util/paths.c), returns NULL.
 * @param tree file hierarchy
 * @param path folder, content of which to list
 * @return content of @p path
 */
char* tree_list(Tree* tree, const char* path);

/**
 * Creates a folder @p path in @p tree. Returns:
 * EINVAL - @p path NULL or invalid (see is_path_valid in src/util/paths.c);
 * EEXIST - @p path already exists inside @p tree;
 * ENOENT - parent of @p path does not exist;
 * 0 - otherwise;
 * @param tree file hierarchy
 * @param path folder to create
 * @return error code or zero if none occurred
 */
int tree_create(Tree* tree, const char* path);

/**
 * Removes a folder @p path in @p tree. Returns:
 * EINVAL - @p path NULL or invalid (see is_path_valid in src/util/paths.c);
 * ENOEMPTY - @p path is not empty (contains at least one folder);
 * ENOENT - @p path does not exist;
 * EBUSY - @p path is a root folder, which is "/";
 * 0 - otherwise;
 * @param tree file hierarchy
 * @param path folder to remove
 * @return error code or zero if none occurred
 */
int tree_remove(Tree* tree, const char* path);

/**
 * Moves a folder @p source to @p target in @p tree. Returns:
 * EINVAL - @p source (or @p target) is NULL or invalid (see is_path_valid in src/util/paths.c);
 * ENOENT - @p source or parent of @p target does not exist;
 * EEXIST - @p target already exists;
 * EBUSY - @p source is a root folder, which is "/";
 * ECYCLE - @p target is a subfolder of a @p source;
 * 0 - otherwise;
 * @param tree file hierarchy
 * @param source folder to move
 * @param target where to move folder
 * @return error code or zero if none occurred
 */
int tree_move(Tree* tree, const char* source, const char* target);