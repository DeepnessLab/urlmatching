// lossycount.h -- header file for Lossy Counting
// see Manku & Motwani, VLDB 2002 for details
// implementation by Graham Cormode, 2002,2003

// Modified: Supporting buffer items instead of integers. 
//           Golan Parashi, Nov, 2013.

#ifndef LOSSYCOUNTING_h
#define LOSSYCOUNTING_h

#include <vector>
#include <map>
#include "common.h"

//////////////////////////////////////////////////////
typedef int LCUWT;
//#define LCU_SIZE 101
// if not defined, then it is dynamically allocated based on user parameter
//////////////////////////////////////////////////////

#define LCU_HASHMULT 3
#ifdef LCU_SIZE
#define LCU_TBLSIZE (LCU_HASHMULT*LCU_SIZE)
#endif

typedef struct lcu_itemlist LCUITEMLIST;
typedef struct lcu_group LCUGROUP;

typedef struct lcu_item LCUITEM;
typedef struct lcu_group LCUGROUP;

struct lcu_group {
  LCUWT     count;
  LCUITEM   *items;
  LCUGROUP  *previousg, *nextg;
};

struct lcu_item  {
  buffer_t item;
  int      hash;
  LCUWT    delta;
  LCUGROUP *parentg;
  LCUITEM  *previousi, *nexti;
  LCUITEM  *nexting, *previousing;
  
  LCUWT    real_count;  
  // including appearances as sub-buffer of other k-gram series
  
  LCUWT    real_count_all_series;
  // sum of all real_count of all the series where this item appears in as sub-buffer
  size_t firstPacket;
  size_t decrement;

};

typedef struct LCU_type{
  LCUWT     n;
  int       gpt;
  int       k;
  int       tblsz;
  LCUGROUP  *root;
#ifdef LCU_SIZE
  LCUITEM   items[LCU_SIZE];
  LCUGROUP  groups[LCU_SIZE];
  LCUGROUP  *freegroups[LCU_SIZE];
  LCUITEM*  hashtable[LCU_TBLSIZE];
#else
  LCUITEM   *items;
  LCUGROUP  *groups;
  LCUGROUP  **freegroups;
  LCUITEM   **hashtable;
#endif

} LCU_type;

extern LCU_type* LCU_Init(float fPhi);
extern void LCU_Destroy(LCU_type *);
extern LCUWT LCU_Update(LCU_type *, unsigned char*, size_t, size_t);
extern int LCU_Size(LCU_type *);
extern std::map<buffer_t, uint32_t> LCU_Output(LCU_type * lcu, int thresh);
extern void LCU_Print_Output(LCU_type *lcu);
extern void LCU_Calc_Total_Count(LCU_type *lcu);
extern std::vector<LCUITEM *> LCU_List_Items(LCU_type * lcu);

#endif
