/* All linked list quiz can be solved by struct list *prev, *curr, *post; */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "slist.h"

slist *proplist_init(void)
{
  slist *l = malloc(sizeof(slist));
  if (!l) {
    printf("No Mem\n");
    return NULL;
  }
  l->head = NULL;
  return l;
}

void proplist_final(slist *l)
{
  struct node *curr = l->head;
  struct node *post;

  for(;curr != NULL; ) {
    post = curr->next;
    free(curr);
    curr = post;
  }

  free(l);
  return;
}
int proplist_get_length(slist *l)
{
  struct node *curr = l->head;
  int len = 0;
  for(;curr != NULL;){
    len++;
    curr=curr->next;
  }
  return len;
}

void proplist_insert(slist *l, int index, Property *data)
{
  struct node *curr=l->head;
  struct node *prev=NULL;
  struct node *new=NULL;

  if(index < 1 || index-1 > proplist_get_length(l)) {
    printf("pos not exist\n");
    return;
  }

  new = malloc(sizeof(struct node));
  if(!new){
    printf("no memory\n");
    return;
  }

  if(index == 1) {
	memcpy(&(new->prop), data, sizeof(Property));
    new->next = l->head;
    l->head = new;
  } else {
    for (int i=1; i<index; i++) {
      prev=curr;
      curr=curr->next;
    }
   	memcpy(&(new->prop), data, sizeof(Property));
    new->next = prev->next;
    prev->next = new;
  }
}

void proplist_delete(slist *l, int index)
{
  struct node *curr=l->head;
  struct node *prev;

  if(l->head == NULL || index < 1 || index > proplist_get_length(l)) {
    printf("pos not exist\n");
    return;
  }

  if(index == 1){//first node
    l->head=curr->next;
    free(curr);
  } else {
    for (int i=1; i<index; i++){
      prev=curr;
      curr=curr->next;
    }
    prev->next=curr->next;
    free(curr);
  }

  return;
}
void proplist_print(slist *l)
{
  if(l == NULL || l->head ==NULL) {
    return;
  }

  struct node *curr = l->head;
  for(;curr != NULL;) {
    printf("%s: %s\n", curr->prop.key, curr->prop.value);
    curr=curr->next;
  }
  printf("\n");
}

