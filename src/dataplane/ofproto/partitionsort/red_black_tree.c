#include "red_black_tree.h"

int compare_box(const box a, const box b);
int compare_query(const box a, const packet q, int level, const int *field_order);
void rb_left_rotate(rb_red_blk_tree* tree, rb_red_blk_node* x);
void rb_right_rotate(rb_red_blk_tree* tree, rb_red_blk_node* x);
bool rb_is_intersect(unsigned a1, unsigned b1, unsigned a2, unsigned b2);
bool rb_is_identical(unsigned a1, unsigned b1, unsigned a2, unsigned b2);
box_vector* rb_prepend_chainbox_a(box_vector *cb, int n_prepend);
box_vector* rb_prepend_chainbox_b(box_vector *cb, int n_prepend);
void rb_in_order_tree_print(rb_red_blk_tree* tree, rb_red_blk_node* x);
void rb_tree_dest_helper(rb_red_blk_tree* tree, rb_red_blk_node* x);
void rb_delete_fix_up(rb_red_blk_tree* tree, rb_red_blk_node* x);

int compare_box(const box a, const box b) {
	int compare_size = BOX_SIZE;
	const int LOW = 0, HIGH = 1;
	if (a[LOW] == b[LOW] && a[HIGH] == b[HIGH])  return 0;
	if (rb_overlap(a[LOW], a[HIGH], b[LOW], b[HIGH])) return 2;
	if (a[HIGH] < b[LOW]) {
		return -1;
	} else if (a[LOW] > b[HIGH]) {
		return 1;
	}
	
	return 0;
}

int compare_query(const box a, const packet q, int level, const int *field_order) {

	const int LOW = 0, HIGH = 1;
	if (a[HIGH] < q[field_order[level]]) {
		return -1;
	} else if (a[LOW] > q[field_order[level]]) {
		return 1;
	}
	
	return 0;
}

/***********************************************************************/
/*  FUNCTION:  rb_tree_create */
/**/
/*  INPUTS:  All the inputs are names of functions.  CompFunc takes to */
/*  void pointers to keys and returns 1 if the first arguement is */
/*  "greater than" the second.   DestFunc takes a pointer to a key and */
/*  destroys it in the appropriate manner when the node containing that */
/*  key is deleted.  InfoDestFunc is similiar to DestFunc except it */
/*  recieves a pointer to the info of a node and destroys it. */
/*  PrintFunc recieves a pointer to the key of a node and prints it. */
/*  PrintInfo recieves a pointer to the info of a node and prints it. */
/*  If rb_tree_print is never called the print functions don't have to be */
/*  defined and NullFunction can be used.  */
/**/
/*  OUTPUT:  This function returns a pointer to the newly created */
/*  red-black tree. */
/**/
/*  Modifies Input: none */
/***********************************************************************/

rb_red_blk_tree* rb_tree_create() {

  rb_red_blk_tree* new_tree;
  rb_red_blk_node* temp;

  new_tree = (rb_red_blk_tree*) safe_malloc(sizeof(rb_red_blk_tree));
  new_tree->count = 0;
  new_tree->max_priority_rule = NULL;
  new_tree->chain_boxes = box_vector_init();
  new_tree->rule_list = rule_vector_init();
  /*  see the comment in the rb_red_blk_tree structure in red_black_tree.h */
  /*  for information on nil and root */
  temp=new_tree->nil= rb_node_create();
  temp->parent=temp->left=temp->right=temp;
  temp->red=0;
  temp->key[0] = 1111;
  temp->key[1] = 1111;
  temp=new_tree->root= rb_node_create();
  temp->parent=temp->left=temp->right=new_tree->nil;
  temp->key[0] = 2222;
  temp->key[1] = 2222;
  temp->red=0; 

  return(new_tree);
}

/***********************************************************************/
/*  FUNCTION:  rb_left_rotate */
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

void rb_left_rotate(rb_red_blk_tree* tree, rb_red_blk_node* x) {
  rb_red_blk_node* y;
  rb_red_blk_node* nil=tree->nil;

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls rb_left_rotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when RBDeleteFixUP */
  /*  calls rb_left_rotate it expects the parent pointer of nil to be */
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
  Assert(!tree->nil->red,"nil not red in rb_left_rotate");
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

void rb_right_rotate(rb_red_blk_tree* tree, rb_red_blk_node* y) {
  rb_red_blk_node* x;
  rb_red_blk_node* nil=tree->nil;

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls rb_left_rotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when RBDeleteFixUP */
  /*  calls rb_left_rotate it expects the parent pointer of nil to be */
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
  Assert(!tree->nil->red,"nil not red in rb_right_rotate");
#endif
}
bool rb_is_intersect(unsigned a1, unsigned b1, unsigned a2, unsigned b2) {
	return (a1 > a2 ? a1 : a2) <= (b1 < b2 ? b1 : b2);
}
bool rb_is_identical(unsigned a1, unsigned b1, unsigned a2, unsigned b2) {
	return a1 == a2  && b1 == b2;
}

bool rb_tree_can_insert(rb_red_blk_tree *tree, const box *z, const int NUM_BOXES, int level, const int *field_order, const int NUM_FIELDS) {
	 
	if (tree == NULL) return true;
	
	if (level == NUM_FIELDS) {
		return true; 
	} else if (tree->count == 1) { 
		box *chain_boxes = box_vector_to_array(tree->chain_boxes);
		for (size_t i = level; i < NUM_FIELDS; ++i) {
			if (rb_is_identical(z[field_order[i]][0], z[field_order[i]][1], chain_boxes[i - level][0], chain_boxes[i - level][1])) continue;
			if (rb_is_intersect(z[field_order[i]][0], z[field_order[i]][1], chain_boxes[i - level][0], chain_boxes[i - level][1])) return false;
			else return true;
		}
		return true;
	}

	rb_red_blk_node* x;
	rb_red_blk_node* y;
	rb_red_blk_node* nil = tree->nil;

	y = tree->root;
	x = tree->root->left;
	while (x != nil) {
		y = x;
		int compare_result = compare_box(x->key, z[field_order[level]]);
		if (compare_result == 1) { /* x.key > z.key */
			x = x->left;
		} else if (compare_result == -1) { /* x.key < z.key */
			x = x->right;
		} else if (compare_result == 0) {  /* x.key = z.key */
			/*printf("rb_tree_insert_help:: Exact Match!\n");
			return true;
			x = x->right;*/
			return level == NUM_BOXES - 1 ? true : rb_tree_can_insert(x->rb_tree_next_level, z, NUM_BOXES, level + 1, field_order, NUM_FIELDS);
		} else {  /* x.key || z.key */
			return false;
		}
	}
	return true;
}


/***********************************************************************/
/*  FUNCTION:  rb_tree_insert_help  */
/**/
/*  INPUTS:  tree is the tree to insert into and z is the node to insert */
/**/
/*  OUTPUT:  none */
/**/
/*  Modifies Input:  tree, z */
/**/
/*  EFFECTS:  Inserts z into the tree as if it were a regular binary tree */
/*            using the algorithm described in _Introduction_To_Algorithms_ */
/*            by Cormen et al.  This funciton is only intended to be called */
/*            by the rb_tree_insert function and not by the user */
/***********************************************************************/

bool rb_tree_insert_with_path_compression_help(rb_red_blk_tree* tree, rb_red_blk_node* z, const box *b, const int NUM_BOXES, int level, const int *field_order, const int NUM_FIELDS, rule * r, rb_red_blk_node** out_ptr) {
  /*  This function should only be called by InsertRBTree (see above) */
  rb_red_blk_node* x;
  rb_red_blk_node* y;
  rb_red_blk_node* nil=tree->nil;
  
  z->left=z->right=nil;
  y=tree->root;
  x=tree->root->left;
  while( x != nil) {
    y=x;
	int compare_result =  compare_box(x->key, z->key);
	if (compare_result == 1) { /* x.key > z.key */
      x=x->left;
    } else if (compare_result ==-1) { /* x.key < z.key */
      x=x->right;
	} else if (compare_result == 0) {  /* x.key = z.key */
		
		if (level != NUM_FIELDS - 1) {
			/*if (x->rb_tree_next_level->count == 1) {
				//path compression

				auto temp_chain_boxes = x->rb_tree_next_level->chain_boxes;
				int x_priority = x->rb_tree_next_level->max_priority_rule;

				free(x->rb_tree_next_level);
				//unzipping the next level
				int run = 1;
		
				std::vector<int> natural_field_order(NUM_FIELDS);
				std::iota(begin(natural_field_order), end(natural_field_order), 0);
				while ( (temp_chain_boxes[run][0] == b[field_order[level + run]][0] && temp_chain_boxes[run][1] == b[field_order[level + run]][1])) {
					x->rb_tree_next_level = rb_tree_create();
					x->rb_tree_next_level->count = 1;
					x = rb_tree_insert(x->rb_tree_next_level, temp_chain_boxes, level + run, natural_field_order, priority);
					run++;
					if (level + run >= NUM_FIELDS) break;
				}

				if (level + run  >= NUM_FIELDS) {
					x->rb_tree_next_level = rb_tree_create();
					x->rb_tree_next_level->pq.push(priority);
					x->rb_tree_next_level->pq.push(x_priority);
					out_ptr = x;
				}
				else if (!(temp_chain_boxes[run][0] == b[field_order[level + run]][0] && temp_chain_boxes[run][1] == b[field_order[level + run]][1])) {
					//split into z and x node
					x->rb_tree_next_level = rb_tree_create();
					rb_tree_insert(x->rb_tree_next_level, temp_chain_boxes, level + run, natural_field_order, x_priority);
					rb_tree_insert(x->rb_tree_next_level, b, level + run, field_order, priority);
				}
			
			}
			else {*/
			rb_tree_insert_with_path_compression(x->rb_tree_next_level, b, NUM_BOXES, level + 1, field_order, NUM_FIELDS, r);
			//}
		
		}
		else {
			if ( x->rb_tree_next_level == NULL)
				x->rb_tree_next_level = rb_tree_create();
			x->rb_tree_next_level->count++;
			rb_push_rule(x->rb_tree_next_level, r);
			*out_ptr = x;
		}
		rb_node_destroy(z);
		return true;
	} else {  /* x.key || z.key */
		printf("Warning TreeInsertPathcompressionHelp : x.key || z.key\n");
	}
  }
  z->parent=y;
  if ( (y == tree->root) ||
	  (1 ==  compare_box(y->key, z->key))) { /* y.key > z.key */
    y->left=z;
  } else {
    y->right=z;
  }
  //found new one to insert to
  //need to create a tree first then followed by insertion
  //we use path-compression here since this tree contains a single rule
  z->rb_tree_next_level = rb_tree_create(); 
  rb_tree_insert_with_path_compression(z->rb_tree_next_level, b, NUM_BOXES, level  +1, field_order, NUM_FIELDS, r);

  

  return false;

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in rb_tree_insert_help");
#endif
}


/*  Before calling Insert RBTree the node x should have its key set */

/***********************************************************************/
/*  FUNCTION:  rb_tree_insert */
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

//MAY HAVE CHANGED LOGIC AND ADDED MEMORY LEAK TO THIS FUNCTION

box_vector* rb_prepend_chainbox_a(box_vector *cb, int n_prepend) {
  box_vector *t = box_vector_init();
  point temp[BOX_SIZE] = {999, 100020};
  for (int i = 0; i < n_prepend; ++i) {
	box_vector_push_back(t, temp);
  }
  box_vector_append(t, cb);
  return t;
};

box_vector* rb_prepend_chainbox_b(box_vector *cb, int n_prepend) {
  box_vector *t = box_vector_init();
  point temp[BOX_SIZE] = {0, 10000000};
  for (int i = 0; i < n_prepend; ++i) {
	box_vector_push_back(t, temp);
  }
  box_vector_append(t, cb);
  return t;
};

rb_red_blk_node * rb_tree_insert_with_path_compression(rb_red_blk_tree* tree, const box *key, const int NUM_BOXES, unsigned int level, const int *field_order, const int NUM_FIELDS, rule *r) {

	
  rb_red_blk_node * y;
  rb_red_blk_node * x;
  rb_red_blk_node * new_node;
 
  if (level == NUM_FIELDS) {  
	  tree->count++;
	  rb_push_rule(tree, r);   
	  return NULL;
  } 

  if (tree->count == 0) { 
	  //no need to create a node yet; just compress the path
	  tree->count++;
	  rb_push_rule(tree, r); 
	  //level <= b.size() -1 
	  for (size_t i = level; i < NUM_FIELDS; ++i)  
		 box_vector_push_back(tree->chain_boxes, key[field_order[i]]);
	  return NULL;
  }
  if (tree->count == 1) {
	  //path compression
	  box_vector *temp_chain_boxes_v = box_vector_initv(tree->chain_boxes); //SUPPOSED TO BE A MOVE
	  box *temp_chain_boxes = box_vector_to_array(temp_chain_boxes_v);
	  rule *x_rule =  tree->max_priority_rule;
	
	//  tree->pq = std::priority_queue<int>();
	  tree->count++;
	//  tree->rb_push_rule(priority);
	 
	 /* bool is_identical = 1;
	  for (int i = level; i < NUM_FIELDS; i++) {
		  if (tree->chain_boxes[i - level] != key[field_order[i]]){
			  is_identical = 0;
			  break;
		  }
	  }
	  if (is_identical) {
		  //TODO:: move tree->pq.push(priority); to this line
		  tree->count = 1;
		  return NULL;
	  }*/
	  //quick check if identical just reset count = 1;
	  
	  box_vector_clear(tree->chain_boxes);

	  //unzipping the next level 

	  int *natural_field_order = safe_malloc(NUM_FIELDS * sizeof(int));
	  for(int i = 0; i < NUM_FIELDS; ++i){
	  	natural_field_order[i] = i;
	  }	 
 
	  size_t run = 0; 
	  if (temp_chain_boxes[run][0] == key[field_order[level + run]][0] && temp_chain_boxes[run][1] == key[field_order[level + run]][1]) {
		//  printf("[%u %u] vs. [%u %u]\n", temp_chain_boxes[run][0], temp_chain_boxes[run][1], key[field_order[level + run]][0], key[field_order[level + run]][1]);
		  x = rb_tree_insert(tree, key, NUM_BOXES, level + run++, field_order, x_rule);
		  if (level + run < NUM_FIELDS) {
			  while ((temp_chain_boxes[run][0] == key[field_order[level + run]][0] && temp_chain_boxes[run][1] == key[field_order[level + run]][1])) {

				  x->rb_tree_next_level = rb_tree_create();

				  //  x->rb_tree_next_level->rb_push_rule(x_priority);
				  // x->rb_tree_next_level->rb_push_rule(priority);
				  x->rb_tree_next_level->count = 2;
				  x = rb_tree_insert(x->rb_tree_next_level, key, NUM_BOXES, level + run, field_order, r);


				  run++;


				  if (level + run >= NUM_FIELDS) break;
			  }
		  }
		  if (level + run >= NUM_FIELDS) {
			  x->rb_tree_next_level = rb_tree_create(); 
			  x->rb_tree_next_level->count++;
			  rb_push_rule(x->rb_tree_next_level, r);
			  x->rb_tree_next_level->count++;
			  rb_push_rule(x->rb_tree_next_level, x_rule);
		  } else if (!(temp_chain_boxes[run][0] == key[field_order[level + run]][0] && temp_chain_boxes[run][1] == key[field_order[level + run]][1])) {
			  if (rb_is_intersect(temp_chain_boxes[run][0], temp_chain_boxes[run][1], key[field_order[level + run]][0], key[field_order[level + run]][1])) {
				  printf("Warning not intersect?\n");
				  printf("[%u %u] vs. [%u %u]\n", temp_chain_boxes[run][low_dim], temp_chain_boxes[run][high_dim], key[field_order[level + run]][low_dim], key[field_order[level + run]][high_dim]);
				  printf("chain_boxes:\n");
				  //for (auto e : temp_chain_boxes)
				  //  printf("[%u %u] ", e[low_dim], e[high_dim]);
				  printf("\n boxes:\n");
				  for (size_t i = 0; i < NUM_BOXES; ++i) {
					  printf("[%u %u] ", key[field_order[i]][low_dim], key[field_order[i]][high_dim]);
				  }
				  printf("\n");
				  exit(0);
			  }
			  //split into z and x node
			  x->rb_tree_next_level = rb_tree_create(); 
			  //x->rb_tree_next_level->rb_push_rule(priority);
			  //x->rb_tree_next_level->rb_push_rule(x_priority);
			  //auto PrependChainbox = [](std::vector<box>& cb, int n_prepend) {
			  //	  std::vector<box> t;
			  //	  for (int i = 0; i < n_prepend; i++) t.push_back({ { 999, 100020 } });
			  //	  t.insert(end(t), begin(cb), end(cb));
			  //	  return t;
			  //};
			  box_vector *key_p = rb_prepend_chainbox_a(temp_chain_boxes_v, level);
			  rb_red_blk_node * z1 = rb_tree_insert(x->rb_tree_next_level, box_vector_to_array(key_p), box_vector_size(key_p), level + run, natural_field_order, x_rule); 
			  rb_red_blk_node * z2 = rb_tree_insert(x->rb_tree_next_level, key, NUM_BOXES, level + run, field_order, r); 

			  x->rb_tree_next_level->count = 2;

			  z1->rb_tree_next_level = rb_tree_create(); 
			  z2->rb_tree_next_level = rb_tree_create();

			  rb_tree_insert_with_path_compression(z1->rb_tree_next_level, box_vector_to_array(key_p), box_vector_size(key_p), level + run + 1, natural_field_order, NUM_FIELDS, x_rule);
			  rb_tree_insert_with_path_compression(z2->rb_tree_next_level, key, NUM_BOXES, level + run + 1, field_order, NUM_FIELDS, r);
			  box_vector_free(key_p);//THIS MAY BREAK THE CODE

		  }
	  } else { 
		  //auto PrependChainbox = [](std::vector<box>& cb, int n_prepend) {
		  //	  std::vector<box> t;
		  //	  for (int i = 0; i < n_prepend; i++) t.push_back({ { 0, 10000000 } });
		  //	  t.insert(end(t), begin(cb), end(cb));
		  //	  return t;
		  //};

		  box_vector *key_p = rb_prepend_chainbox_b(temp_chain_boxes_v, level);
 
		  box *temp_box_array = box_vector_to_array(key_p);
		  int bsize = box_vector_size(key_p);

		  rb_red_blk_node * z1 = rb_tree_insert(tree, box_vector_to_array(key_p), box_vector_size(key_p), level + run, natural_field_order, x_rule);
		  rb_red_blk_node * z2 = rb_tree_insert(tree, key, NUM_BOXES, level + run, field_order, r);

		  z1->rb_tree_next_level = rb_tree_create(); 
		  z2->rb_tree_next_level = rb_tree_create();

		  rb_tree_insert_with_path_compression(z1->rb_tree_next_level, box_vector_to_array(key_p), box_vector_size(key_p), level + run + 1, natural_field_order, NUM_FIELDS, x_rule);
		  rb_tree_insert_with_path_compression(z2->rb_tree_next_level, key, NUM_BOXES, level + run + 1, field_order, NUM_FIELDS, r);

		  box_vector_free(key_p);

	  }
	  box_vector_free(temp_chain_boxes_v);
	  free(natural_field_order);
	  return NULL;
  } 


  tree->count++;
 /* tree->rb_push_rule(priority);
  int maxpri = tree->max_priority_rule;
  tree->rule_list.clear();
  tree->rb_push_rule(maxpri);*/


  x = rb_node_create();
  x->key[0] = key[field_order[level]][0];
  x->key[1] = key[field_order[level]][1];
  rb_red_blk_node * out_ptr;

  if (rb_tree_insert_with_path_compression_help(tree, x, key, NUM_BOXES, level, field_order, NUM_FIELDS, r, &out_ptr)){
	  
	  //insertion finds identical box.
	  //do nothing for now
	  return out_ptr;
  }

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
	  rb_left_rotate(tree,x);
	}
	x->parent->red=0;
	x->parent->parent->red=1;
	rb_right_rotate(tree,x->parent->parent);
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
	  rb_right_rotate(tree,x);
	}
	x->parent->red=0;
	x->parent->parent->red=1;
	rb_left_rotate(tree,x->parent->parent);
      } 
    }
  }
  tree->root->left->red=0;
  return(new_node);

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in rb_tree_insert");
  Assert(!tree->root->red,"root not red in rb_tree_insert");
#endif
}

/*  Before calling Insert RBTree the node x should have its key set */

/***********************************************************************/
/*  FUNCTION:  rb_tree_insert */
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



rb_red_blk_node * rb_tree_insert(rb_red_blk_tree* tree, const box *key, const int KEY_SIZE, int level, const int *field_order, rule *r) {

	if (level == KEY_SIZE) return NULL;
 
	rb_red_blk_node * y;
	rb_red_blk_node * x;
	rb_red_blk_node * new_node;

	x = rb_node_create();
	x->key[0] = key[field_order[level]][0];//THIS MAY CAUSE SEGFAULT
	x->key[1] = key[field_order[level]][1];//THIS MAY CAUSE SEGFAULT
	rb_red_blk_node * out_ptr = NULL;
	if (rb_tree_insert_help(tree, x, key, KEY_SIZE, level, field_order, r, out_ptr)){
		//insertion finds identical box.
		//do nothing for now
		return out_ptr;
	}

	new_node = x;
	x->red = 1;
	while (x->parent->red) { /* use sentinel instead of checking for root */
		if (x->parent == x->parent->parent->left) {
			y = x->parent->parent->right;
			if (y->red) {
				x->parent->red = 0;
				y->red = 0;
				x->parent->parent->red = 1;
				x = x->parent->parent;
			} else {
				if (x == x->parent->right) {
					x = x->parent;
					rb_left_rotate(tree, x);
				}
				x->parent->red = 0;
				x->parent->parent->red = 1;
				rb_right_rotate(tree, x->parent->parent);
			}
		} else { /* case for x->parent == x->parent->parent->right */
			y = x->parent->parent->left;
			if (y->red) {
				x->parent->red = 0;
				y->red = 0;
				x->parent->parent->red = 1;
				x = x->parent->parent;
			} else {
				if (x == x->parent->left) {
					x = x->parent;
					rb_right_rotate(tree, x);
				}
				x->parent->red = 0;
				x->parent->parent->red = 1;
				rb_left_rotate(tree, x->parent->parent);
			}
		}
	}
	tree->root->left->red = 0;
	//printf("Done: [%u %u]\n", new_node->key);
	return(new_node);

#ifdef DEBUG_ASSERT
	Assert(!tree->nil->red, "nil not red in rb_tree_insert");
	Assert(!tree->root->red, "root not red in rb_tree_insert");
#endif
}

bool rb_tree_insert_help(rb_red_blk_tree* tree, rb_red_blk_node* z, const box *b, const int B_SIZE, int level, const int *field_order, rule *r, rb_red_blk_node* out_ptr) {
	/*  This function should only be called by InsertRBTree  */
	rb_red_blk_node* x;
	rb_red_blk_node* y;
	rb_red_blk_node* nil = tree->nil;

	z->left = z->right = nil;
	y = tree->root;
	x = tree->root->left;
	while (x != nil) {
		y = x;
		int compare_result = compare_box(x->key, z->key);
		if (compare_result == 1) { /* x.key > z.key */
			x = x->left;
		} else if (compare_result == -1) { /* x.key < z.key */
			x = x->right;
		} else if (compare_result == 0) {  /* x.key = z.key */
			printf("Warning compare_result == 0??\n");
			if (level != B_SIZE - 1) {
				rb_tree_insert(x->rb_tree_next_level, b, B_SIZE, level + 1, field_order, r);
			} else {
			//	x->node_max_priority = std::max(x->node_max_priority, priority);
				//x->nodes_priority[x->num_node_priority++] = priority;
				out_ptr = x;
			}
			rb_node_destroy(z);
			return true;
		} else {  /* x.key || z.key */
			printf("x:[%u %u], z:[%u %u]\n", x->key[low_dim], x->key[high_dim], z->key[low_dim], z->key[high_dim]);
			printf("Warning rb_tree_insert_help : x.key || z.key\n");
		}
	}
	z->parent = y;
	if ((y == tree->root) ||
		(1 == compare_box(y->key, z->key))) { /* y.key > z.key */
		y->left = z;
	} else {
		y->right = z;
	}
	//found new one to insert to but will not propagate
	out_ptr = z;

	return false;

#ifdef DEBUG_ASSERT
	Assert(!tree->nil->red, "nil not red in rb_tree_insert_help");
#endif
}


/***********************************************************************/
/*  FUNCTION:  rb_tree_successor  */
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
  
rb_red_blk_node* rb_tree_successor(rb_red_blk_tree* tree,rb_red_blk_node* x) { 
  rb_red_blk_node* y;
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* root=tree->root;

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
/*  FUNCTION:  rb_tree_predecessor  */
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

rb_red_blk_node* rb_tree_predecessor(rb_red_blk_tree* tree, rb_red_blk_node* x) {
  rb_red_blk_node* y;
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* root=tree->root;

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
/*  FUNCTION:  rb_in_order_tree_print */
/**/
/*    INPUTS:  tree is the tree to print and x is the current inorder node */
/**/
/*    OUTPUT:  none  */
/**/
/*    EFFECTS:  This function recursively prints the nodes of the tree */
/*              inorder using the rb_print_key and PrintInfo functions. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:    This function should only be called from rb_tree_print */
/***********************************************************************/

void rb_in_order_tree_print(rb_red_blk_tree* tree, rb_red_blk_node* x) {
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* root=tree->root;
  if (x != tree->nil) {
    rb_in_order_tree_print(tree,x->left);
    //printf("  key="); 
  rb_print_key(x->key);
  //  printf("  l->key=");
   // if( x->left != nil) tree->rb_print_key(x->left->key);
  //  printf("  r->key=");
  //  if( x->right != nil)  tree->rb_print_key(x->right->key);
   // printf("  p->key=");
   // if( x->parent != root) /*printf("NULL"); else*/ tree->rb_print_key(x->parent->key);
   // printf("  red=%i\n",x->red);
    rb_in_order_tree_print(tree,x->right);
  }
}


/***********************************************************************/
/*  FUNCTION:  rb_tree_dest_helper */
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
/*    Note:    This function should only be called by rb_tree_destroy */
/***********************************************************************/

void rb_tree_dest_helper(rb_red_blk_tree* tree, rb_red_blk_node* x) {
  rb_red_blk_node* nil=tree->nil;
  if (x != nil) {
	  if (x->rb_tree_next_level != NULL)
		rb_tree_destroy(x->rb_tree_next_level);
    rb_tree_dest_helper(tree,x->left);
    rb_tree_dest_helper(tree,x->right);
    rb_node_destroy(x);
  }
}


/***********************************************************************/
/*  FUNCTION:  rb_tree_destroy */
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

void rb_tree_destroy(rb_red_blk_tree* tree) {
  rb_tree_dest_helper(tree,tree->root->left);
  rb_node_destroy(tree->root);
  rb_node_destroy(tree->nil);
  box_vector_free(tree->chain_boxes);
  rule_vector_free(tree->rule_list);
  free(tree); 
}


/***********************************************************************/
/*  FUNCTION:  rb_tree_print */
/**/
/*    INPUTS:  tree is the tree to print */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  This function recursively prints the nodes of the tree */
/*             inorder using the rb_print_key and PrintInfo functions. */
/**/
/*    Modifies Input: none */
/**/
/***********************************************************************/

void rb_tree_print(rb_red_blk_tree* tree) {
  printf("tree->count = %d\n", tree->count);
  rb_in_order_tree_print(tree,tree->root->left);
}


/***********************************************************************/
/*  FUNCTION:  rb_exact_query */
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

rule * rb_exact_query( rb_red_blk_tree*  tree, const packet q,int level, const int *field_order, const int NUM_FIELDS) {

	//printf("entering level %d - tree->GetMaxPriority =%d\n", level,tree->max_priority_rule);

  //check if singleton
	if (level == NUM_FIELDS) {
		return tree->max_priority_rule;
	}
  else if (tree->count == 1) {  
	//  auto chain_boxes = tree->chain_boxes;
	  box *ch_box_a = box_vector_to_array(tree->chain_boxes); 
	  for (size_t i = level; i < NUM_FIELDS; ++i) { 
		  if (q[field_order[i]] < ch_box_a[i - level][0]) return NULL;
		  if (q[field_order[i]] > ch_box_a[i - level][1]) return NULL;
	  }
	  return tree->max_priority_rule;
  }

  rb_red_blk_node* x = tree->root->left;
  rb_red_blk_node* nil = tree->nil;
  int comp_val;
  if (x == nil) return NULL;
  comp_val= compare_query(x->key,  q, level, field_order);
 // printf("Compval = %d\n", comp_val);
  while(0 != comp_val) {/*assignemnt*/
    if (1 == comp_val) { /* x->key > q */
      x=x->left;
    } 
	else if (-1 == comp_val) /*x->key < q */{
      x=x->right;
    }
	if (x == nil) return NULL;
	comp_val = compare_query(x->key, q, level, field_order);
  }

 // printf("level = %d, priority = %d\n", level, x->mid_ptr.node_max_priority);
  return rb_exact_query(x->rb_tree_next_level,q,level+1,field_order, NUM_FIELDS);
}

rule * rb_exact_query_iterative(rb_red_blk_tree*  tree, const packet q, const int *field_order, const int NUM_FIELDS) {
	 
	int level = 0;
	int comp_val;
	while (true) { 
		//check if singleton 
		if (level == NUM_FIELDS) {
			return tree->max_priority_rule;
		} else if (tree->count == 1) {
			//  auto chain_boxes = tree->chain_boxes;
			box *ch_box_a = box_vector_to_array(tree->chain_boxes);
			for (size_t i = level; i < NUM_FIELDS; ++i) {
				if (q[field_order[i]] < ch_box_a[i - level][0]) return NULL;
				if (q[field_order[i]] > ch_box_a[i - level][1]) return NULL;
			}
			return tree->max_priority_rule;
		}

 		rb_red_blk_node* x = tree->root->left;
		rb_red_blk_node* nil = tree->nil;
		
		if (x == nil) return NULL;
		comp_val = compare_query(x->key, q, level, field_order);
		// printf("Compval = %d\n", comp_val);
		while (0 != comp_val) {/*assignemnt*/
			if (1 == comp_val) { /* x->key > q */
				x = x->left;
			} else if (-1 == comp_val) /*x->key < q */{
				x = x->right;
			}
			if (x == nil) return NULL;
			comp_val = compare_query(x->key, q, level, field_order);
		}
		tree = x->rb_tree_next_level;
		level++;
	}

	// printf("level = %d, priority = %d\n", level, x->mid_ptr.node_max_priority);
	//return rb_exact_query(x->rb_tree_next_level, q, level + 1, field_order);
}

rule * rb_exact_query_priority(rb_red_blk_tree*  tree, const packet q, int level, const int *field_order, const int NUM_FIELDS, rule *best_rule_so_far) {

	//printf("entering level %d - tree->GetMaxPriority =%d\n", level,tree->max_priority_rule);
	if (best_rule_so_far->master->priority > tree->max_priority_rule->master->priority) return NULL;
	//check if singleton
	if (level == NUM_FIELDS) {
		return tree->max_priority_rule;
	} else if (tree->count == 1) {
		//  auto chain_boxes = tree->chain_boxes; 
		box *ch_box_a = box_vector_to_array(tree->chain_boxes); 
		for (size_t i = level; i < NUM_FIELDS; ++i) {
			if (q[field_order[i]] < ch_box_a[i - level][0]) return NULL;
			if (q[field_order[i]] > ch_box_a[i - level][1]) return NULL;
		}
		return tree->max_priority_rule;
	}

	rb_red_blk_node* x = tree->root->left;
	rb_red_blk_node* nil = tree->nil;
	int comp_val;
	if (x == nil) return NULL;
	comp_val = compare_query(x->key, q, level, field_order);
	// printf("Compval = %d\n", comp_val);
	while (0 != comp_val) {/*assignemnt*/
		if (1 == comp_val) { /* x->key > q */
			x = x->left;
		} else if (-1 == comp_val) /*x->key < q */{
			x = x->right;
		}
		if (x == nil) return NULL;
		comp_val = compare_query(x->key, q, level, field_order);
	}

	// printf("level = %d, priority = %d\n", level, x->mid_ptr.node_max_priority);
	return rb_exact_query_priority(x->rb_tree_next_level, q, level + 1, field_order, NUM_FIELDS, best_rule_so_far);
}

/***********************************************************************/
/*  FUNCTION:  rb_delete_fix_up */
/**/
/*    INPUTS:  tree is the tree to fix and x is the child of the spliced */
/*             out node in RBTreeDelete. */
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

void rb_delete_fix_up(rb_red_blk_tree* tree, rb_red_blk_node* x) {
  rb_red_blk_node* root=tree->root->left;
  rb_red_blk_node* w;

  while( (!x->red) && (root != x)) {
    if (x == x->parent->left) {
      w=x->parent->right;
      if (w->red) {
	w->red=0;
	x->parent->red=1;
	rb_left_rotate(tree,x->parent);
	w=x->parent->right;
      }
      if ( (!w->right->red) && (!w->left->red) ) { 
	w->red=1;
	x=x->parent;
      } else {
	if (!w->right->red) {
	  w->left->red=0;
	  w->red=1;
	  rb_right_rotate(tree,w);
	  w=x->parent->right;
	}
	w->red=x->parent->red;
	x->parent->red=0;
	w->right->red=0;
	rb_left_rotate(tree,x->parent);
	x=root; /* this is to exit while loop */
      }
    } else { /* the code below is has left and right switched from above */
      w=x->parent->left;
      if (w->red) {
	w->red=0;
	x->parent->red=1;
	rb_right_rotate(tree,x->parent);
	w=x->parent->left;
      }
      if ( (!w->right->red) && (!w->left->red) ) { 
	w->red=1;
	x=x->parent;
      } else {
	if (!w->left->red) {
	  w->right->red=0;
	  w->red=1;
	  rb_left_rotate(tree,w);
	  w=x->parent->left;
	}
	w->red=x->parent->red;
	x->parent->red=0;
	w->left->red=0;
	rb_right_rotate(tree,x->parent);
	x=root; /* this is to exit while loop */
      }
    }
  }
  x->red=0;

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not black in rb_delete_fix_up");
#endif
}

void rb_tree_delete_with_path_compression(rb_red_blk_tree** tree, const box *key, int level, const int *field_order, const int NUM_FIELDS, long long priority, bool *just_deleted_tree) {

 
	if (level == NUM_FIELDS) {
		(*tree)->count--;
		rb_pop_rule(*tree, priority); 
		if ((*tree)->count == 0) {
			rb_tree_destroy(*tree);
			*just_deleted_tree = true;
		}
		return;
	}
	if ((*tree)->count == 1) {
	

		if (level == 0) {
			(*tree)->count = 0;
			box_vector_clear((*tree)->chain_boxes);
			rb_clear_rule(*tree); 
		}
		else {
			rb_tree_destroy(*tree); 
			*just_deleted_tree = true;
		}
		return ;
	}
	if ((*tree)->count == 2) {
		int run = 0;
		rb_red_blk_tree * temp_tree = *tree;
	
		//first time tree->count ==2; this mean you need to create chain box here;
		//keep going until you find the node that contians tree->count = 1
	//	std::stack<std::pair<rb_red_blk_tree *, rb_red_blk_node *>> stack_so_far;

		while (true) {
			if (temp_tree->count == 1 || level + run == NUM_FIELDS) {
				box_vector_append((*tree)->chain_boxes, temp_tree->chain_boxes);
		
				rb_red_blk_tree* new_tree = rb_tree_create();
				box_vector_append(new_tree->chain_boxes, (*tree)->chain_boxes);

				if (rb_get_size_list(temp_tree) == 2) {
					rb_pop_rule(temp_tree, priority);
				}
				rule *new_max = temp_tree->max_priority_rule;
				rb_tree_destroy(*tree);

				*tree = new_tree;
				(*tree)->count = 1;
				rb_push_rule(*tree, new_max);
				return ;
			}
			/*if (level + run == NUM_FIELDS) {
				auto new_tree = rb_tree_create();
				new_tree->chain_boxes = tree->chain_boxes;
				rb_tree_destroy(tree);
				tree = new_tree;
				tree->count = 1;
				if (temp_tree->rb_get_size_list() == 2) {
					temp_tree->rb_pop_rule(priority);
				}
				tree->rb_push_rule(temp_tree->max_priority_rule);

			//	rb_tree_destroy(temp_tree);
				//ClearStack(stack_so_far, tree);
				//tree->rb_pop_rule(priority);
				return ;
			}*/
			temp_tree->count--;
			rb_red_blk_node * x = temp_tree->root->left;
			if (x->left == temp_tree->nil && x->right == temp_tree->nil) {
				box_vector_push_back((*tree)->chain_boxes, x->key);
				//stack_so_far.push(std::make_pair(temp_tree, x));
				temp_tree = x->rb_tree_next_level;
			}
			else 
			{
				int compare_result = compare_box(x->key, key[field_order[level+run]]);
				//stack_so_far.push(std::make_pair(temp_tree, x));
				if (compare_result == 0) { //hit top = delete top then go leaf node to collect correct chain box
					if (x->left == temp_tree->nil) {
						temp_tree = x->right->rb_tree_next_level;
						box_vector_push_back((*tree)->chain_boxes, x->right->key);
					}
					else {
						temp_tree = x->left->rb_tree_next_level;
						box_vector_push_back((*tree)->chain_boxes, x->left->key);
					}
				//	tree->chain_boxes.push_back(x->left == temp_tree->nil?x->right->key:x->left->key);
				//	temp_tree = x->rb_tree_next_level;
				}
				else {
					temp_tree = x->rb_tree_next_level;
					box_vector_push_back((*tree)->chain_boxes, x->key);
				}
				
				/*if (compare_result == -1){  root < z.key 
						//delete right child
						//stack_so_far.push(std::make_pair(temp_tree, x->right)); 
						tree->chain_boxes.push_back( x->key);
						temp_tree = x->right->rb_tree_next_level;
				} else if(compare_result == 1) {
						//delete left child
						//stack_so_far.push(std::make_pair(temp_tree, x->left));
						temp_tree = x->left->rb_tree_next_level;
						tree->chain_boxes.push_back(x->key);
				} else {
					printf("Warning::rb_tree_delete_with_path_compression rb_overlap at level %d\n",level+run);
				}*/
				
			}
			run++;
		
		}
		return ;
	}

	
	rb_red_blk_node* x;
	rb_red_blk_node* y;
	rb_red_blk_node* nil = (*tree)->nil;
	y = (*tree)->root;
	x = (*tree)->root->left;


	while (x != nil) {
		y = x;
		int compare_result = compare_box(x->key, key[field_order[level]]);
		if (compare_result == 1) { /* x.key > z.key */
			x = x->left;
		} else if (compare_result == -1) { /* x.key < z.key */
			x = x->right;
		} else if (compare_result == 0) {  /* x.key = z.key */ 	
			bool just_delete = false;
			(*tree)->count--;
			rb_tree_delete_with_path_compression(&x->rb_tree_next_level, key, level + 1, field_order, NUM_FIELDS, priority, &just_delete);
			if (just_delete) rb_delete(*tree, x);

			return; 
		
		} else {  /* x.key || z.key */
			printf("x:[%u %u], key:[%u %u]\n", x->key[low_dim], x->key[high_dim], key[field_order[level]][low_dim], key[field_order[level]][high_dim]);
			printf("Warning RBFindNodeSequence : x.key || key[field_order[level]]\n");
		}
	}

	printf("Error rb_tree_delete_with_path_compression: can't find a node at level %d\n", level);
	exit(0);


}
/***********************************************************************/
/*  FUNCTION:  rb_delete */
/**/
/*    INPUTS:  tree is the tree to delete node z from */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Deletes z from tree and frees the key and info of z */
/*             using DestoryKey and DestoryInfo.  Then calls */
/*             rb_delete_fix_up to restore red-black properties */
/**/
/*    Modifies Input: tree, z */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

void rb_delete(rb_red_blk_tree* tree, rb_red_blk_node* z){
  rb_red_blk_node* y;
  rb_red_blk_node* x;
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* root=tree->root;

  y= ((z->left == nil) || (z->right == nil)) ? z : rb_tree_successor(tree,z);
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
    Assert( (y!=tree->nil),"y is nil in rb_delete\n");
#endif
    /* y is the node to splice out and x is its child */

    if (!(y->red)) rb_delete_fix_up(tree,x);
  
  //  tree->DestroyKey(z->key);
  //  tree->DestroyInfo(z->info);
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
    rb_node_destroy(z);
  } else {
//    tree->DestroyKey(y->key);
 //   tree->DestroyInfo(y->info);
    if (!(y->red)) rb_delete_fix_up(tree,x);
    rb_node_destroy(y);
  }
  
#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not black in rb_delete");
#endif
}


/***********************************************************************/
/*  FUNCTION:  RBDEnumerate */
/**/
/*    INPUTS:  tree is the tree to look for keys >= low */
/*             and <= high with respect to the Compare function */
/**/
/*    OUTPUT:  stack containing pointers to the nodes between [low,high] */
/**/
/*    Modifies Input: none */
/***********************************************************************/

stk_stack* rb_enumerate(rb_red_blk_tree* tree, const box low, const box high) {
  stk_stack* enum_result_stack;
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* x=tree->root->left;
  rb_red_blk_node* last_best=nil;

  enum_result_stack=stack_create();
  while(nil != x) {
    if ( 1 == (compare_box(x->key,high)) ) { /* x->key > high */
      x=x->left;
    } else {
      last_best=x;
      x=x->right;
    }
  }
  while ( (last_best != nil) && (1 != compare_box(low,last_best->key))) {
    stack_push(enum_result_stack,last_best);
    last_best=rb_tree_predecessor(tree,last_best);
  }
  return(enum_result_stack);
}

/***********************************************************************/
/*  FUNCTION:  rb_serialize_into_rules_recursion */
/**/
/*    INPUTS:  tree: 'tree' at 'level' for current 'field_order'
/*              
/**/
/*    OUTPUT:  a vector of rule */
/**/
/*    Modifies Input: none */
/***********************************************************************/

//!!!!!MAKE SURE TO DEALLOCATE ALL RULES CREATED HERE OR MEMORY LEAK WILL OCCUR!!!!!

void  rb_serialize_into_rules_recursion(rb_red_blk_tree * tree_node, rb_red_blk_node * node, int level, const int *field_order, const int NUM_FIELDS, box_vector *box_so_far, rule_vector *rules_so_far) {
	if (level == NUM_FIELDS ) {
		for (int n = 0; n < rule_vector_size(tree_node->rule_list); ++n) {
			rule *r = rule_create(NUM_FIELDS);
			for (int i = 0; i < NUM_FIELDS; ++i){
				box temp = box_vector_get(box_so_far, i);
				r->range[field_order[i]][0] = temp[0];
				r->range[field_order[i]][1] = temp[1];
			}
			rule *temp_rule = rule_vector_get(tree_node->rule_list, n);
			r->priority = temp_rule->priority;
			r->master = temp_rule->master;
			rule_vector_push_back(rules_so_far, r);
		}
		return;
	}
	if (tree_node->count == 1) {
		box_vector_append(box_so_far, tree_node->chain_boxes);
		for (int n = 0; n < rule_vector_size(tree_node->rule_list); ++n) {
			rule *r = rule_create(NUM_FIELDS);
			for (int i = 0; i < NUM_FIELDS; ++i){
				box temp = box_vector_get(box_so_far, i);
				r->range[field_order[i]][0] = temp[0];
				r->range[field_order[i]][1] = temp[1];
			}
			rule *temp_rule = rule_vector_get(tree_node->rule_list, n);
			r->priority = temp_rule->priority;
			r->master = temp_rule->master;
			rule_vector_push_back(rules_so_far, r);
		}
		
		for (size_t i = 0; i < box_vector_size(tree_node->chain_boxes); ++i)
			box_vector_pop_back(box_so_far);

		return;
	}

	if (tree_node->nil == node) return;

	box_vector_push_back(box_so_far, node->key);
	rb_red_blk_tree *tree = node->rb_tree_next_level;
	rb_serialize_into_rules_recursion(tree,tree->root->left, level + 1, field_order, NUM_FIELDS, box_so_far, rules_so_far);
	box_vector_pop_back(box_so_far);

	rb_serialize_into_rules_recursion(tree_node,node->left, level, field_order, NUM_FIELDS, box_so_far, rules_so_far);
	rb_serialize_into_rules_recursion(tree_node,node->right, level, field_order, NUM_FIELDS, box_so_far, rules_so_far);

}

rule_vector* rb_serialize_into_rules(rb_red_blk_tree* tree, const int *field_order, const int NUM_FIELDS) {
	box_vector *boxes_so_far = box_vector_init();
	rule_vector *rules_so_far = rule_vector_init();
	
	rb_serialize_into_rules_recursion(tree,tree->root->left, 0, field_order, NUM_FIELDS, boxes_so_far, rules_so_far);
	box_vector_free(boxes_so_far);
	return rules_so_far;
}

int  rb_calculate_memory_consumption_recursion(rb_red_blk_tree * tree_node, rb_red_blk_node * node, int level, const int *field_order, const int NUM_FIELDS) {
	if (level == NUM_FIELDS) {

		int num_rules_in_this_leaf = rule_vector_size(tree_node->rule_list);
		int sizeofint = 4;
		long long also_max_priority = 1;
		return (num_rules_in_this_leaf + also_max_priority)*sizeofint;

	}
	if (tree_node->count == 1) {
		int size_of_remaining_intervals = 0;
		for (size_t i = level; i < NUM_FIELDS; ++i) {
			int field = field_order[i];
			int intervalsize;
			if (field == 0 || field == 1) {
				intervalsize = 9;
			}
			if (field == 2 || field == 3) {
				intervalsize = 4;
			}
			if (field == 4) {
				intervalsize = 1;
			}
			size_of_remaining_intervals += intervalsize;
		} 
		int num_rules_in_this_leaf = rule_vector_size(tree_node->rule_list);
		int sizeofint = 4;
		long long also_max_priority = 1;

		return   size_of_remaining_intervals + (num_rules_in_this_leaf + also_max_priority)*sizeofint;
	}

	if (tree_node->nil == node) return 0;
	
	int memory_usage = 0; 
	rb_red_blk_tree *tree = node->rb_tree_next_level;

	memory_usage += rb_calculate_memory_consumption_recursion(tree, tree->root->left, level + 1, field_order, NUM_FIELDS);
	memory_usage += rb_calculate_memory_consumption_recursion(tree_node, node->left, level, field_order, NUM_FIELDS);
	memory_usage += rb_calculate_memory_consumption_recursion(tree_node, node->right, level, field_order, NUM_FIELDS);

	int number_pointers_in_internal_node = 4;
	int aux_data_internal_node_byte = 1;
	int sizeofint = 4;
	long long also_max_priority_interval_node = 1;

	int field = field_order[level]%5;
	int intervalsize;
	if (field == 0 || field == 1) {
		intervalsize = 9;
	}
	if (field == 2 || field == 3) {
		intervalsize = 4;
	}
	if (field == 4) {
		intervalsize = 1;
	}
	return memory_usage + intervalsize+ (number_pointers_in_internal_node+ also_max_priority_interval_node) *sizeofint + aux_data_internal_node_byte;
}

int rb_calculate_memory_consumption(rb_red_blk_tree* tree, const int *field_order, const int NUM_FIELDS) {

	return rb_calculate_memory_consumption_recursion(tree, tree->root->left, 0, field_order, NUM_FIELDS);
}

rb_red_blk_node* rb_node_create(){
	rb_red_blk_node *node = (rb_red_blk_node*)safe_malloc(sizeof(rb_red_blk_node));
	node->key = (box)safe_malloc(sizeof(point) * BOX_SIZE);
	return node;
}

void rb_node_destroy(rb_red_blk_node *node){
	free(node->key);
	free(node);
}


