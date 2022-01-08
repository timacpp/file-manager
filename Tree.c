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
        fatal("allocation failed in ", __FUNCTION__)

#define CHECK_ERR(err)  \
    if (err != 0)       \
        syserr(__FUNCTION__, err)

#define RETURN_IF_ERR(err) \
    if (err != 0)          \
        return err

struct Tree {
    HashMap* children;
    pthread_cond_t reader;
    pthread_cond_t writer;
    pthread_mutex_t mutex;
};

static int tree_lock(Tree* tree) {
    return pthread_mutex_lock(&tree->mutex);
}

static int tree_unlock(Tree* tree) {
    return pthread_mutex_unlock(&tree->mutex);
}

static void tree_lock_checked(Tree* tree) {
    CHECK_ERR(tree_lock(tree));
}

static void tree_unlock_checked(Tree* tree) {
    CHECK_ERR(tree_unlock(tree));
}

static void tree_mutex_init(Tree* tree) {
    CHECK_ERR(pthread_mutex_init(&tree->mutex, NULL));
}

static void tree_mutex_destroy(Tree* tree) {
    CHECK_ERR(pthread_mutex_destroy(&tree->mutex));
}

static Tree* tree_new_populated(HashMap* content) {
    Tree* root = malloc(sizeof(Tree));
    CHECK_PTR(root);

    root->children = (content ? content : hmap_new());
    CHECK_PTR(root->children);

    tree_mutex_init(root);

    return root;
}

Tree* tree_new() {
    return tree_new_populated(NULL);
}

static void tree_free_children(Tree* tree) {
    void* child;
    const char* folder;
    HashMapIterator it = hmap_iterator(tree->children);

    while (hmap_next(tree->children, &it, &folder, &child)) {
        tree_free(child);
    }
}

void tree_free(Tree* tree) {
    if (!tree)
        return;

    tree_lock_checked(tree);

    if (tree->children) {
        tree_free_children(tree);
        hmap_free(tree->children);
    }

    tree_unlock_checked(tree);
    tree_lock_checked(tree);
    tree_mutex_destroy(tree);

    free(tree);
}

static Tree* tree_get_child(Tree* tree, const char* folder) {
    if (!tree)
        return NULL;

    tree_lock_checked(tree); // TODO: check or NULL?
    Tree* child = hmap_get(tree->children, folder);
    tree_unlock_checked(tree);

    return child;
}

static int tree_extract_safe(Tree* tree, Tree** subtree, const char* path) {
    *subtree = tree;
    const char* subpath = path;
    char folder_buf[MAX_FOLDER_NAME_LENGTH + 1];

    while (*subtree && strcmp(subpath, "/") != 0) {
        subpath = split_path(subpath, folder_buf);
        *subtree = tree_get_child(*subtree, folder_buf);
    }

    return *subtree ? 0 : ENOENT;
}

static int tree_extract(Tree* tree, Tree** subtree, const char* path) {
    if (!is_path_valid(path))
        return EINVAL;

    return tree_extract_safe(tree, subtree, path);
}

static int tree_extract_parent_safe(Tree* tree, Tree** parent,
                                    const char* path, char* folder) {
    char* subpath = make_path_to_parent(path, folder);
    int err = (subpath ? tree_extract_safe(tree, parent, subpath) : EBUSY);

    free(subpath);

    return err;
}

static int tree_extract_parent(Tree* tree, Tree** parent,
                               const char* path, char* folder) {
    if (!is_path_valid(path))
        return EINVAL;

    return tree_extract_parent_safe(tree, parent, path, folder);
}

char* tree_list(Tree* tree, const char* path) {
    Tree* subtree;
    int err = tree_extract(tree, &subtree, path);

    if (err != 0)
        return NULL;

    tree_lock_checked(tree); // TODO: check or NULL?
    char* list = make_map_contents_string(subtree->children);
    tree_unlock_checked(tree);

    return list;
}

static int tree_add_child(Tree* tree, const char* folder, HashMap* content) {
    if (tree_get_child(tree, folder))
        return EEXIST;

    RETURN_IF_ERR(tree_lock(tree));

    Tree* child = tree_new_populated(content);
    hmap_insert(tree->children, folder, child);

    RETURN_IF_ERR(tree_unlock(tree));

    return 0;
}

int tree_create(Tree* tree, const char* path) {
    Tree* subtree;
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    int err = tree_extract_parent(tree, &subtree, path, folder);

    if (err == EBUSY)
        return EEXIST;

    return err == 0 ? tree_add_child(subtree, folder, NULL) : err;
}

static int tree_erase_child(Tree* tree, const char* folder) {
    Tree* child = tree_get_child(tree, folder);

    if (!child)
        return ENOENT;

    RETURN_IF_ERR(tree_lock(child));

    if (child->children && hmap_size(child->children) > 0) {
        RETURN_IF_ERR(tree_unlock(child));
        return ENOTEMPTY;
    }

    RETURN_IF_ERR(tree_unlock(child));

    tree_free(child);

    RETURN_IF_ERR(tree_lock(tree));
    hmap_remove(tree->children, folder);
    RETURN_IF_ERR(tree_unlock(tree));

    return 0;
}

int tree_remove(Tree* tree, const char* path) {
    Tree* subtree;
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    RETURN_IF_ERR(tree_extract_parent(tree, &subtree, path, folder));

    return tree_erase_child(subtree, folder);
}

static int tree_move_child(Tree* source_parent, Tree* target_parent,
                           const char* source_folder, const char* target_folder) {
    bool same_folder = (strcmp(source_folder, target_folder) == 0);
    Tree* source_tree = tree_get_child(source_parent, source_folder);

    if (!source_tree)
        return ENOENT;
    if (source_parent == target_parent && same_folder)
        return 0;

    RETURN_IF_ERR(tree_lock(source_tree));

    if (tree_add_child(target_parent, target_folder, source_tree->children)) {
        RETURN_IF_ERR(tree_unlock(source_tree));
        return EEXIST;
    }

    source_tree->children = NULL;
    RETURN_IF_ERR(tree_unlock(source_tree));

    tree_erase_child(source_parent, source_folder);

    return 0;
}

static int tree_move_safe(Tree* tree, const char* source, const char* target) {
    Tree* source_parent, *target_parent;
    char source_folder[MAX_FOLDER_NAME_LENGTH + 1];
    char target_folder[MAX_FOLDER_NAME_LENGTH + 1];

    RETURN_IF_ERR(tree_extract_parent_safe(tree, &source_parent, source, source_folder));
    RETURN_IF_ERR(tree_extract_parent_safe(tree, &target_parent, target, target_folder));

    return tree_move_child(source_parent, target_parent, source_folder, target_folder);
}

int tree_move(Tree* tree, const char* source, const char* target) {
    if (!is_path_valid(source) || !is_path_valid(target))
        return EINVAL;
    if (strcmp(source, "/") == 0)
        return EBUSY;
    if (strcmp(target, "/") == 0)
        return EEXIST;
    if (is_subpath(source, target))
        return ECYCLE;

    return tree_move_safe(tree, source, target);
}