#ifndef sorting_h
#define sorting_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		19-4-2000
 Contents:	Array sorting: heap sort and sort for target index
 RCS:		$Id: sorting.h,v 1.2 2000-04-20 15:51:13 bert Exp $
________________________________________________________________________

quickSort is quicker than sort_array for arrays larger than about 100 values.

sortFor sorts the array until the 'itarget' element has exactly the right
value. The rest of the array must be considered unsorted after the operation,
although it will generally be better sorted.

-*/

#include <gendefs.h>


#ifdef __cpp__
extern "C" {
#endif

void quickSort(float*,int sz);
void sortFor(float*,int sz,int itarget);

#ifdef __cpp__
}
#endif


#endif
