#ifndef _SLIST_H_
#define _SLIST_H_

#include "propd.h"

struct node
{
  Property prop;
  struct node *next;
};

typedef struct {
  struct node *head;
} slist;

slist *proplist_init(void);
void proplist_final(slist *l);
int proplist_get_length(slist *l);
void proplist_insert(slist *l, int index, Property *data);
void proplist_delete(slist *l, int index);
void proplist_print(slist *l);
#endif
