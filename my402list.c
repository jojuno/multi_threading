/*
 * Author:      Moonsoo Jo (moonsooj@usc.edu)
 * Structure provided by:    William Chia-Wei Cheng (bill.cheng@usc.edu)
 *
 * @(#)$Id: listtest.c,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cs402.h"

#include "my402list.h"

int  My402ListLength(My402List* list) {
    return list -> num_members;
    // return 0;
}
int  My402ListEmpty(My402List* list) {

    if (list -> num_members == 0) {
        return TRUE;
    }
    return FALSE;
}

int  My402ListAppend(My402List* list, void* obj) {
    //use num members
    My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
    My402ListElem *anchorPtr = &(list -> anchor);

    newElem -> obj = obj;
    //pass an argument into the function via parentheses after "Empty"
    //if (list -> Empty(list) == TRUE) {
    if (My402ListEmpty(list)) {

        newElem -> prev = anchorPtr;
        newElem -> next = anchorPtr;
        anchorPtr -> next = newElem;
        anchorPtr -> prev = newElem;
        //assign address to pointers
        list -> num_members = (list -> num_members) + 1;
        return TRUE;
    } else {
        My402ListElem *lastElem = My402ListLast(list);

        newElem -> next = anchorPtr;
        newElem -> prev = lastElem;
        lastElem -> next = newElem;
        anchorPtr -> prev = newElem;
        list -> num_members = (list -> num_members) + 1;

        return TRUE;
    }
}

int  My402ListPrepend(My402List* list, void* obj) {
    //use num members
    My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
    newElem -> obj = obj;
    My402ListElem *anchorPtr = &(list->anchor);
    //pass an argument into the function via parentheses after "Empty"
    //if (list -> Empty(list) == TRUE) {
    if (My402ListEmpty(list)) {
        newElem -> prev = anchorPtr;
        newElem -> next = anchorPtr;
        anchorPtr -> next = newElem;
        anchorPtr -> prev = newElem;
        //assign address to pointers
        list -> num_members = (list -> num_members) + 1;
        return TRUE;
    } else {
        My402ListElem *firstElem = (My402ListElem*)malloc(sizeof(My402ListElem));
        firstElem = My402ListFirst(list);

        newElem -> prev = anchorPtr;
        newElem -> next = firstElem;
        firstElem -> prev = newElem;
        anchorPtr -> next = newElem;
        list -> num_members = (list -> num_members) + 1;

        return TRUE;
    }
    return FALSE;
}
void My402ListUnlink(My402List* list, My402ListElem* elem) {
    My402ListElem *prevElem = elem -> prev;
    My402ListElem *nextElem = elem -> next;
    prevElem -> next = nextElem;
    nextElem -> prev = prevElem;
    list -> num_members = list -> num_members - 1;
    free(elem);
}
void My402ListUnlinkAll(My402List* list) {
    while (My402ListEmpty(list) == FALSE) {
        My402ListUnlink(list, My402ListFirst(list));
    }
}
int  My402ListInsertAfter(My402List* list, void* obj, My402ListElem* elem) {
    if (elem->next == NULL) {
        My402ListPrepend(list, obj);
        return TRUE;
    } else {
        My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
        My402ListElem *nextElem = elem->next;

        newElem->prev = elem;
        newElem->next = nextElem;

        newElem->obj = obj;
        elem->next = newElem;
        nextElem->prev = newElem;

        list -> num_members = list -> num_members + 1;
        return TRUE;
    }
    return FALSE;
}

//link new elem to other elems first
int  My402ListInsertBefore(My402List* list, void* obj, My402ListElem* elem) {
    if (elem->prev == NULL) {
        My402ListAppend(list, obj);
        return TRUE;
    } else {
        My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
        My402ListElem *prevElem = elem->prev;

        newElem->next = elem;
        newElem->prev = prevElem;
        elem->prev = newElem;
        prevElem->next = newElem;
        newElem->obj = obj;
        list -> num_members = list -> num_members + 1;
        return TRUE;
    }
    return FALSE;
}

My402ListElem *My402ListFirst(My402List* list) {
    if (My402ListEmpty(list)) {
        return NULL;
    }
    My402ListElem *anchorPtr = &(list -> anchor);
    return anchorPtr -> next;
    //return NULL;
}
My402ListElem *My402ListLast(My402List* list) {
    if (My402ListEmpty(list)) {
        return NULL;
    }
    My402ListElem *anchorPtr = &(list -> anchor);
    return anchorPtr -> prev;
    //return NULL;
}
My402ListElem *My402ListNext(My402List* list, My402ListElem* elem) {
    //elem already points to an element in the list
    if (&(list->anchor) == elem->next) {
        return NULL;
    }
    return elem->next;
    //return NULL;
}
My402ListElem *My402ListPrev(My402List* list, My402ListElem* elem) {
    if (&(list->anchor) == elem->prev) {
        return NULL;
    }
    return elem->prev;
    //return NULL;
}

My402ListElem *My402ListFind(My402List* list, void* obj) {
    //traverse list until an elem is found with the content of obj
    My402ListElem *elem = NULL;
    for (elem = My402ListFirst(list); 
        elem != NULL; 
        elem = My402ListNext(list, elem)) {
            if ((elem->obj) == obj) {
                return elem;
            }
    }
    return NULL;
}

int My402ListInit(My402List* list) {
    //empty 'list'
    //fill it in
    My402ListUnlinkAll(list);
    My402ListElem *anchorPtr = &(list -> anchor);
    if (My402ListLength(list) == 0) { 
        anchorPtr -> next = &(list -> anchor);
        anchorPtr -> prev = &(list -> anchor);
        return TRUE;
    }
    return FALSE;
}