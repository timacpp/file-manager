#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "Tree.h"
#include "HashMap.h"
#include "path_utils.h"

#define CHECK_PTR_FATAL(ptr, msg) if (!ptr) fatal(msg)

struct Tree {
    HashMap* children;
};

Tree* tree_new() {
    Tree* root = malloc(sizeof(Tree));
    CHECK_PTR_FATAL(root, "Failed to allocate memory for root");

    root->children = hmap_new();
    CHECK_PTR_FATAL(root->children, "Failed to allocate memory for children");

    return root;
}

void tree_free(Tree* tree) {
    if (!tree)
        return;

    void* children;
    const char* folder;
    HashMapIterator it = hmap_iterator(tree->children);

    while (hmap_next(tree->children, &it, &folder, &children)) {
        tree_free(children);
    }

    hmap_free(tree->children);
    free(tree);
}

extern int tree_advance_safe(Tree** tree, const char* path, char* folder) {
    char folder_buf[MAX_FOLDER_NAME_LENGTH + 1];
    const char* subpath = path;

    while (*tree && !is_delimiter(subpath)) {
        subpath = split_path(subpath, folder_buf);
        *tree = hmap_get((*tree)->children, folder_buf);
    }

    if (*tree && folder) {
        folder_copy(folder, folder_buf);
    }

    return *tree ? 0 : ENOENT;
}

extern int tree_advance(Tree** tree, const char* path, char* folder) {
    if (!path || !is_path_valid(path))
        return EINVAL;

    return tree_advance_safe(tree, path, folder);
}

extern int tree_advance_to_parent(Tree** tree, const char* path, char* folder) {
    if (!path || !is_path_valid(path))
        return EINVAL;

    const char* subpath = make_path_to_parent(path, folder);
    return subpath ? tree_advance_safe(tree, subpath, NULL) : ENOENT;
}

char* tree_list(Tree* tree, const char* path) {
    int err = tree_advance(&tree, path, NULL);
    return err == 0 ? make_map_contents_string(tree->children) : NULL;
}

extern int tree_add_child(Tree* tree, const char* folder) {
    bool child_unique = hmap_insert(tree->children, folder, tree_new());
    return child_unique ? 0 : EEXIST;
}

int tree_create(Tree* tree, const char* path) {
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    int err = tree_advance_to_parent(&tree, path, folder);
    return err == 0 ? tree_add_child(tree, folder) : err;
}

extern int tree_erase_child(Tree* tree, const char* folder) {
    Tree* child = hmap_get(tree->children, folder);

    if (!child)
        return ENOENT;
    if (hmap_size(child->children) > 0)
        return ENOTEMPTY;

    tree_free(child);
    hmap_remove(tree->children, folder);

    return 0;
}

int tree_remove(Tree* tree, const char* path) {
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    int err = tree_advance_to_parent(&tree, path, folder);
    return err == 0 ? tree_erase_child(tree, folder) : err;
}

int tree_move(Tree* tree, const char* source, const char* target) {
    if (is_delimiter(source))
        return EBUSY;

    int source_err = 0; // Get child of source

    int target_err = 0; // Get parent of target

    if (source_err || target_err) // TODO: backtracking
        return source_err ? source_err : target_err;

//    return tree_add_child();
}