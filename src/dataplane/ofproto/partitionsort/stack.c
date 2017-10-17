#include "stack.h"

void stack_destroy(stk_stack * the_stack,void dest_func(void * a));

int stack_not_empty(stk_stack * the_stack) {
  return( the_stack ? (int) the_stack->top : 0);
}

stk_stack * stack_join(stk_stack * stack1, stk_stack * stack2) {
  if (!stack1->tail) {
    free(stack1);
    return(stack2);
  } else {
    stack1->tail->next=stack2->top;
    stack1->tail=stack2->tail;
    free(stack2);
    return(stack1);
  }
}

stk_stack * stack_create() {
  stk_stack * new_stack;
  
  new_stack=(stk_stack *) safe_malloc(sizeof(stk_stack));
  new_stack->top=new_stack->tail=NULL;
  return(new_stack);
}


void stack_push(stk_stack * the_stack, DATA_TYPE new_info_pointer) {
  stk_stack_node * new_node;

  if(!the_stack->top) {
    new_node=(stk_stack_node *) safe_malloc(sizeof(stk_stack_node));
    new_node->info=new_info_pointer;
    new_node->next=the_stack->top;
    the_stack->top=new_node;
    the_stack->tail=new_node;
  } else {
    new_node=(stk_stack_node *) safe_malloc(sizeof(stk_stack_node));
    new_node->info=new_info_pointer;
    new_node->next=the_stack->top;
    the_stack->top=new_node;
  }
  
}

DATA_TYPE stack_pop(stk_stack * the_stack) {
  DATA_TYPE pop_info;
  stk_stack_node * old_node;

  if(the_stack->top) {
    pop_info=the_stack->top->info;
    old_node=the_stack->top;
    the_stack->top=the_stack->top->next;
    free(old_node);
    if (!the_stack->top) the_stack->tail=NULL;
  } else {
    pop_info=NULL;
  }
  return(pop_info);
}

void stack_destroy(stk_stack * the_stack,void dest_func(void * a)) {
  stk_stack_node * x=the_stack->top;
  stk_stack_node * y;

  if(the_stack) {
    while(x) {
      y=x->next;
      dest_func(x->info);
      free(x);
      x=y;
    }
    free(the_stack);
  }
} 
    
