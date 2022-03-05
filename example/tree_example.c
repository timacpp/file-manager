/** @file
 * Example usage of file manager.
 * @author Tymofii Vedmedenko <tv433559@students.mimuw.edu.pl>
 * @date 2022
*/

#include <stdio.h>
#include <stdlib.h>

#include "../src/tree.h"

int main(void) {
    /* Creates the root folder */
    Tree* tree = tree_new();

    /* Creates folder 'a' inside root folder */
    tree_create(tree, "/a/");

    /* Creates folder 'b' inside folder 'a' */
    tree_create(tree, "/a/b/");

    /* Removes folder 'b' from folder 'a' */
    tree_remove(tree, "/a/b/");

    /* Renames folder 'a' to 'b' */
    tree_move(tree, "/a/", "/b/");

    /* Creates folder 'c' inside folder 'b' */
    tree_create(tree, "/b/c/");

    /* Moves folder 'c' from folder 'b' to the root */
    tree_move(tree, "/b/c/", "/");

    /* This results in two folders in the root: 'b' and 'c' */
    char* content = tree_list(tree, "/");
    printf("%s\n", content);

    free(content);
    tree_free(tree);

    return 0;
}