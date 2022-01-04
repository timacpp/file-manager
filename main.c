#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "Tree.h"
#include "HashMap.h"
#include "path_utils.h"

void print_map(HashMap* map) {
    const char* key = NULL;
    void* value = NULL;
    printf("Size=%zd\n", hmap_size(map));
    HashMapIterator it = hmap_iterator(map);
    while (hmap_next(map, &it, &key, &value)) {
        printf("Key=%s Value=%p\n", key, value);
    }
    printf("\n");
}

int main(void) {
    Tree* tree = tree_new();
    tree_create(tree, "/a/");
    tree_create(tree, "/a/b/");
    tree_remove(tree, "/a/b/");
    char* content = tree_list(tree, "/a/");
    if (!content) {
        printf("content is empty\n");
    } else {
        printf("%s\n", content);
        free(content);
    }
    tree_free(tree);

    return 0;
}