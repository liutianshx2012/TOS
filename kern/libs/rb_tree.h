/*************************************************************************
	> File Name: rb_tree.h
	> Author: TTc  
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Wed 27 Jul 2016 05:42:14 PM PDT
 ************************************************************************/
#ifndef __KERN_LIBS_RB_TREE_H
#define __KERN_LIBS_RB_TREE_H

#include <defs.h>

typedef struct rb_node
{
	bool red; // if red =0, it's a black node
	struct rb_node *parent;
	struct rb_node *left, *right;
} rb_node;

typedef struct rb_tree
{
	// return -1 if *node1 < *node 2 ; return 1 if *node1 > *node2, return 0 otherwise
	int (*compare)(rb_node *node1, rb_node *node2);
	struct rb_node *nil, *root;
} rb_tree;

rb_tree *rb_tree_create(int (*compare)(rb_node *node1, rb_node *node2));
void rb_tree_destroy(rb_tree *tree);
void rb_insert(rb_tree *tree, rb_node *node);
void rb_delete(rb_tree *tree, rb_node *node);
rb_node *rb_search(rb_tree *tree, int (*compare)(rb_node *node, void *key), void *key);
rb_node *rb_node_prev(rb_tree *tree, rb_node *node);
rb_node *rb_node_next(rb_tree *tree, rb_node *node);
rb_node *rb_node_root(rb_tree *tree);
rb_node *rb_node_left(rb_tree *tree, rb_node *node);
rb_node *rb_node_right(rb_tree *tree, rb_node *node);

void check_rb_tree(void);

#endif
