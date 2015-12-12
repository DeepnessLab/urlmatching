/***************************************************************************************************
'C' bitmap functions 'bitmapfs' (c) 2002-2012 Alf Lacis.
mail 1: alfredo4570 at gmail dot com, or 2: lacis_alfredo at yahoo dot com
http://alfredo4570.net

This library is free software; you can redistribute it and/or modify it under the terms of the GNU
Lesser General Public License as published by the Free Software Foundation; either version 2.1 of
the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with this library;
if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA
----------------------------------------------------------------------------------------------------


Large Bitmap Routines
=====================
Declare a pointer such as 'unsigned char *map' (ie, pointer to unsigned char).

'bit_index' is a number from 0 to the maximum number of bits-1 << range checking is to be
done by the application programmer.

Example Usage:
==============
Refer to bitmaptest.c

20120504 alf Because some CPUs don't have a hardware divide opcode, and because all divisions
             involve the value '8', changed division (/) and modulo (%) to shift & mask operators.
             (An optimising compiler could do this, but we won't rely on this possibility.)
20120309 alf Changed url to http://alfredo4570.net
20100908 alf Macrod BM_FREE to use NULLIFY.
20061012 alf Converted bit_index and bit_max parameters from 'int' to 'size_t'. Removed macros
             after doing speed tests under gcc compiler against functions. Functions are slightly
             slower with zero optimisation, but as soon as optimisation is turned on, functions are
             slightly faster. There is no strong argument either way, except, of course, that
             multiply-called functions will result in a smaller application than repeated in-line
             code macros. Added defined(__GNUC__). Renamed bitno to bit_index & bitmax to bit_max.
20050224 alf Removed old, commented-out code. Replaced HEW with __RENESAS__.
20041123 alf Moved declaration for BM_ANY to outside of the BM_FUNCS_AS_FUNCTIONS fence. Put some
             extra braces in BM_ASSIGN because I questioned its operation.
20041025 alf Put fence BM_NO_MALLOC around BM_ALLOC() & BM_FREE() because HEW linker does not
             properly remove 'dead' code reference to malloc().
20041022 alf Added extra parentheses around 'map' in the macros for BM_SET(), BM_CLR(), BM_TEST()
             & BM_ASSIGN().  This is required if the bitmap is not an array, ie, is just a single
             variable.  This correctly associates the '&' operator with 'map'.
20041021 alf Defined as macro regardless of BM_FUNCS_AS_FUNCTIONS flag:
                 #define BM_MAX_BYTES(bit_max)   ( ( (bit_max)/8 ) + 1 )
20040928 alf Added BM_DECLARE(map,bit_max) to declare a bitmap, instead of using malloc().

***************************************************************************************************/

#ifndef _bitmapfs_h_
#define _bitmapfs_h_ 1

#include <stdlib.h>
#include <stddef.h>        // for size_t

#ifndef BM_SIZE_T          // May be any other type, eg, int.
#define BM_SIZE_T size_t   // But size_t is most widely used
#endif

//#if defined(__GNUC__) || defined(__arm) || (__CPU_MODE__==ARM_MODE) || defined(_CVI_) || defined(__RENESAS__)
#if defined(__GNUC__) || defined(__arm) || defined(ARM_MODE) || defined(_CVI_) || defined(__RENESAS__)
#include <string.h>
#endif

#if defined __BORLANDC__
//#include <mem.h> // 20041020- alf Removed because HEW dependency check does not do conditional checking.
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

// 20041021 alf: Defined as macros regardless of BM_FUNCS_AS_FUNCTIONS flag

//#define BM_MAX_BYTES(bit_max)        ( ( (bit_max)/8 ) + 1 )
#define BM_MAX_BYTES(bit_max)        ( ( (bit_max) >> 3 ) + 1 )  // 2012-05-04 AL*

#define BM_DECLARE(map,bit_max)      unsigned char map[BM_MAX_BYTES(bit_max)]

#ifndef BM_NO_MALLOC // 20041025 alf:  Put fence BM_NO_MALLOC around BM_ALLOC() & BM_FREE().
extern unsigned char *BM_ALLOC (BM_SIZE_T bit_max);
//extern void           BM_FREE  (unsigned char *map); // now done in a macro
#define               BM_FREE(a) NULLIFY(a)  // 2011-09-08 *
#endif // #ifndef BM_NO_MALLOC
extern void           BM_SET   (unsigned char *map, BM_SIZE_T bit_index);
extern void           BM_CLR   (unsigned char *map, BM_SIZE_T bit_index);
extern int            BM_TEST  (unsigned char *map, BM_SIZE_T bit_index);
extern void           BM_ASSIGN(unsigned char *map, int value, BM_SIZE_T bit_index);
extern unsigned char *BM_ALL   (unsigned char *map, int value, BM_SIZE_T bit_max);
extern int            BM_ANY   (unsigned char *map, int value, BM_SIZE_T bit_max);

#endif // #ifndef _bitmapfs_h_

