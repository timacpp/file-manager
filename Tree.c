#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "Tree.h"
#include "HashMap.h"
#include "path_utils.h"

#define CHECK_PTR_FATAL(ptr, msg) if (!ptr) fatal(msg)
#define ECYCLE -1 /* Attempt to move a directory to its subdirectory. */

struct Tree {
    HashMap* children;
};

static Tree* tree_new_populated(HashMap* content) {
    Tree* root = malloc(sizeof(Tree));
    CHECK_PTR_FATAL(root, "Failed allocation for root.");

    root->children = (content ? content : hmap_new());
    CHECK_PTR_FATAL(root->children, "Failed allocation for root children.");

    return root;
}

Tree* tree_new() {
    return tree_new_populated(hmap_new());
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

    if (tree->children) {
        tree_free_children(tree);
        hmap_free(tree->children);
    }

    free(tree);
}

static Tree* tree_get_child(const Tree* tree, const char* folder) {
    return tree ? hmap_get(tree->children, folder) : NULL;
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

static int tree_extract_parent_safe(Tree* tree, Tree** parent, const char* path, char* folder) {
    char* subpath = make_path_to_parent(path, folder);
    int err = (subpath ? tree_extract_safe(tree, parent, subpath) : EBUSY);

    free(subpath);

    return err;
}

static int tree_extract_parent(Tree* tree, Tree** parent, const char* path, char* folder) {
    if (!is_path_valid(path))
        return EINVAL;

    return tree_extract_parent_safe(tree, parent, path, folder);
}

char* tree_list(Tree* tree, const char* path) {
    Tree* subtree;
    int err = tree_extract(tree, &subtree, path);
    return err == 0 ? make_map_contents_string(subtree->children) : NULL;
}

static int tree_add_child(const Tree* tree, const char* folder, HashMap* content) {
    if (tree_get_child(tree, folder))
        return EEXIST;

    Tree* child = tree_new_populated(content);
    hmap_insert(tree->children, folder, child);

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

static int tree_erase_child(const Tree* tree, const char* folder) {
    Tree* child = tree_get_child(tree, folder);

    if (!child)
        return ENOENT;
    if (child->children && hmap_size(child->children) > 0)
        return ENOTEMPTY;

    tree_free(child);
    hmap_remove(tree->children, folder);

    return 0;
}

int tree_remove(Tree* tree, const char* path) {
    Tree* subtree;
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    int err = tree_extract_parent(tree, &subtree, path, folder);

    return err == 0 ? tree_erase_child(subtree, folder) : err;
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
    tree_erase_child(source_parent, source_folder);

    return 0;
}

static int tree_move_non_root(Tree* tree, const char* source, const char* target) {
    int err;
    Tree* source_parent, *target_parent;
    char source_folder[MAX_FOLDER_NAME_LENGTH + 1];
    char target_folder[MAX_FOLDER_NAME_LENGTH + 1];

    if ((err = tree_extract_parent_safe(tree, &source_parent, source, source_folder)))
        return err;
    if ((err = tree_extract_parent_safe(tree, &target_parent, target, target_folder)))
        return err;

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

    return tree_move_non_root(tree, source, target);
}