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

static Tree* tree_new_populated(HashMap* content) {
    Tree* root = malloc(sizeof(Tree));
    CHECK_PTR_FATAL(root, "Failed allocation for root in tree_new.");

    root->children = content;

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

    while (*subtree && !is_delimiter(subpath)) {
        subpath = split_path(subpath, folder_buf);
        *subtree = tree_get_child(*subtree, folder_buf);
    }

    return *subtree ? 0 : ENOENT;
}

static int tree_extract(Tree* tree, Tree** subtree, const char* path) {
    if (!path || !is_path_valid(path))
        return EINVAL;

    return tree_extract_safe(tree, subtree, path);
}

static int tree_extract_parent(Tree* tree, Tree** parent,
                               const char* path, char* folder) {
    if (!path || !is_path_valid(path))
        return EINVAL;

    char* subpath = make_path_to_parent(path, folder);
    int err = (subpath ? tree_extract_safe(tree, parent, subpath) : EBUSY);

    free(subpath);

    return err;
}

char* tree_list(Tree* tree, const char* path) {
    Tree* subtree;
    int err = tree_extract(tree, &subtree, path);
    return err == 0 ? make_map_contents_string(subtree->children) : NULL;
}

static int tree_add_child(const Tree* tree, const char* folder, HashMap* content) {
    if (tree_get_child(tree, folder)) {
        hmap_free(content);
        return EEXIST;
    }

    Tree* child = tree_new_populated(content);
    hmap_insert(tree->children, folder, child);

    return 0;
}

static int tree_create_populated(Tree* tree, const char* path, HashMap* content) {
    CHECK_PTR_FATAL(content, "Content of a folder cannot be null.");

    Tree* subtree;
    char folder[MAX_FOLDER_NAME_LENGTH + 1];
    int err = tree_extract_parent(tree, &subtree, path, folder);

    return err == 0 ? tree_add_child(subtree, folder, content) : err;
}

int tree_create(Tree* tree, const char* path) {
    return tree_create_populated(tree, path, hmap_new());
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

int tree_move(Tree* tree, const char* source, const char* target) {
    if (strcmp(source, target) == 0)
        return 0;

    int err;
    Tree* src_tree;
    Tree* src_parent;
    char src_folder[MAX_FOLDER_NAME_LENGTH + 1];

    if ((err = tree_extract_parent(tree, &src_parent, source, src_folder)))
        return err;

    if ((src_tree = tree_get_child(src_parent, src_folder)) == NULL)
        return ENOENT;

    if ((err = tree_create_populated(tree, target, src_tree->children)))
        return err;

    src_tree->children = NULL;
    tree_erase_child(src_parent, src_folder);

    return 0;
}