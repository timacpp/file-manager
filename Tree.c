#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "err.h"
#include "Tree.h"
#include "HashMap.h"
#include "path_utils.h"

#define ECYCLE -1 /* Attempt to move a directory to its subdirectory. */

#define CHECK_PTR(ptr) \
    if (!ptr)          \
        fatal(__FUNCTION__)

#define CHECK_ERR(err)      \
    if ((errno = err) != 0) \
        syserr(__FUNCTION__, err)

#define RETURN_ERR(err) \
    if (err != 0)          \
        return err

struct Tree {
    HashMap* children;
    pthread_rwlock_t lock;
};

/**
 * Creates a new tree with the given children.
 * If @p content is NULL, creates a new hashmap.
 * @return pointer to an allocated tree
 */
static Tree* tree_new_populated(HashMap* content) {
    Tree* root = malloc(sizeof(Tree));
    CHECK_PTR(root);

    root->children = (content ? content : hmap_new());
    CHECK_PTR(root->children);

    CHECK_ERR(pthread_rwlock_init(&root->lock, NULL));

    return root;
}

Tree* tree_new() {
    return tree_new_populated(NULL);
}

/**
 * Frees memory allocated for children of @p parent.
 * @param parent non NULL tree
 */
static void tree_free_children(Tree* parent) {
    void* child;
    const char* folder;
    HashMapIterator it = hmap_iterator(parent->children);

    while (hmap_next(parent->children, &it, &folder, &child)) {
        if (child) {
            tree_free(child);
        }
    }
}

void tree_free(Tree* tree) {
    if (!tree)
        return;

    if (tree->children) {
        tree_free_children(tree);
        hmap_free(tree->children);
    }

    CHECK_ERR(pthread_rwlock_destroy(&tree->lock));
    free(tree);
}

/**
 * Gives a child of @p parent named @p folder
 * or NULL if it does not exist.
 * @param parent tree
 * @param folder name of folder
 * @return child or NULL
 */
static Tree* tree_get_child(Tree* parent, const char* folder) {
    return hmap_get(parent->children, folder);
}

/**
 * Writes a subtree of @p tree located by path @p path to @p subtree.
 * Assumes @p path is valid and not NULL; @p tree is not NULL.
 * @param tree root node for search
 * @param subtree pointer to assign a founded tree
 * @param path target tree location
 * @return 0 in case of success and a custom error otherwise
 */
static int tree_extract_safe(Tree* tree, Tree** subtree, const char* path) {
    *subtree = tree;
    const char* subpath = path;
    char folder_buf[MAX_FOLDER_NAME_LENGTH + 1];

    while (*subtree && strcmp(subpath, "/") != 0) {
        subpath = split_path(subpath, folder_buf);

        CHECK_ERR(pthread_rwlock_rdlock(&(*subtree)->lock));
        Tree* next_subtree = tree_get_child(*subtree, folder_buf);
        CHECK_ERR(pthread_rwlock_unlock(&(*subtree)->lock));

        *subtree = next_subtree;
    }

    return *subtree ? 0 : ENOENT;
}

static int tree_extract(Tree* tree, Tree** subtree, const char* path) {
    if (!path || !is_path_valid(path))
        return EINVAL;

    return tree_extract_safe(tree, subtree, path);
}

/**
 * Writes a parent to @p parent of tree located by path @p path
 * starting from @p tree root. Also writes child folder name
 * to @p folder.
 * Assumes @p tree is not NULL; @p path is valid and not NULL;
 * @p folder is a buffer of at least MAX_FOLDER_NAME + 1 size.
 * @param tree root node for search
 * @param parent pointer to assign a founded parent
 * @param path target parent tree location
 * @param folder buffer to assign a name of child folder
 * @return 0 in case of success and a custom error otherwise
 */
static int tree_extract_parent_safe(Tree* tree, Tree** parent,
                                    const char* path, char* folder) {
    char* subpath = make_path_to_parent(path, folder);
    int err = (subpath ? tree_extract_safe(tree, parent, subpath) : EBUSY);

    free(subpath);

    return err;
}

static int tree_extract_parent(Tree* tree, Tree** parent,
                               const char* path, char* folder) {
    if (!path || !is_path_valid(path))
        return EINVAL;

    return tree_extract_parent_safe(tree, parent, path, folder);
}


char* tree_list(Tree* tree, const char* path) {
    Tree* subtree;
    int err = tree_extract(tree, &subtree, path);

    if (err != 0)
        return NULL;

    CHECK_ERR(pthread_rwlock_rdlock(&subtree->lock));
    char* list = make_map_contents_string(subtree->children);
    CHECK_ERR(pthread_rwlock_unlock(&subtree->lock));

    return list;
}

static int tree_add_child(Tree* parent, const char* folder, HashMap* content) {
    if (tree_get_child(parent, folder))
        return EEXIST;

    Tree* child = tree_new_populated(content);
    hmap_insert(parent->children, folder, child);

    return 0;
}

int tree_create(Tree* tree, const char* path) {
    Tree* parent;
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    int err = tree_extract_parent(tree, &parent, path, folder);

    if (err)
        return err == EBUSY ? EEXIST : err;

    RETURN_ERR(pthread_rwlock_wrlock(&parent->lock));
    err = tree_add_child(parent, folder, NULL);
    RETURN_ERR(pthread_rwlock_unlock(&parent->lock));

    return err;
}

static int tree_erase_child(Tree* parent, const char* folder) {
    Tree* child = tree_get_child(parent, folder);

    if (!child)
        return ENOENT;
    if (child->children && hmap_size(child->children) > 0)
        return ENOTEMPTY;

    tree_free(child);
    hmap_remove(parent->children, folder);

    return 0;
}

int tree_remove(Tree* tree, const char* path) {
    Tree* parent;
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    RETURN_ERR(tree_extract_parent(tree, &parent, path, folder));

    RETURN_ERR(pthread_rwlock_wrlock(&parent->lock));
    int err = tree_erase_child(parent, folder);
    RETURN_ERR(pthread_rwlock_unlock(&parent->lock));

    return err;
}

static int tree_move_child(Tree* source_parent, Tree* target_parent,
                           const char* source_folder, const char* target_folder) {
    bool same_folder = (strcmp(source_folder, target_folder) == 0);
    Tree* source_tree = tree_get_child(source_parent, source_folder);

    if (!source_tree)
        return ENOENT;
    if (source_parent == target_parent && same_folder)
        return 0;
    if (tree_add_child(target_parent, target_folder, source_tree->children))
        return EEXIST;

    source_tree->children = NULL;
    RETURN_ERR(tree_erase_child(source_parent, source_folder));

    return 0;
}

static int tree_move_non_root(Tree* tree, const char* source, const char* target) {
    Tree* source_parent, *target_parent;
    char source_folder[MAX_FOLDER_NAME_LENGTH + 1];
    char target_folder[MAX_FOLDER_NAME_LENGTH + 1];

    RETURN_ERR(tree_extract_parent_safe(tree, &source_parent, source, source_folder));
    RETURN_ERR(tree_extract_parent_safe(tree, &target_parent, target, target_folder));

    RETURN_ERR(pthread_rwlock_wrlock(&tree->lock));

//    RETURN_ERR(pthread_rwlock_wrlock(&source_parent->lock));
//    RETURN_ERR(pthread_rwlock_wrlock(&target_parent->lock));

    int err = tree_move_child(source_parent, target_parent, source_folder, target_folder);

//    RETURN_ERR(pthread_rwlock_unlock(&target_parent->lock));
//    RETURN_ERR(pthread_rwlock_unlock(&source_parent->lock));
    RETURN_ERR(pthread_rwlock_unlock(&tree->lock));

    return err;
}

int tree_move(Tree* tree, const char* source, const char* target) {
    if (!source || !target || !is_path_valid(source) || !is_path_valid(target))
        return EINVAL;
    if (strcmp(source, "/") == 0)
        return EBUSY;
    if (strcmp(target, "/") == 0)
        return EEXIST;
    if (is_subpath(source, target))
        return ECYCLE;

    return tree_move_non_root(tree, source, target);
}