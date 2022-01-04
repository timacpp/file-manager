#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "Tree.h"
#include "HashMap.h"
#include "path_utils.h"

struct Tree {
    HashMap* children;
};

Tree* tree_new() {
    Tree* root = malloc(sizeof(Tree));

    if (root) {
        root->children = hmap_new();
    }

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

extern Tree* tree_get_child_folder(Tree* tree, const char* path,
                                   const char** subpath, char* folder) {
    if (subpath) {
        *subpath = split_path(path, folder);
    } else {
        split_path(path, folder);
    }

    return hmap_get(tree->children, folder);
}

extern Tree* tree_get_child(Tree* tree, const char* path,
                            const char** subpath, char* folder) {
    if (folder) {
        return tree_get_child_folder(tree, path, subpath, folder);
    } else {
        char folder_buf[MAX_FOLDER_NAME_LENGTH + 1];
        return tree_get_child_folder(tree, path, subpath, folder_buf);
    }
}

extern char* tree_list_valid(Tree* tree, const char* path) {
    if (!tree)
        return NULL;

    const char* subpath = NULL;
    Tree* child = tree_get_child(tree, path, &subpath, NULL);

    return is_delimiter(path) ? make_map_contents_string(tree->children)
                              : tree_list_valid(child, subpath);
}

extern int tree_add_child(Tree* tree, const char* folder) {
    return hmap_insert(tree->children, folder, tree_new()) ? 0 : EEXIST;
}

extern int tree_create_valid(Tree* tree, const char* path) {
    if (!tree)
        return ENOENT;

    const char* subpath = NULL;
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    Tree* child = tree_get_child(tree, path, &subpath, folder);

    return is_delimiter(subpath) ? tree_add_child(tree, folder)
                                 : tree_create_valid(child, subpath);
}

extern int tree_erase_child(Tree* tree, const char* folder) {
    Tree* child = hmap_get(tree->children, folder);
    bool empty_child = (hmap_size(child->children) == 0);

    if (empty_child) {
        tree_free(child);
        hmap_remove(tree->children, folder);
    }

    return empty_child ? 0 : ENOTEMPTY;
}

extern int tree_remove_valid(Tree* tree, const char* path) {
    if (!tree)
        return ENOENT;

    const char* subpath = NULL;
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    Tree* child = tree_get_child(tree, path, &subpath, folder);

    return is_delimiter(subpath) ? tree_erase_child(tree, folder)
                                 : tree_remove_valid(child, subpath);
}

extern int tree_move_valid(Tree* tree, const char* path, const char* target) {
    return 0;
}

char* tree_list(Tree* tree, const char* path) {
    return is_path_valid(path) ? tree_list_valid(tree, path) : NULL;
}

int tree_create(Tree* tree, const char* path) {
    return is_path_valid(path) ? tree_create_valid(tree, path) : EINVAL;
}

int tree_remove(Tree* tree, const char* path) {
    if (is_delimiter(path))
        return EBUSY;

    return is_path_valid(path) ? tree_remove_valid(tree, path) : EINVAL;
}

int tree_move(Tree* tree, const char* source, const char* target) {
    bool correct_paths = is_path_valid(source) && is_path_valid(target);
    return correct_paths ? tree_move_valid(tree, source, target) : EINVAL;
}


