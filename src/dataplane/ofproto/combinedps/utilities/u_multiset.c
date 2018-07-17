#include "u_multiset.h"

void u_multiset_left_rotate(u_multiset* tree, u_multiset_node* x);
void u_multiset_right_rotate(u_multiset* tree, u_multiset_node* x);
u_multiset_node* u_multiset_tree_insert_help(u_multiset* tree, u_multiset_node* z);
void u_multiset_tree_dest_helper(u_multiset* tree, u_multiset_node* x);
void u_multiset_delete_fix_up(u_multiset* tree, u_multiset_node* x);
void u_multiset_delete_node(u_multiset* tree, u_multiset_node* z);
unsigned u_multiset_max_helper(u_multiset* tree, u_multiset_node *node,  unsigned current_max);
unsigned u_multiset_min_helper(u_multiset* tree, u_multiset_node *node,  unsigned current_min);
void u_multiset_create_array_helper(u_multiset* tree, u_multiset_node *node,  unsigned *set_array, int *index);

/***********************************************************************/
/*  FUNCTION:  u_multiset_create */
/**/
/*  INPUTS:  All the inputs are names of functions.  CompFunc takes to */
/*  void pointers to keys and returns 1 if the first arguement is */
/*  "greater than" the second.   DestFunc takes a pointer to a key and */
/*  destroys it in the appropriate manner when the node containing that */
/*  key is deleted.  InfoDestFunc is similiar to DestFunc except it */
/*  recieves a pointer to the info of a node and destroys it. */
/*  PrintFunc recieves a pointer to the key of a node and prints it. */
/*  PrintInfo recieves a pointer to the info of a node and prints it. */
/*  If UMultisetPrint is never called the print functions don't have to be */
/*  defined and null_function can be used.  */
/**/
/*  OUTPUT:  This function returns a pointer to the newly created */
/*  red-black tree. */
/**/
/*  Modifies Input: none */
/***********************************************************************/

u_multiset* u_multiset_create() {
  u_multiset* new_tree;
  u_multiset_node* temp;

  new_tree=(u_multiset*) safe_malloc(sizeof(u_multiset));

  new_tree->size = 0; 

  /*  see the comment in the u_multiset structure in red_black_tree.h */
  /*  for information on nil and root */
  temp=new_tree->nil= (u_multiset_node*) safe_malloc(sizeof(u_multiset_node));
  temp->parent=temp->left=temp->right=temp;
  temp->red=0;
  temp->key=0;
  temp->multiplicity=0;
  temp=new_tree->root= (u_multiset_node*) safe_malloc(sizeof(u_multiset_node));
  temp->parent=temp->left=temp->right=new_tree->nil;
  temp->key=0;
  temp->red=0;
  temp->multiplicity=0;
  return(new_tree);
}

/***********************************************************************/
/*  FUNCTION:  u_multiset_left_rotate */
/**/
/*  INPUTS:  This takes a tree so that it can access the appropriate */
/*           root and nil pointers, and the node to rotate on. */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input: tree, x */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly. */
/***********************************************************************/

void u_multiset_left_rotate(u_multiset* tree, u_multiset_node* x) {
  u_multiset_node* y;
  u_multiset_node* nil=tree->nil;

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls u_multiset_left_rotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when u_multiset_delete_fix_up */
  /*  calls u_multiset_left_rotate it expects the parent pointer of nil to be */
  /*  unchanged. */

  y=x->right;
  x->right=y->left;

  if (y->left != nil) y->left->parent=x; /* used to use sentinel here */
  /* and do an unconditional assignment instead of testing for nil */
  
  y->parent=x->parent;   

  /* instead of checking if x->parent is the root as in the book, we */
  /* count on the root sentinel to implicitly take care of this case */
  if( x == x->parent->left) {
    x->parent->left=y;
  } else {
    x->parent->right=y;
  }
  y->left=x;
  x->parent=y;

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in u_multiset_left_rotate");
#endif
}


/***********************************************************************/
/*  FUNCTION:  RighttRotate */
/**/
/*  INPUTS:  This takes a tree so that it can access the appropriate */
/*           root and nil pointers, and the node to rotate on. */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input?: tree, y */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly. */
/***********************************************************************/

void u_multiset_right_rotate(u_multiset* tree, u_multiset_node* y) {
  u_multiset_node* x;
  u_multiset_node* nil=tree->nil;

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls u_multiset_left_rotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when u_multiset_delete_fix_up */
  /*  calls u_multiset_left_rotate it expects the parent pointer of nil to be */
  /*  unchanged. */

  x=y->left;
  y->left=x->right;

  if (nil != x->right)  x->right->parent=y; /*used to use sentinel here */
  /* and do an unconditional assignment instead of testing for nil */

  /* instead of checking if x->parent is the root as in the book, we */
  /* count on the root sentinel to implicitly take care of this case */
  x->parent=y->parent;
  if( y == y->parent->left) {
    y->parent->left=x;
  } else {
    y->parent->right=x;
  }
  x->right=y;
  y->parent=x;

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in u_multiset_right_rotate");
#endif
}

/***********************************************************************/
/*  FUNCTION:  u_multiset_tree_insert_help  */
/**/
/*  INPUTS:  tree is the tree to insert into and z is the node to insert */
/**/
/*  OUTPUT:  Returns 0 if the node was inserted. Otherwise, returns a pointer to the duplicate node*/
/**/
/*  Modifies Input:  tree, z */
/**/
/*  EFFECTS:  Inserts z into the tree as if it were a regular binary tree */
/*            using the algorithm described in _Introduction_To_Algorithms_ */
/*            by Cormen et al.  If the node is a duplicate, it increases the multiplicity*/
/*	      and returns a pointer to the node. This funciton is only intended to be called */
/*            by the u_multiset_insert function and not by the user */
/***********************************************************************/

u_multiset_node* u_multiset_tree_insert_help(u_multiset* tree, u_multiset_node* z) {
  /*  This function should only be called by InsertUMultiset (see above) */
  u_multiset_node* x;
  u_multiset_node* y;
  u_multiset_node* nil=tree->nil;
  
  z->left=z->right=nil;
  y=tree->root;
  x=tree->root->left;
  while( x != nil) {
    y=x;
    if (x->key > z->key) { 
      x=x->left;
    } else if(x->key < z->key){ 
      x=x->right;
    } else{
      x->multiplicity += z->multiplicity;
      return x;
    }
  }
  z->parent=y;
  if ( (y == tree->root) ||
       (y->key > z->key)) { /* y.key > z.key */
    y->left=z;
  } else {
    y->right=z;
  }

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in u_multiset_tree_insert_help");
#endif
  return NULL;
}

/*  Before calling Insert UMultiset the node x should have its key set */

/***********************************************************************/
/*  FUNCTION:  u_multiset_insert */
/**/
/*  INPUTS:  tree is the red-black tree to insert a node which has a key */
/*           pointed to by key and info pointed to by info.  */
/**/
/*  OUTPUT:  This function returns a pointer to the newly inserted node */
/*           which is guarunteed to be valid until this node is deleted. */
/*           What this means is if another data structure stores this */
/*           pointer then the tree does not need to be searched when this */
/*           is to be deleted. */
/**/
/*  Modifies Input: tree */
/**/
/*  EFFECTS:  Creates a node node which contains the appropriate key and */
/*            info pointers and inserts it into the tree. */
/***********************************************************************/

u_multiset_node * u_multiset_insert(u_multiset* tree, unsigned key, unsigned multiplicity) {
  u_multiset_node * y;
  u_multiset_node * x;
  u_multiset_node * new_node;

  x=(u_multiset_node*) safe_malloc(sizeof(u_multiset_node));
  x->key=key;
  x->multiplicity=multiplicity;
	
  tree->size += multiplicity;  

  new_node = u_multiset_tree_insert_help(tree,x);

  if(new_node == NULL){
	  new_node=x;
	  x->red=1;
	  while(x->parent->red) { /* use sentinel instead of checking for root */
	    if (x->parent == x->parent->parent->left) {
	      y=x->parent->parent->right;
	      if (y->red) {
		x->parent->red=0;
		y->red=0;
		x->parent->parent->red=1;
		x=x->parent->parent;
	      } else {
		if (x == x->parent->right) {
		  x=x->parent;
		  u_multiset_left_rotate(tree,x);
		}
		x->parent->red=0;
		x->parent->parent->red=1;
		u_multiset_right_rotate(tree,x->parent->parent);
	      } 
	    } else { /* case for x->parent == x->parent->parent->right */
	      y=x->parent->parent->left;
	      if (y->red) {
		x->parent->red=0;
		y->red=0;
		x->parent->parent->red=1;
		x=x->parent->parent;
	      } else {
		if (x == x->parent->left) {
		  x=x->parent;
		  u_multiset_right_rotate(tree,x);
		}
		x->parent->red=0;
		x->parent->parent->red=1;
		u_multiset_left_rotate(tree,x->parent->parent);
	      } 
	    }
	  }
	  tree->root->left->red=0;
  } else{ //Node already existed in the tree
  	  free(x);
  }

  return(new_node);
#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in u_multiset_insert");
  Assert(!tree->root->red,"root not red in u_multiset_insert");
#endif
}

/***********************************************************************/
/*  FUNCTION:  u_multiset_tree_successor  */
/**/
/*    INPUTS:  tree is the tree in question, and x is the node we want the */
/*             the successor of. */
/**/
/*    OUTPUT:  This function returns the successor of x or NULL if no */
/*             successor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/
  
u_multiset_node* u_multiset_tree_successor(u_multiset* tree,u_multiset_node* x) { 
  u_multiset_node* y;
  u_multiset_node* nil=tree->nil;
  u_multiset_node* root=tree->root;

  if (nil != (y = x->right)) { /* assignment to y is intentional */
    while(y->left != nil) { /* returns the minium of the right subtree of x */
      y=y->left;
    }
    return(y);
  } else {
    y=x->parent;
    while(x == y->right) { /* sentinel used instead of checking for nil */
      x=y;
      y=y->parent;
    }
    if (y == root) return(nil);
    return(y);
  }
}

/***********************************************************************/
/*  FUNCTION:  u_multiset_tree_predecessor  */
/**/
/*    INPUTS:  tree is the tree in question, and x is the node we want the */
/*             the predecessor of. */
/**/
/*    OUTPUT:  This function returns the predecessor of x or NULL if no */
/*             predecessor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/

u_multiset_node* u_multiset_tree_predecessor(u_multiset* tree, u_multiset_node* x) {
  u_multiset_node* y;
  u_multiset_node* nil=tree->nil;
  u_multiset_node* root=tree->root;

  if (nil != (y = x->left)) { /* assignment to y is intentional */
    while(y->right != nil) { /* returns the maximum of the left subtree of x */
      y=y->right;
    }
    return(y);
  } else {
    y=x->parent;
    while(x == y->left) { 
      if (y == root) return(nil); 
      x=y;
      y=y->parent;
    }
    return(y);
  }
}


/***********************************************************************/
/*  FUNCTION:  u_multiset_tree_dest_helper */
/**/
/*    INPUTS:  tree is the tree to destroy and x is the current node */
/**/
/*    OUTPUT:  none  */
/**/
/*    EFFECTS:  This function recursively destroys the nodes of the tree */
/*              postorder using the DestroyKey and DestroyInfo functions. */
/**/
/*    Modifies Input: tree, x */
/**/
/*    Note:    This function should only be called by u_multiset_destroy */
/***********************************************************************/

void u_multiset_tree_dest_helper(u_multiset* tree, u_multiset_node* x) {
  u_multiset_node* nil=tree->nil;
  if (x != nil) {
    u_multiset_tree_dest_helper(tree,x->left);
    u_multiset_tree_dest_helper(tree,x->right);
    free(x);
  }
}


/***********************************************************************/
/*  FUNCTION:  u_multiset_destroy */
/**/
/*    INPUTS:  tree is the tree to destroy */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Destroys the key and frees memory */
/**/
/*    Modifies Input: tree */
/**/
/***********************************************************************/

void u_multiset_destroy(u_multiset* tree) {
  u_multiset_tree_dest_helper(tree,tree->root->left);
  free(tree->root);
  free(tree->nil);
  free(tree);
}

/***********************************************************************/
/*  FUNCTION:  u_multiset_exact_query */
/**/
/*    INPUTS:  tree is the tree to print and q is a pointer to the key */
/*             we are searching for */
/**/
/*    OUTPUT:  returns the a node with key equal to q.  If there are */
/*             multiple nodes with key equal to q this function returns */
/*             the one highest in the tree */
/**/
/*    Modifies Input: none */
/**/
/***********************************************************************/
  
u_multiset_node* u_multiset_exact_query(u_multiset *tree, unsigned q){
  u_multiset_node* x=tree->root->left;
  u_multiset_node* nil=tree->nil;
  if (x == nil) return(0);
  while(x->key != q) {/*assignemnt*/
    if (x->key > q) { /* x->key > q */
      x=x->left;
    } else {
      x=x->right;
    }
    if ( x == nil) return(0);
  }
  return(x);
}


/***********************************************************************/
/*  FUNCTION:  u_multiset_delete_fix_up */
/**/
/*    INPUTS:  tree is the tree to fix and x is the child of the spliced */
/*             out node in u_multiset_delete. */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Performs rotations and changes colors to restore red-black */
/*             properties after a node is deleted */
/**/
/*    Modifies Input: tree, x */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

void u_multiset_delete_fix_up(u_multiset* tree, u_multiset_node* x) {
  u_multiset_node* root=tree->root->left;
  u_multiset_node* w;

  while( (!x->red) && (root != x)) {
    if (x == x->parent->left) {
      w=x->parent->right;
      if (w->red) {
	w->red=0;
	x->parent->red=1;
	u_multiset_left_rotate(tree,x->parent);
	w=x->parent->right;
      }
      if ( (!w->right->red) && (!w->left->red) ) { 
	w->red=1;
	x=x->parent;
      } else {
	if (!w->right->red) {
	  w->left->red=0;
	  w->red=1;
	  u_multiset_right_rotate(tree,w);
	  w=x->parent->right;
	}
	w->red=x->parent->red;
	x->parent->red=0;
	w->right->red=0;
	u_multiset_left_rotate(tree,x->parent);
	x=root; /* this is to exit while loop */
      }
    } else { /* the code below is has left and right switched from above */
      w=x->parent->left;
      if (w->red) {
	w->red=0;
	x->parent->red=1;
	u_multiset_right_rotate(tree,x->parent);
	w=x->parent->left;
      }
      if ( (!w->right->red) && (!w->left->red) ) { 
	w->red=1;
	x=x->parent;
      } else {
	if (!w->left->red) {
	  w->right->red=0;
	  w->red=1;
	  u_multiset_left_rotate(tree,w);
	  w=x->parent->left;
	}
	w->red=x->parent->red;
	x->parent->red=0;
	w->left->red=0;
	u_multiset_right_rotate(tree,x->parent);
	x=root; /* this is to exit while loop */
      }
    }
  }
  x->red=0;

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not black in u_multiset_delete_fix_up");
#endif
}


/***********************************************************************/
/*  FUNCTION:  u_multiset_delete */
/**/
/*    INPUTS:  tree is the tree to delete node z from */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Deletes z from tree and frees the key and info of z */
/*             using DestoryKey and DestoryInfo.  Then calls */
/*             u_multiset_delete_fix_up to restore red-black properties */
/**/
/*    Modifies Input: tree, z */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

void u_multiset_delete_node(u_multiset* tree, u_multiset_node* z){
  u_multiset_node* y;
  u_multiset_node* x;
  u_multiset_node* nil=tree->nil;
  u_multiset_node* root=tree->root;

  y= ((z->left == nil) || (z->right == nil)) ? z : u_multiset_tree_successor(tree,z);
  x= (y->left == nil) ? y->right : y->left;
  if (root == (x->parent = y->parent)) { /* assignment of y->p to x->p is intentional */
    root->left=x;
  } else {
    if (y == y->parent->left) {
      y->parent->left=x;
    } else {
      y->parent->right=x;
    }
  }
  if (y != z) { /* y should not be nil in this case */

#ifdef DEBUG_ASSERT
    Assert( (y!=tree->nil),"y is nil in u_multiset_delete\n");
#endif
    /* y is the node to splice out and x is its child */

    if (!(y->red)) u_multiset_delete_fix_up(tree,x);
  
    y->left=z->left;
    y->right=z->right;
    y->parent=z->parent;
    y->red=z->red;
    z->left->parent=z->right->parent=y;
    if (z == z->parent->left) {
      z->parent->left=y; 
    } else {
      z->parent->right=y;
    }
    free(z); 
  } else {
    if (!(y->red)) u_multiset_delete_fix_up(tree,x);
    free(y);
  }
  
#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not black in u_multiset_delete");
#endif
}


/***********************************************************************/
/*  FUNCTION:  UMultisetDEnumerate */
/**/
/*    INPUTS:  tree is the tree to look for keys >= low */
/*             and <= high with respect to the Compare function */
/**/
/*    OUTPUT:  stack containing pointers to the nodes between [low,high] */
/**/
/*    Modifies Input: none */
/***********************************************************************/

stk_stack* u_multiset_enumerate(u_multiset* tree, unsigned low, unsigned high) {
  stk_stack* enum_result_stack;
  u_multiset_node* nil=tree->nil;
  u_multiset_node* x=tree->root->left;
  u_multiset_node* last_best=nil;

  enum_result_stack=stack_create();
  while(nil != x) {
    if (x->key > high ) { /* x->key > high */
      x=x->left;
    } else {
      last_best=x;
      x=x->right;
    }
  }
  while ( (last_best != nil) && (low <= last_best->key)) {
    stack_push(enum_result_stack,last_best);
    last_best=u_multiset_tree_predecessor(tree,last_best);
  }
  return(enum_result_stack);
}
      
void u_multiset_clear(u_multiset* tree){
	u_multiset_tree_dest_helper(tree,tree->root->left);
	tree->root->left = tree->nil;
	tree->size = 0;
}


int u_multiset_empty(u_multiset* tree){ 
	return tree->size == 0;
}
  
void u_multiset_delete(u_multiset* tree, unsigned key){
	u_multiset_node *item = u_multiset_exact_query(tree, key);
	if(item == NULL) { return; }
	if(item->multiplicity > 1){
		item->multiplicity--;
	}
	else{
		u_multiset_delete_node(tree, item);
	}
	tree->size--;
}

void u_multiset_delete_equal(u_multiset* tree, unsigned key){
	u_multiset_node *item = u_multiset_exact_query(tree, key);
	if(item == NULL) { return; }
	tree->size -= item->multiplicity;
	u_multiset_delete_node(tree, item);	
}

unsigned u_multiset_max_helper(u_multiset* tree, u_multiset_node *node, unsigned current_max){
	if(node == tree->nil){
		return 0;
	}
	else{
		unsigned a = u_multiset_max_helper(tree, node->left, current_max);
		unsigned b = u_multiset_max_helper(tree, node->right, current_max);
		if(node->key > a){
			if(node->key > b){
				return node->key;
			}
			else{
				return b; 
			}
		}
		else{
			if(a > b){
				return a;
			}
			else{
				return b;
			}
		}
	}
	
}

unsigned u_multiset_max(u_multiset* tree){
	return u_multiset_max_helper(tree, tree->root, 0);
}

unsigned u_multiset_min_helper(u_multiset* tree, u_multiset_node *node, unsigned current_min){
	if(node == tree->nil){
		return 0;
	}
	else{
		unsigned a = u_multiset_min_helper(tree, node->left, current_min);
		unsigned b = u_multiset_min_helper(tree, node->right, current_min);
		if(node->key < a){
			if(node->key < b){
				return node->key;
			}
			else{
				return b; 
			}
		}
		else{
			if(a < b){
				return a;
			}
			else{
				return b;
			}
		}
	}
	
}

unsigned u_multiset_min(u_multiset* tree){
	return u_multiset_min_helper(tree, tree->root, 0);
}

int u_multiset_size(u_multiset* tree){
	return tree->size;
}

void u_multiset_create_array_helper(u_multiset* tree, u_multiset_node *node, unsigned *set_array, int *index){
	if(*index < tree->size && node != tree->nil) {
		for(int count = 0; count < node->multiplicity; ++count, ++(*index)){
			set_array[*index] = node->key;
		}
		u_multiset_create_array_helper(tree, node->left, set_array, index);	
		u_multiset_create_array_helper(tree, node->right, set_array, index);
	}	
}

unsigned * u_multiset_create_array(u_multiset* tree){
	unsigned *set_array = (unsigned *)safe_malloc(sizeof(unsigned) * tree->size);
	int index = 0;	
	
	u_multiset_create_array_helper(tree, tree->root->left, set_array, &index);

	return set_array;
}

