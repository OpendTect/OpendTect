#ifndef sorting_h
#define sorting_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		19-4-2000
 Contents:	Array sorting
 RCS:		$Id: sorting.h,v 1.17 2012-08-03 13:00:05 cvskris Exp $
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"
#include "general.h"
#include "ptrman.h"
#include "task.h"
#include "thread.h"


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


/*!Sorting in parallel. Code is still experimental. The basic principle is:
1. Divide samples into subsets.
2. Sort subsets in parallel
3. Merge pairs of subsets iteratively until there are only one subset left.
*/

template <class T>
mClass(Algo) ParallelSorter : public ParallelTask
{
public:
				ParallelSorter(T* vals, int sz);
				ParallelSorter(T* vals, int* idxs, int sz);
protected:
    od_int64			nrIterations() const { return nrvals_; }

    int				minThreadSize() const { return 10000; }
    bool			doPrepare(int);
    bool			doFinish(bool);
    bool			doWork(od_int64,od_int64,int);
    static bool			mergeLists(const T* vals, T* res,
	    				   int start0,int start1,int start2,
	    				   int stop, int& totalsz );
    od_int64			nrDone() const { return totalnr_; }

    T*				vals_;
    ArrPtrMan<T>		tmpbuffer_;

    int*			idxs_;
    T*				curvals_;
    T*				buf_;

    const int			nrvals_;
    int				totalnr_;

    Threads::ConditionVar	condvar_;
    TypeSet<int>		starts_;
    TypeSet<int>		newstarts_;

    Threads::Barrier		barrier_;
};


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


//ParallelSort implementation
template <class T> inline
ParallelSorter<T>::ParallelSorter(T* vals, int sz)
    : vals_( vals )
    , nrvals_( sz )
    , tmpbuffer_( 0 )
    , barrier_( -1, false )
    , totalnr_(0)		     
    , idxs_( 0 )
{
    mTryAlloc( tmpbuffer_, T[sz] );
}


template <class T> inline
ParallelSorter<T>::ParallelSorter(T* vals, int* idxs, int sz)
    : vals_( vals )
    , nrvals_( sz )
    , tmpbuffer_( 0 )
    , totalnr_(0)		     
    , barrier_( -1, false )
    , idxs_( idxs )
{
    mTryAlloc( tmpbuffer_, T[sz] );
}


template <class T> inline
bool ParallelSorter<T>::doPrepare( int nrthreads )
{
    if ( !tmpbuffer_ )
	return false;

    barrier_.setNrThreads( nrthreads );

    starts_.erase();
    newstarts_.erase();

    int nrmerges = -1;
    while ( nrthreads )
    {
	nrmerges++;
	nrthreads>>=1;
    }

    totalnr_ = (1+nrmerges)*nrvals_;
    return true;
}


template <class T> inline
bool ParallelSorter<T>::doFinish( bool success )
{
    if ( !success )
	return false;

    if ( curvals_!=vals_ )
	memcpy( vals_, curvals_, nrvals_*sizeof(T) );

    return true;
}


template <class T> inline
bool ParallelSorter<T>::doWork( od_int64 start, od_int64 stop, int thread )
{
    const int threadsize = stop-start+1;
    if ( threadsize<100 )
    {
	if ( idxs_ )
	    sort_coupled( vals_+start, idxs_+start, threadsize );
	else
	    sort_array( vals_+start, threadsize );
    }
    else
    {
	if ( idxs_ )
	    quickSort( vals_+start, idxs_+start, threadsize );
	else
	    quickSort( vals_+start, threadsize );
    }

    if ( !shouldContinue() )
	return false;

    addToNrDone( threadsize );

    barrier_.mutex().lock();
    newstarts_ += start;
    barrier_.mutex().unLock();

    while ( true )
    {
	if ( barrier_.waitForAll(false) )
	{
	    if ( curvals_==vals_ )
	    {
		curvals_ = tmpbuffer_;
		buf_ = vals_;
	    }
	    else
	    {
		buf_ = tmpbuffer_;
		curvals_ = vals_;
	    }

	    starts_ = newstarts_;
	    barrier_.setNrThreads( starts_.size()/2 );
	    barrier_.releaseAllNoLock();
	}

	if ( thread>=barrier_.nrThreads() )
	{
	    barrier_.mutex().unLock();
	    //I'm not needed any longer
	    break;
	}

	const int curstart0 = starts_[0]; starts_.remove( 0 );
	const int curstart1 = starts_[0]; starts_.remove( 0 );
	int curstart2;
	if ( starts_.size()==1 )
	{
	    curstart2 = starts_[0];
	    starts_.remove( 0 );
	}
	else
	    curstart2 = -1;

	const int curstop = (starts_.size() ? starts_[0] : nrvals_)-1;
	newstarts_ += curstart0;
	barrier_.mutex().unLock();

	int cursize;
	if ( !mergeLists( curvals_, buf_, 
		    curstart0, curstart1, curstart2, curstop, cursize) )
	    return false;

	if ( !shouldContinue() )
	    return false;

	addToNrDone( cursize );
    }

    return true;
}


template <class T> inline
bool ParallelSorter<T>::mergeLists( const T* valptr, T* result,
				    int start0, int start1, int start2,
				    int stop, int& totalsz )
{
    const int sz0 = start1-start0;
    const int sz1 = start2==-1 ? stop-start1+1 : start2-start1;
    const int sz2 = start2==-1 ? 0 : stop-start2+1;
    totalsz = sz0+sz1+sz2;

    const T* ptr0 = valptr + start0;
    const T* stopptr0 = ptr0+sz0;
    const T* ptr1 = valptr + start1;
    const T* stopptr1 = ptr1+sz1;
    const T* ptr2 = start2==-1 ? 0 : valptr + start2;
    const T* stopptr2 = ptr2+sz2;

    while ( true )
    {
	if ( ptr0 && (!ptr1 || *ptr0<*ptr1) && (!ptr2 || *ptr0<*ptr2 ) )
	{
	    (*result++) = (*ptr0++);
	    if ( ptr0==stopptr0 )
		ptr0 = 0;
	}
	else if ( ptr1 && ( !ptr2 || *ptr1<*ptr2 ) )
	{
	    (*result++) = (*ptr1++);
	    if ( ptr1==stopptr1 )
		ptr1 = 0;
	}
	else if ( ptr2 )
	{
	    (*result++) = (*ptr2++);
	    if ( ptr2==stopptr2 )
		ptr2 = 0;
	}
	else
	    break;
    }

    return true;
}



#endif

