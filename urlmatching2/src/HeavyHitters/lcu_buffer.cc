#include <stdlib.h>
#include <stdio.h>
#include "lcu_buffer.h"
#include "crc32c.h"
#include <iostream>
#include "addons.h"

using namespace std;


/********************************************************************
Implementation of Frequent algorithm to Find Frequent Items
Based on papers by:
Misra and Gries, 1982
Demaine, Lopez-Ortiz, Munroe, 2002
Karp, Papadimitriou and Shenker, 2003
Implementation by G. Cormode 2002, 2003

Original Code: 2002-11
This version: 2003-10

This work is licensed under the Creative Commons
Attribution-NonCommercial License. To view a copy of this license,
visit http://creativecommons.org/licenses/by-nc/1.0/ or send a letter
to Creative Commons, 559 Nathan Abbott Way, Stanford, California
94305, USA. 
 *********************************************************************/

LCU_type * LCU_Init(float fPhi) {
    int i;
    int k = 1 + (int) 1.0 / fPhi;

    LCU_type* result = (LCU_type*) calloc(1, sizeof (LCU_type));

    if (k < 1) k = 1;
    result->k = k;
    result->n = 0;

    result->tblsz = LCU_HASHMULT*k;
    result->hashtable = (LCUITEM**) calloc(result->tblsz, sizeof (LCUITEM *));
    result->groups = (LCUGROUP *) calloc(k, sizeof (LCUGROUP));
    result->items = (LCUITEM *) calloc(k, sizeof (LCUITEM));
    result->freegroups = (LCUGROUP **) calloc(k, sizeof (LCUGROUP*));

    for (i = 0; i < result->tblsz; i++)
        result->hashtable[i] = NULL;

    result->root = result->groups;
    result->groups->count = 0;
    result->groups->nextg = NULL;
    result->groups->previousg = NULL;

    result->groups->items = result->items;
    for (i = 0; i < k; i++)
        result->freegroups[i] = &result->groups[i];
    result->gpt = 1; // initialize list of free groups

    for (i = 0; i < k; i++) {
        result->items[i].item = buffer_t();
        result->items[i].real_count = 0;

        result->items[i].delta = 0;
        result->items[i].hash = 0;
        result->items[i].nexti = NULL;
        result->items[i].previousi = NULL; // initialize values

        result->items[i].parentg = result->groups;
        result->items[i].nexting = &(result->items[i + 1]);
        result->items[i].previousing = &(result->items[i - 1]);
        // create doubly linked list
    }
    result->items[0].previousing = &(result->items[k - 1]);
    result->items[k - 1].nexting = &(result->items[0]);
    // fix start and end of linked list

    return (result);
}

void LCU_ShowGroups(LCU_type * lcu) {
    LCUGROUP *g;
    LCUITEM *i, *first;
    int n, wt;

    g = lcu->groups;
    wt = 0;
    n = 0;
    while (g != NULL) {
        printf("Group %d :", g->count);
        first = g->items;
        i = first;
        if (i != NULL)
            do {
                printf("%lu -> ", i->item.size());
                i = i->nexting;
                wt += g->count;
                n++;
            } while (i != first);
        else printf(" empty");
        printf(")");
        g = g->nextg;
        if ((g != NULL) && (g->previousg->nextg != g))
            printf("Badly linked");
        printf("\n");
    }
    printf("In total, %d items, with a total count of %d\n", n, wt);
}

static inline void LCU_InsertIntoHashtable(LCU_type *lcu, LCUITEM *newi, int i, unsigned char *newbuf, size_t newbuf_size) {
    newi->nexti = lcu->hashtable[i];
    newi->item.assign(newbuf, newbuf + newbuf_size); // overwrite the old item
    newi->real_count = 0;
    newi->hash = i;
    newi->previousi = NULL;
    // insert item into the hashtable
    if (lcu->hashtable[i])
        lcu->hashtable[i]->previousi = newi;
    lcu->hashtable[i] = newi;
}

int LCU_cmp(const void * a, const void * b) {
    LCUITEM * x = (LCUITEM*) a;
    LCUITEM * y = (LCUITEM*) b;
    if (x->parentg->count < y->parentg->count) return -1;
    else if (x->parentg->count > y->parentg->count) return 1;
    else return 0;
}

std::vector<LCUITEM *> LCU_List_Items(LCU_type * lcu) {
    std::vector<LCUITEM *> item_list;
    
	for (int i = 0; i < lcu->tblsz; i++) {
		LCUITEM *il = lcu->hashtable[i];
        
		while (il) {
			item_list.push_back(il);
			il = il->nexti;
		}
	}

    return item_list;
}


std::map<buffer_t, uint32_t> LCU_Output(LCU_type * lcu, int thresh) {
    std::map<buffer_t, uint32_t> res;

    for (int i = 0; i < lcu->k; ++i)
        if (lcu->items[i].parentg->count >= thresh)
            res.insert(std::pair<buffer_t, uint32_t>(lcu->items[i].item, lcu->items[i].parentg->count));

    return res;
}

static inline LCUITEM * LCU_GetNewCounter(LCU_type * lcu) {
    LCUITEM * newi;
    int j;

    newi = lcu->root->items; // take a counter from the first group
    // but currently it remains in the same group

    newi->nexting->previousing = newi->previousing;
    newi->previousing->nexting = newi->nexting;
    // unhook the new item from the linked list in the hash table	    

    // need to remove this item from the hashtable
    j = newi->hash;
    if (lcu->hashtable[j] == newi)
        lcu->hashtable[j] = newi->nexti;

    if (newi->nexti != NULL)
        newi->nexti->previousi = newi->previousi;
    if (newi->previousi != NULL)
        newi->previousi->nexti = newi->nexti;

    return (newi);
}

static inline void LCU_PutInNewGroup(LCU_type * lcu, LCUITEM *newi, LCUGROUP * tmpg) {
    LCUGROUP * oldgroup;

    oldgroup = newi->parentg;
    // put item in the tmpg group
    newi->parentg = tmpg;

    if (newi->nexting != newi) // if the group does not have size 1
    { // remove the item from its current group
        newi->nexting->previousing = newi->previousing;
        newi->previousing->nexting = newi->nexting;
        oldgroup->items = oldgroup->items->nexting;
    } else { // group will be empty
        if (oldgroup->nextg != NULL) // there is another group
            oldgroup->nextg->previousg = oldgroup->previousg;
        if (lcu->root == oldgroup) // this is the first group
            lcu->root = oldgroup->nextg;
        else
            oldgroup->previousg->nextg = oldgroup->nextg;
        lcu->freegroups[--lcu->gpt] = oldgroup;
        // if we have created an empty group, remove it 
    }
    newi->nexting = tmpg->items;
    newi->previousing = tmpg->items->previousing;
    newi->previousing->nexting = newi;
    newi->nexting->previousing = newi;
}

static inline void LCU_AddNewGroupAfter(LCU_type * lcu, LCUITEM *newi, LCUGROUP *oldgroup) {
    LCUGROUP *newgroup;

    // remove item from old group...
    newi->nexting->previousing = newi->previousing;
    newi->previousing->nexting = newi->nexting;
    oldgroup->items = newi->nexting;
    //get new group
    newgroup = lcu->freegroups[lcu->gpt++];
    newgroup->count = oldgroup->count + 1; // set count to be one more the prev group
    newgroup->items = newi;
    newgroup->previousg = oldgroup;
    newgroup->nextg = oldgroup->nextg;
    oldgroup->nextg = newgroup;
    if (newgroup->nextg != NULL) // if there is another group
        newgroup->nextg->previousg = newgroup;
    newi->parentg = newgroup;
    newi->nexting = newi;
    newi->previousing = newi;
}

static inline void LCU_IncrementCounter(LCU_type * lcu, LCUITEM *newi) {
    LCUGROUP *oldgroup;

    oldgroup = newi->parentg;
    if ((oldgroup->nextg != NULL) &&
            (oldgroup->nextg->count - oldgroup->count == 1))
        LCU_PutInNewGroup(lcu, newi, oldgroup->nextg);
        // if the next group exists
    else { // need to create a new group with a differential of one
        if (newi->nexting == newi) // if there is only one item in the group...
            newi->parentg->count++;
        else
            LCU_AddNewGroupAfter(lcu, newi, oldgroup);
    }
    
    newi->real_count++;
}

static inline bool cmp_buf_to_raw(buffer_t& buffer, unsigned char *rawbuf, size_t rawbuf_size) {
    if (buffer.size() != rawbuf_size) return false;

    return std::memcmp(&buffer[0], rawbuf, rawbuf_size) == 0;
}

LCUWT LCU_Update(LCU_type * lcu, unsigned char *newbuf, size_t newbuf_size, size_t first) {
    LCUWT item_hh_count;
    LCUITEM *il;
    int h;

    lcu->n++;
    //I think that the tblsz might be different 
    h = crc32cComplete((const void *)newbuf, newbuf_size) % lcu->tblsz;
    il = lcu->hashtable[h];
    while (il) {
        if (cmp_buf_to_raw(il->item, newbuf, newbuf_size))
            break;
        il = il->nexti;
    }
    if (il == NULL) // item is not monitored (not in hashtable) 
    {
        il = LCU_GetNewCounter(lcu);
        //il->firstPacket = first;
	/// and put it into the hashtable for the new item 
        il->delta = lcu->root->count;
        // initialize delta with count of first group
        LCU_InsertIntoHashtable(lcu, il, h, newbuf, newbuf_size);
        // put the new counter into the first group
        // counter is already in first group by defn of how we got it
        LCU_IncrementCounter(lcu, il);
        //changed this
        item_hh_count = 0;//1;//= 0;
    } else {
        LCU_IncrementCounter(lcu, il);
        // if we have an item, we need to increment its counter 
        
        item_hh_count = il->parentg->count;
    }
    
    return item_hh_count;
}

int LCU_Size(LCU_type * lcu) {
    return sizeof (LCU_type)+(lcu->tblsz) * sizeof (LCUITEM*) +
            (lcu->k)*(sizeof (LCUITEM) + sizeof (LCUGROUP) + sizeof (LCUITEM*));
}

void LCU_Destroy(LCU_type * lcu) {
    if (NULL == lcu) {
        return;
    }
    
    free(lcu->freegroups);
    free(lcu->items);
    free(lcu->groups);
    free(lcu->hashtable);
    free(lcu);
}

void LCU_Print_Output(LCU_type *lcu) {
    std::map<buffer_t, uint32_t> hh_items = LCU_Output(lcu, 0);
    std::map<buffer_t, uint32_t>::iterator item_it;

    int i = 0;
    for (item_it = hh_items.begin(); item_it != hh_items.end(); item_it++) {
        buffer_t buf = item_it->first;

        printf("%d. item=##%.*s##, count=%d\n", ++i, (int) buf.size(), &buf[0], item_it->second);
    }
}


void LCU_Calc_Total_Count(LCU_type *lcu) {
    LCUITEM     *il;
    LCUITEM     *other_il;

    for (int i = 0; i < lcu->tblsz; ++i) {
        il = lcu->hashtable[i];
        
        while (il) {
            //current string is the string we are looking for in all other strings
            buffer_t& current_buffer      = il->item;
            size_t    current_buffer_size = current_buffer.size();
            
	    il->real_count_all_series = il->real_count; //try hh count instead
            //il->real_count_all_series = il->parentg->count; //this is the hh count//real_count; //try hh count instead
            
            // go over all other items and check if current one contained inside of them...
            for (int j = 0; j < lcu->tblsz; ++j) {
                other_il = lcu->hashtable[j];
                
                while (other_il) {
                    buffer_t& other_buffer = other_il->item;
		    //changed to <=
		    //Turns out the equals added all the cases of the same first packet which should have been obvious.
                    if (current_buffer_size <= other_buffer.size()) {
                        void* is_contained = memmem((void*)&other_buffer[0], other_buffer.size(),
                                                    (void*)&current_buffer[0], current_buffer_size);
                        
                        if (is_contained) {
			  //if (other_il->firstPacket != il->firstPacket){
			    //std::cout << "no double count" << std::endl;
			    //}
			    //else{
			    //std::cout << other_il->firstPacket << std::endl << il->firstPacket << std::endl;
                            il->real_count_all_series += other_il->real_count;
			    //}
			}
                    }                                       

                    other_il = other_il->nexti;
                }
            }
            
            il = il->nexti;
        }
    }
}
