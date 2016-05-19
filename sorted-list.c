#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sorted-list.h"

SortedListPtr SLCreate(CompareFuncT cf, DestructFuncT df){

	SortedListPtr list = (SortedListPtr)malloc(sizeof(struct SortedList));
	list->compare = cf;
	list->exclude = df;
	
	list->head= NULL;		

	return list;
}


void SLDestroy(SortedListPtr list){
	//this will start the process of deleting the main SLcreate list and pointers associated ot it
	
	Node *exterminator = list->head;
	while (exterminator != NULL){
		Node *exterminatorSub =exterminator;
		exterminator = exterminator->next;
		free(exterminatorSub); 
		if (exterminator->next ==NULL){
			free(exterminator);
			free(list);
			return;
		}
	}
	//accounts for the last node which will be taken care of here.
	
	free(list);
	return;
}



//===0.2: List Insert/Remove

/*
 * SLInsert inserts a given data item 'newObj' into a SortedList while maintaining the
 *   order and uniqueness of list items.
 * 
 * SLInsert should return 1 if 'newObj' is not equal to any other items in the list and
 *   was successfully inserted.
 * SLInsert should return 0 if 'newObj' is equal to an item already in the list or it was
 *   not successfully inserted
 *
 * Data item equality should be tested with the user's comparator function *
 */

int SLInsert(SortedListPtr list, void *newObj){
	//these pointers will be used in the sorting insert as we go along
	Node *ptrPrev=NULL;
	Node *ptrCurr=list->head; 
	
	//first case when there is nothing in list, just put head and the newobj as head
	if (list->head == NULL){
	Node* temp = malloc(sizeof(Node));
	temp->data = newObj;
	temp->count = 1;
	list->head = temp;
	return 1;
	}
		//itterates through to find a place to plop the node
	while (ptrCurr != NULL && list->compare(ptrCurr->data, newObj )>0){
		ptrPrev = ptrCurr;
		ptrCurr = ptrCurr->next;

	}
		//if we need to add at the start of head when its greatest
	if (ptrPrev==NULL && list->compare(ptrCurr->data, newObj) <0){
		Node* temp = malloc(sizeof(Node));
		temp->data= newObj;
		temp->count = 1;
		list->head->count--;
		temp->next=list->head;
		list->head = temp;
		return 1;
		
	}
	if(ptrPrev==NULL && list->compare(ptrCurr->data, newObj)==0){ // same object
		return 0;
	}	

		//if we reach the end, just add to end
	if(ptrCurr == NULL){
		Node* temp = malloc(sizeof(Node));
		temp->count = 0;
		temp->data = newObj;
		ptrPrev->next = temp;
		return 1;

	}  if (list->compare(ptrCurr->data, newObj)==0) {
		return 0;
	// list has reached end, just put it at the end now since it is smallest item
	} else {
		Node* temp = malloc(sizeof(Node));
		temp->data = newObj;
		temp->count = 0;
		temp->next = ptrCurr;
		ptrPrev->next = temp;
		return 1;
	}

	return 0;

}




/*
 * SLRemove should remove 'newObj' from the SortedList in a manner that
 *   maintains list order.
 *
 * SLRemove must not modify the data item pointed to by 'newObj'.
 *
 * SLRemove should return 1 on success, and 0 on failure.
 */

int SLRemove(SortedListPtr list, void *newObj){
	Node* curr = list->head;
	Node* prev = NULL;

	while(curr != NULL){
		if(curr->data == newObj){
			curr->count--;
			if(prev == NULL){ //if we are removing the head 
				list->head = curr->next;
				list->head->count++;
				if(curr->count < 1){
					free(curr);		
				}
				return 1;
			}
			Node* temp = curr;
			prev->next = curr->next;
			if(curr->count < 1){
				free(curr);
			}
			return 1;
		}else{
			prev = curr;
			curr = curr->next;
		}
	}

	return 0;
}



//======1: SortedList Iterator============================
//===1.1: SortedList Iterator Definition, Create/Destroy

/*
 * SortListIterator allows a SortedList to be easily 'walked' through,
 *  item by item using repeated calls to 'SLNextItem'.
 * Each 'SLNextItem' call to a SortedListIterator should result in the 
 *  next consecutive item in a SortedList, from the beginning to the end.
 * A SortedListIterator provides a one-way traversal through all of a SortedList
 */


/*
 * SLCreateIterator creates a SortedListIterator for the SortedList pointed to by 'list'.
 *
 * SLCreateIterator should return a non-NULL SortedListIteratorPtr on success.
 * SLCreateIterator should return a NULL SortedListIteratorPtr if it could not
 *  construct a SortedListIterator, or if the SortedListPtr parameter 'list' is NULL.
 */

SortedListIteratorPtr SLCreateIterator(SortedListPtr list){

   SortedListIteratorPtr sli = (SortedListIteratorPtr) malloc(sizeof(struct SortedListIterator));
	//check to see if there is nothing in list and makes sure we don't create and empty list for no reason
      if(sli == NULL || list == NULL){
          // This is so that if the list does not contain any value inside for anything to read
          return NULL;

      }else{
          // This turns the pointer to move it forwards along with the list and then return
          sli->current = list->head;
                   
         return sli;


}
}


/*
 * SLDestroyIterator destroys a SortedListIterator pointed to by parameter 'iter'.
 *
 * SLDestroyIterator should destroy the SortedListIterator, but should NOT
 *  affect the list used to create the SortedListIterator in any way.
 */

void SLDestroyIterator(SortedListIteratorPtr iter){
	if(iter->current == NULL){
		free(iter);
	}else{
		iter->current->count--;
		free(iter);
	}
}



//===1.2: SortedList Iterator Get/Next Operations

/*
 * SLNextItem returns a pointer to the data associated with the
 *  next item in the SortedListIterator's list
 *
 * SLNextItem should return a NULL if all of iter's elements have
 *  been iterated through.
 *
 * NB: Be sure to check the length of the list that SortedListIterator holds
 *         before attempting to access and return an item from it.
 *         If an item is removed from the list, making it shorter, be careful not
 *         to try to read and return an item off the end of the list.
 */

void * SLNextItem(SortedListIteratorPtr iter){
	if(iter->current==NULL){

        	 // The item in the next section does not contain any data
         	 return NULL;
      	}
      	//decrement pointer counter by one
	iter->current->count--;
         // Continue down to the next node next
        iter->current = iter->current->next;
      return SLGetItem(iter);

}


/*
 * SLGetItem should return a pointer to the current data item
 *   The current data item is the item that was most recently returned by SLNextItem
 *   SLGetItem should not alter the data item that SortedListIterator currently refers to
 *
 * SLGetItem should return a NULL pointer if the SortedListIterator has been advanced
 *  all the way through its list.
 *
 * NB: Be sure to check the length of the list that SortedListIterator holds
 *         before attempting to access and return an item from it.
 *         If an items are removed from the list, making it shorter, be careful not
 *         to try to read and return an item off the end of the list.
 */

void * SLGetItem( SortedListIteratorPtr iter ){
	//if nothing inside return
	if(iter->current==NULL){
		return NULL;
	}
	else{
		//increment pointer by 1
		iter->current->count++;
		//Just get data from current node which was set up by SLNextItem
		return iter->current->data;
	}

}




