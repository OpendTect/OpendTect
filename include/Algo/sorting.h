#ifndef sorting_h
#define sorting_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		19-4-2000
 Contents:	Array sorting
 RCS:		$Id: sorting.h,v 1.7 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

#ifndef Vector_H
#include <Vector.h>
#endif


#define mDoSort(extra_var,extra_action) \
{ \
    T tmp; extra_var; \
    for ( int d=sz/2; d>0; d=d/2 ) \
	for ( int i=d; i<sz; i++ ) \
	    for ( int j=i-d; j>=0 && arr[j]>arr[j+d]; j-=d ) \
	    { \
		tmp = arr[j]; arr[j] = arr[j+d]; arr[j+d] = tmp; \
		extra_action; \
	    } \
}

/*!> sort quickly (algorithm taken from xv). */
template <class T>
inline void sort_array( T* arr, int sz )
mDoSort(,)

/*!> sort and remember where it was before sorting. */
template <class T, class IT>
inline void sort_coupled( T* arr, IT* idxs, int sz )
mDoSort(IT itmp,itmp = idxs[j]; idxs[j] = idxs[j+d]; idxs[j+d] = itmp)

#undef mDoSort
#define mDoSort(extra_var,extra_action) \
{ \
    extra_var; \
    for ( int d=sz/2; d>0; d=d/2 ) \
	for ( int i=d; i<sz; i++ ) \
	    for ( int j=i-d; j>=0 && arr[j]>arr[j+d]; j-=d ) \
	    { \
		Swap( arr[j], arr[j+d] ); \
		extra_action; \
	    } \
}

/*!> sort quickly (algorithm taken from xv). */
template <class T>
inline void sort_idxabl( T& arr, int sz )
mDoSort(,)

/*!> sort and remember where it was before sorting. */
template <class T, class IT>
inline void sort_idxabl_coupled( T& arr, IT* idxs, int sz )
mDoSort(IT itmp,itmp = idxs[j]; idxs[j] = idxs[j+d]; idxs[j+d] = itmp)
#undef mDoSort

#define NSMALL 7
#define FM 7875
#define FA 211
#define FC 1663
#define NSTACK 50

template <class T> inline
void partSort( T* arr, int istart, int istop,
		      int* jstart, int* jstop )
{
    int ipivot, ileft, iright;
    T pivotval, tmp;
    static long int seed = 0L;

    seed = (seed * FA + FC) % FM;
    ipivot = (int)(istart + (istop-istart) * (float)seed / (float)FM + .5);
    if ( ipivot < istart ) ipivot = istart;
    if ( ipivot > istop ) ipivot = istop;
    pivotval = arr[ipivot];

    for ( ileft=istart, iright=istop; ; )
    {
	while ( arr[ileft] <=pivotval && ileft<istop )   ileft++;
	while ( arr[iright]>=pivotval && iright>istart ) iright--;
	if ( ileft < iright )
	{
	    tmp = arr[ileft];
	    arr[ileft++] = arr[iright];
	    arr[iright--] = tmp;
	}
	else break;
    }

    if ( ileft < ipivot )
    {
	tmp = arr[ileft];
	arr[ileft++] = arr[ipivot];
	arr[ipivot] = tmp;
    }
    else if ( ipivot < iright )
    {
	tmp = arr[iright];
	arr[iright--] = arr[ipivot];
	arr[ipivot] = tmp;
    }

    *jstart = iright;
    *jstop = ileft;
}


template <class T> inline
void insertionSort( T* arr, int istart, int istop )
{
    int i, j;
    T arr_i;

    for ( i=istart+1; i<=istop; i++ )
    {
	for ( arr_i=arr[i],j=i; j>istart && arr[j-1]>arr_i; j-- )
	    arr[j] = arr[j-1];
	arr[j] = arr_i;
    }
}


template <class T> inline
void sortFor( T* arr, int sz, int itarget )
/*!> sorts the array until the 'itarget' element has exactly the right
value. The rest of the array must be considered unsorted after the operation,
although it will generally be better sorted. */
{
    int j, k, p = 0, q = sz-1;

    while( q - p > NSMALL )
    {
        partSort( arr, p, q, &j, &k );

        if ( itarget <= j )             q = j;
        else if ( itarget >= k )        p = k;
        else                            return;
    }

    insertionSort( arr, p, q );
}


template <class T> inline
void quickSort( T* arr, int sz )
/*!> is quicker than sort_array for arrays larger than about 100 values. */
{
    int pstack[NSTACK], qstack[NSTACK], j, k, p, q, top=0;

    pstack[top] = 0;
    qstack[top++] = sz - 1;

    while( top )
    {
	p = pstack[--top];
	q = qstack[top];

	while( q - p > NSMALL )
	{
	    partSort( arr, p, q, &j, &k );

	    if ( j-p < q-k )
	    {
		pstack[top] = k;
		qstack[top++] = q;
		q = j;
	    }
	    else
	    {
		pstack[top] = p;
		qstack[top++] = j;
		p = k;
	    }
	}
	insertionSort( arr, p, q );
    }
}


template <class T, class IT> inline
void partSort( T* arr, IT* iarr, int istart, int istop, int* jstart, int* jstop)
{
    int ipivot, ileft, iright;
    T pivotval, tmp;
    IT itmp;
    static long int seed = 0L;

    seed = (seed * FA + FC) % FM;
    ipivot = (int)(istart + (istop-istart) * (float)seed / (float)FM);
    if ( ipivot < istart ) ipivot = istart;
    if ( ipivot > istop ) ipivot = istop;
    pivotval = arr[ipivot];

    for ( ileft=istart, iright=istop; ; )
    {
	while ( arr[ileft] <=pivotval && ileft<istop )   ileft++;
	while ( arr[iright]>=pivotval && iright>istart ) iright--;
	if ( ileft < iright )
	{
	    itmp = iarr[ileft];
	    tmp = arr[ileft];

	    iarr[ileft] = iarr[iright];
	    arr[ileft++] = arr[iright];

	    iarr[iright] = itmp;
	    arr[iright--] = tmp;
	}
	else break;
    }

    if ( ileft < ipivot )
    {
	itmp = iarr[ileft];
	tmp = arr[ileft];

	iarr[ileft] = iarr[ipivot];
	arr[ileft++] = arr[ipivot];

	iarr[ipivot] = itmp;
	arr[ipivot] = tmp;
    }
    else if ( ipivot < iright )
    {
	itmp = iarr[iright];
	tmp = arr[iright];

	iarr[iright] = iarr[ipivot];
	arr[iright--] = arr[ipivot];

	iarr[ipivot] = itmp;
	arr[ipivot] = tmp;
    }

    *jstart = iright;
    *jstop = ileft;
}


template <class T, class IT> inline
void insertionSort( T* arr, IT* iarr, int istart, int istop )
{
    int i, j;
    T arr_i;
    IT iarr_i;

    for ( i=istart+1; i<=istop; i++ )
    {
	for ( iarr_i=iarr[i],arr_i=arr[i],j=i; j>istart && arr[j-1]>arr_i; j-- )
	{
	    arr[j] = arr[j-1];
	    iarr[j] = iarr[j-1];
	}

	arr[j] = arr_i;
	iarr[j] = iarr_i;
    }
}

template <class T, class IT>
void sortFor( T* arr, IT* iarr, int sz, int itarget )
{
    int j, k, p = 0, q = sz-1;

    while( q - p > NSMALL )
    {
        partSort( arr, iarr, p, q, &j, &k );

        if ( itarget <= j )             q = j;
        else if ( itarget >= k )        p = k;
        else                            return;
    }

    insertionSort( arr, iarr, p, q );
}


template <class T, class IT> inline
void quickSort( T* arr, IT* iarr, int sz )
{
    int pstack[NSTACK], qstack[NSTACK], j, k, p, q, top=0;

    pstack[top] = 0;
    qstack[top++] = sz - 1;

    while( top )
    {
	p = pstack[--top];
	q = qstack[top];

	while( q - p > NSMALL )
	{
	    partSort( arr, iarr, p, q, &j, &k );

	    if ( j-p < q-k )
	    {
		pstack[top] = k;
		qstack[top++] = q;
		q = j;
	    }
	    else
	    {
		pstack[top] = p;
		qstack[top++] = j;
		p = k;
	    }
	}
	insertionSort( arr, iarr, p, q );
    }
}

#undef NSMALL
#undef FM
#undef FA
#undef FC 
#undef NSTACK

#endif
