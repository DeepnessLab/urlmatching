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


20120504 alf Because some CPUs don't have a hardware divide opcode, and because all divisions
             involve the value '8', changed division (/) and modulo (%) to shift & mask operators.
             (An optimising compiler could do this, but we won't rely on this possibility.)
             Moved test app to here from test_bitmapsfs.c.
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

#include <stdlib.h>
#include <stddef.h>     // for size_t
#include "bitmapfs.h"

#ifdef TEST_APP_BITMAPFS
#include <stdlib.h>
#include <stdio.h>
//#define BM_FUNCS_AS_FUNCTIONS 1
#include "alflb.h"
#endif

//#if defined(__GNUC__) || defined(__arm) || (__CPU_MODE__==ARM_MODE) || defined(_CVI_) || defined(__RENESAS__)
#if defined(__GNUC__) || defined(__arm) || defined(ARM_MODE) || defined(_CVI_) || defined(__RENESAS__) || defined(_WIN32)
#include <string.h>
#endif
#if defined __BORLANDC__
//#include <memory.h> // removed so HEW doesn't generate dependency error
#endif

#ifndef BM_NO_MALLOC // 20041025 alf:  Put fence BM_NO_MALLOC around BM_ALLOC() & BM_FREE().
unsigned char *BM_ALLOC(BM_SIZE_T bit_max)
{
	unsigned char *ptr;

	ptr = ((unsigned char *)malloc( (BM_SIZE_T)(BM_MAX_BYTES(bit_max)) ));

	return ptr;
}
//	void BM_FREE(unsigned char *map) // now done in a macro
//	{
//	    free((void*)map);
//	}
#endif // #ifndef BM_NO_MALLOC

// 2012-05-04 AL+
#define GET_INDICES \
	byteoffset = bit_index >> 3; \
	bitoffset  = bit_index &  7

void BM_SET(unsigned char *map, BM_SIZE_T bit_index)
{
	BM_SIZE_T byteoffset;
	BM_SIZE_T bitoffset;

	GET_INDICES;  // 2012-05-04 AL*

	map[byteoffset] |= (1 << bitoffset);
}

void BM_CLR(unsigned char *map, BM_SIZE_T bit_index)
{
	BM_SIZE_T byteoffset;
	BM_SIZE_T bitoffset;

	GET_INDICES;  // 2012-05-04 AL*

	map[byteoffset] &= ~(1 << bitoffset);
}

int BM_TEST(unsigned char *map, BM_SIZE_T bit_index)
{
	BM_SIZE_T byteoffset;
	BM_SIZE_T bitoffset;

	GET_INDICES;  // 2012-05-04 AL*

	if ( (map[byteoffset] & (1 << bitoffset)) != 0 )
		return 1;
	else
		return 0;
}

unsigned char *BM_ALL(unsigned char *map, int value, BM_SIZE_T bit_max)
{
	BM_SIZE_T max_bytes;

	max_bytes = BM_MAX_BYTES(bit_max);

	value = (value) ? 0xff : 0;

	return memset ( (void *)map, value, (size_t)max_bytes );
}

void BM_ASSIGN(unsigned char *map, int value, BM_SIZE_T bit_index)
{
	if ( value )
		BM_SET(map, bit_index);
	else
		BM_CLR(map, bit_index);
}

int BM_ANY(unsigned char *map, int value, BM_SIZE_T bit_max) // 20041022+
{
	BM_SIZE_T max_bytes;
	BM_SIZE_T max_aligned;
	BM_SIZE_T check;

	max_aligned = bit_max >> 3;  // 2012-05-04 AL*
	max_bytes   = BM_MAX_BYTES(bit_max);

	for ( check = 0; check < max_aligned; check++ ) // do a byte-wise check
	{
		if ( value ) // are we looking for any '1's?
		{
			if ( map[check] ) // have we found any '1's?
				return 1;
		}
		else // we are looking for any '0's
		{
			if ( ! map[check] ) // have we found any '0's?
				return 1;
		}
	}

	// If there's any left-over bits, test these separately in a bit-wise check.
	if ( max_aligned != max_bytes )
	{
		for ( check = max_aligned * 8; check < bit_max; check++ )
		{
			if ( value ) // are we looking for any '1's?
			{
				if ( BM_TEST(map, check) ) // have we found any '1's?
					return 1;
			}
			else // we are looking for any '0's
			{
				if ( ! BM_TEST(map, check) ) // have we found any '0's?
					return 1;
			}
		}
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef TEST_APP_BITMAPFS
/* Output should be, e.g.:

ALLOCATED AN ARRAY OF 1000000003 BITS
SETTING ARRAY TO ALL '1'S
     bit           12 is T/T  Ok  byte:         0 is 0xff
SETTING ARRAY TO ALL '0'S
     bit           12 is F/F  Ok  byte:         0 is 0x00
ASSIGN 11111 AT 1000000002
     bit   1000000002 is T/T  Ok  byte: 125000000 is 0x04
ASSIGN 0 AT 1000000002
     bit   1000000002 is F/F  Ok  byte: 125000000 is 0x00
EVERY EVEN BIT 0, EVERY ODD BIT 1...
     bit         3456 is F/F  Ok  byte:       432 is 0xaa
     bit         4567 is T/T  Ok  byte:       570 is 0xaa
  checking for any '1's: T/T  Ok
  checking for any '0's: T/T  Ok
  scan test...                OK
EVERY ODD BIT 0, EVERY EVEN BIT 1...
     bit         3456 is T/T  Ok  byte:       432 is 0x55
     bit         4567 is F/F  Ok  byte:       570 is 0x55
  checking for any '1's: T/T  Ok
  checking for any '0's: T/T  Ok
  scan test...                OK
SETTING ARRAY TO ALL '1'S
  checking for any '1's: T/T  Ok
  checking for any '0's: F/F  Ok
SETTING ARRAY TO ALL '0'S
  checking for any '1's: F/F  Ok
  checking for any '0's: T/T  Ok
SETTING BIT 1000000001 TO '1'
  checking for any '1's: T/T  Ok
  checking for any '0's: T/T  Ok
SETTING ARRAY TO ALL '1'S
SETTING BIT 1000000001 TO '0'
  checking for any '1's: T/T  Ok
  checking for any '0's: T/T  Ok
*/

#define HUGE (4294967294U) // 2^32-2 ... last two bits are unusable in this test app (for-loop tests),
// but should be ok up to 2^32 due to last byte being mapped correctly.

#define RESULTS(shouldbe)  result?'T':'F', shouldbe, (result?'T':'F')==shouldbe?"Ok": "MISMATCH!"
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

int main(int argc, char *argv[])
{
	int ii;
	unsigned char *map;
	int value;
	int index;
	int result;
	bool ok;

	map = BM_ALLOC(HUGE);
	if ( map == NULL )
	{
		fprintf(stderr, "CANNOT ALLOCATE MEMORY FOR BIT MAP OF %u BITS\n", HUGE);
		goto Error;
	}
	else
	{
		printf("ALLOCATED AN ARRAY OF %u BITS\n", HUGE);
	}

	printf("SETTING ARRAY TO ALL '1'S\n");
	BM_ALL(map, 1, HUGE);
	result = BM_TEST(map, 12)                ; printf("     bit %12u is %c/%c  %s  byte:%10u is 0x%02x\n", 12, RESULTS('T'), 0, map[0]);

	printf("SETTING ARRAY TO ALL '0'S\n");
	BM_ALL(map, 0, HUGE);
	result = BM_TEST(map, 12)                ; printf("     bit %12u is %c/%c  %s  byte:%10u is 0x%02x\n", 12, RESULTS('F'), 0, map[0]);

	// Assign value to the bitmap map at position index:
	value = 11111;
	index = HUGE - 1;  // the bitmap is indexed like a C array.. 0 to size-1
	printf("ASSIGN %d AT %u\n", value, index);
	BM_ASSIGN(map, value, index);
	result = BM_TEST(map, index)             ; printf("     bit %12u is %c/%c  %s  byte:%10u is 0x%02x\n", index, RESULTS('T'), index / 8, map[index / 8]);

	value = 0;
	printf("ASSIGN %d AT %u\n", value, index);
	BM_ASSIGN(map, value, index);
	result = BM_TEST(map, index)             ; printf("     bit %12u is %c/%c  %s  byte:%10u is 0x%02x\n", index, RESULTS('F'), index / 8, map[index / 8]);

	printf("EVERY EVEN BIT 0, EVERY ODD BIT 1...\n");
	for ( ii = 0; ii < HUGE; ii += 2 )
	{
		BM_CLR(map, ii);
		BM_SET(map, ii + 1);
	}
	result = BM_TEST(map, 3456)              ; printf("     bit %12u is %c/%c  %s  byte:%10u is 0x%02x\n", 3456, RESULTS('F'),   3456 / 8, map[3456 / 8]);
	result = BM_TEST(map, 4567)              ; printf("     bit %12u is %c/%c  %s  byte:%10u is 0x%02x\n", 4567, RESULTS('T'),   4567 / 8, map[4567 / 8]);
	result = BM_ANY(map, 1, HUGE)             ; printf("  checking for any '1's: %c/%c  %s\n", RESULTS('T'));
	result = BM_ANY(map, 0, HUGE)             ; printf("  checking for any '0's: %c/%c  %s\n", RESULTS('T'));
	ok = TRUE;
	for ( ii = 0; ii < HUGE; ii += 2 )
	{
		if ( ii & 1 )
		{
			if ( ! BM_TEST(map, ii) )       { printf("  FAIL: scan test: bit %u should be 1  byte:%10u is 0x%02x\n", ii, ii / 8, map[ii / 8]); ok = FALSE; break; }
		}
		else
		{
			if (   BM_TEST(map, ii) )       { printf("  FAIL: scan test: bit %u should be 0  byte:%10u is 0x%02x\n", ii, ii / 8, map[ii / 8]); ok = FALSE; break; }
		}
	}
	if ( ok )                                 printf("  scan test...                OK\n");

	printf("EVERY ODD BIT 0, EVERY EVEN BIT 1...\n");
	for ( ii = 0; ii < HUGE; ii += 2 )
	{
		BM_SET(map, ii);
		BM_CLR(map, ii + 1);
	}
	result = BM_TEST(map, 3456)              ; printf("     bit %12u is %c/%c  %s  byte:%10u is 0x%02x\n", 3456, RESULTS('T'),   3456 / 8, map[3456 / 8]);
	result = BM_TEST(map, 4567)              ; printf("     bit %12u is %c/%c  %s  byte:%10u is 0x%02x\n", 4567, RESULTS('F'),   4567 / 8, map[4567 / 8]);
	result = BM_ANY(map, 1, HUGE)             ; printf("  checking for any '1's: %c/%c  %s\n", RESULTS('T'));
	result = BM_ANY(map, 0, HUGE)             ; printf("  checking for any '0's: %c/%c  %s\n", RESULTS('T'));
	ok = TRUE;
	for ( ii = 0; ii < HUGE; ii += 2 )
	{
		if ( ii & 1 )
		{
			if (   BM_TEST(map, ii) )       { printf("  FAIL: scan test: bit %u should be 0  byte:%10u is 0x%02x\n", ii, ii / 8, map[ii / 8]); ok = FALSE; break; }
		}
		else
		{
			if ( ! BM_TEST(map, ii) )       { printf("  FAIL: scan test: bit %u should be 1  byte:%10u is 0x%02x\n", ii, ii / 8, map[ii / 8]); ok = FALSE; break; }
		}
	}
	if ( ok )                                 printf("  scan test...                OK\n");

	printf("SETTING ARRAY TO ALL '1'S\n");
	BM_ALL(map, 1, HUGE);
	result = BM_ANY(map, 1, HUGE)             ; printf("  checking for any '1's: %c/%c  %s\n", RESULTS('T'));
	result = BM_ANY(map, 0, HUGE)             ; printf("  checking for any '0's: %c/%c  %s\n", RESULTS('F'));

	printf("SETTING ARRAY TO ALL '0'S\n");
	BM_ALL(map, 0, HUGE);
	result = BM_ANY(map, 1, HUGE)             ; printf("  checking for any '1's: %c/%c  %s\n", RESULTS('F'));
	result = BM_ANY(map, 0, HUGE)             ; printf("  checking for any '0's: %c/%c  %s\n", RESULTS('T'));

	printf("SETTING BIT %u TO '1'\n", HUGE - 2);
	BM_SET(map, HUGE - 2);
	result = BM_ANY(map, 1, HUGE)             ; printf("  checking for any '1's: %c/%c  %s\n", RESULTS('T'));
	result = BM_ANY(map, 0, HUGE)             ; printf("  checking for any '0's: %c/%c  %s\n", RESULTS('T'));

	printf("SETTING ARRAY TO ALL '1'S\n");
	BM_ALL(map, 1, HUGE);
	printf("SETTING BIT %u TO '0'\n", HUGE - 2);
	BM_CLR(map, HUGE - 2);
	result = BM_ANY(map, 1, HUGE)             ; printf("  checking for any '1's: %c/%c  %s\n", RESULTS('T'));
	result = BM_ANY(map, 0, HUGE)             ; printf("  checking for any '0's: %c/%c  %s\n", RESULTS('T'));

	// Freeing space requested: We can just use free(), or for closing the circle:
	BM_FREE(map);
	return 0;

Error:
	return 1;
}
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////

// end of bitmapfs.c
