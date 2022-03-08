# General info
This is a thread-safe library for *abstract* file management.
Library provides tools for creating, removing, moving folders and listing their content.

# Path representation
Folders are represented as sequences of lower ASCII letters from **a** to **z** with a maximum
lengths of **255**.

Paths are folder names separated with **/**. Note, that each path is at most **4095**
characters long contains a separator at the beginning and end.

With this definition ```/``` is considered a valid path.

# Hierarchy representation
General Tree data structure is used to represent a folder hierarchy. Therefore, each node is
either a tree storing references to its children or a ```NULL```. For details, see ```tree.c```.

# Concurrency
Each of the mentioned operations is **atomic**.
Meaning that if operations (of a single hierarchy) are called *concurrently*,
the resulting hierarchy will be same as if the operations was processed
sequentially in some order.

Shortly, there is *no guarantee about the order* of concurrently processed operations.

# Error handling
There exists a lot of edge cases with no rational outcome. For example:
  - creating an already existing folder
  - removing an non-existing folder
  - moving a folder to its subfolder

Such cases result in error (error codes are presented in documentation).
In case of occurred error, the hierarchy's state is preserved.