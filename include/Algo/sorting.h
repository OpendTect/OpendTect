#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		19-4-2000
 Contents:	Array sorting
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"
#include "ptrman.h"
#include "thread.h"
#include "paralleltask.h"


#define mDoSort(extra_var,extra_action,sztype) \
{ \
    T tmp; extra_var; \
    for ( sztype d=sz/2; d>0; d=d/2 ) \
	for ( sztype i=d; i<sz; i++ ) \
	    for ( sztype j=i-d; j>=0 && arr[j]>arr[j+d]; j-=d ) \
	    { \
		tmp = arr[j]; arr[j] = arr[j+d]; arr[j+d] = tmp; \
		extra_action; \
	    } \
}

/*!> sort quickly (algorithm taken from xv). */
template <class T,class I>
inline void sort_array( T* arr, I sz )
mDoSort(,,I)

/*!> sort and remember where it was before sorting. */
template <class T, class IT,class I>
inline void sort_coupled( T* arr, IT* idxs, I sz )
mDoSort(IT itmp,itmp = idxs[j]; idxs[j] = idxs[j+d]; idxs[j+d] = itmp,I)

#undef mDoSort
#define mDoSort(extra_var,extra_action,sztype) \
{ \
    extra_var; \
    for ( sztype d=sz/2; d>0; d=d/2 ) \
	for ( sztype i=d; i<sz; i++ ) \
	    for ( sztype j=i-d; j>=0 && arr[j]>arr[j+d]; j-=d ) \
	    { \
		std::swap( arr[j], arr[j+d] ); \
		extra_action; \
	    } \
}

/*!> sort quickly (algorithm taken from xv). */
template <class T>
inline void sort_idxabl( T& arr, int sz )
mDoSort(,,int)

/*!> sort and remember where it was before sorting. */
template <class T, class IT>
inline void sort_idxabl_coupled( T& arr, IT* idxs, int sz )
mDoSort(IT itmp,itmp = idxs[j]; idxs[j] = idxs[j+d]; idxs[j+d] = itmp,int)
#undef mDoSort


/*!> Sorting for data with many duplicates. */
template <class T,class I>
inline bool duplicate_sort( T* arr, I sz, int maxnrvals )
{
    TypeSet<T> vals;
    TypeSet<int> count;
    for ( I idx=0; idx<sz; ++idx )
    {
	const int vidx = vals.indexOf( arr[idx] );
	if ( vidx<0 )
	{
	    if ( vals.size()>maxnrvals )
	    {
		return false;
	    }

	    count += 1;
	    vals += arr[idx];
	}
	else
	    count[vidx] += 1;
    }

    const int vsize = mCast(int,vals.size());
    TypeSet<int> idxs;
    for ( int idx=0; idx<vsize; idx++ )
	idxs += idx;
    sort_coupled( vals.arr(), idxs.arr(), vsize );

    I index = -1;
    for ( int idx=0; idx<vsize; ++idx )
    {
	for ( int idy=count[idxs[idx]]-1; idy>=0; --idy )
	    arr[++index] = vals[idx];
    }

    return true;
}


/*!
\brief Sorting in parallel. Code is still experimental.

  The basic principle is:
  1. Divide samples into subsets.
  2. Sort subsets in parallel.
  3. Merge pairs of subsets iteratively until there are only one subset left.
*/

template <class T>
mClass(Algo) ParallelSorter : public ParallelTask
{ mODTextTranslationClass(ParallelSorter);
public:

    typedef od_int64		idx_type;
    typedef idx_type		size_type;

				ParallelSorter(T* vals,size_type);
				ParallelSorter(T* vals,idx_type*,size_type);
protected:
    od_int64			nrIterations() const { return nrvals_; }

    int				minThreadSize() const { return 10000; }
    bool			doPrepare(int);
    bool			doFinish(bool);
    bool			doWork(od_int64,od_int64,int);
    static bool			mergeLists(const T* vals, T* res,
					   idx_type start0,idx_type start1,
					   idx_type start2,idx_type stop,
					   size_type& totalsz );
    od_int64			nrDone() const { return totalnr_; }

    T*				vals_;
    ArrPtrMan<T>		tmpbuffer_;

    idx_type*			idxs_;
    T*				curvals_;
    T*				buf_;

    const size_type		nrvals_;
    size_type			totalnr_;

    Threads::ConditionVar	condvar_;
    TypeSet<idx_type>		starts_;
    TypeSet<idx_type>		newstarts_;

    Threads::Barrier		barrier_;
};


#define NSMALL 7
#define FM 7875
#define FA 211
#define FC 1663
#define NSTACK 50

mExtern(Algo) Threads::Atomic<int> partsortglobalseed;

inline float getPartSortSeed()
{
    const int localseed = (partsortglobalseed * FA + FC) % FM;

    //This is not really atomic, so a MT environment may alter the
    //global seed, but who cares as it is a seed, and should be
    //a random number

    partsortglobalseed = localseed;

    return (float) localseed;
}


template <class T,class I> inline
void partSort( T* arr, I istart, I istop,
		      I* jstart, I* jstop )
{
    I ipivot, ileft, iright;
    T pivotval, tmp;

    const float localseed = getPartSortSeed();

    ipivot = istart + (istop-istart) * (float)localseed / (float)FM + .5;
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


template <class T, class I> inline
void insertionSort( T* arr, I istart, I istop )
{
    I i, j;
    T arr_i;

    for ( i=istart+1; i<=istop; i++ )
    {
	for ( arr_i=arr[i],j=i; j>istart && arr[j-1]>arr_i; j-- )
	    arr[j] = arr[j-1];
	arr[j] = arr_i;
    }
}


template <class T,class I> inline
void sortFor( T* arr, I sz, I itarget )
/*!> sorts the array until the 'itarget' element has exactly the right
value. The rest of the array must be considered unsorted after the operation,
although it will generally be better sorted. */
{
    I j, k, p = 0, q = sz-1;

    while( q - p > NSMALL )
    {
        partSort( arr, p, q, &j, &k );

        if ( itarget <= j )             q = j;
        else if ( itarget >= k )        p = k;
        else                            return;
    }

    insertionSort( arr, p, q );
}


template <class T,class I> inline
void quickSort( T* arr, I sz )
/*!> is quicker than sort_array for arrays larger than about 100 values. */
{
    I pstack[NSTACK], qstack[NSTACK], j, k, p, q, top=0;

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
void partSort( T* arr, IT* iarr, od_int64 istart, od_int64 istop,
	       od_int64* jstart, od_int64* jstop)
{
    od_int64 ipivot, ileft, iright;
    T pivotval, tmp;
    IT itmp;

    const float localseed = getPartSortSeed();

    ipivot = istart + (istop-istart) * (float)localseed / (float)FM;
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
void insertionSort( T* arr, IT* iarr, od_int64 istart, od_int64 istop )
{
    od_int64 i, j;
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
void sortFor( T* arr, IT* iarr, od_int64 sz, od_int64 itarget )
{
    od_int64 j, k, p = 0, q = sz-1;

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
void quickSort( T* arr, IT* iarr, od_int64 sz )
{
    od_int64 pstack[NSTACK], qstack[NSTACK], j, k, p, q, top=0;

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
ParallelSorter<T>::ParallelSorter( T* vals, size_type sz)
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
ParallelSorter<T>::ParallelSorter( T* vals, idx_type* idxs, size_type sz )
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

    od_int64 nrmerges = -1;
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
	OD::memCopy( vals_, curvals_, nrvals_*sizeof(T) );

    return true;
}


template <class T> inline
bool ParallelSorter<T>::doWork( od_int64 start, od_int64 stop, int thread )
{
    const od_int64 threadsize = stop-start+1;
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

	const idx_type curstart0 = starts_[0]; starts_.removeSingle( 0 );
	const idx_type curstart1 = starts_[0]; starts_.removeSingle( 0 );
	idx_type curstart2;
	if ( starts_.size()==1 )
	{
	    curstart2 = starts_[0];
	    starts_.removeSingle( 0 );
	}
	else
	    curstart2 = -1;

	const idx_type curstop = (starts_.size() ? starts_[0] : nrvals_)-1;
	newstarts_ += curstart0;
	barrier_.mutex().unLock();

	size_type cursize;
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
				    idx_type start0, idx_type start1,
				    idx_type start2, idx_type stop,
				    size_type& totalsz )
{
    const size_type sz0 = start1-start0;
    const size_type sz1 = start2==-1 ? stop-start1+1 : start2-start1;
    const size_type sz2 = start2==-1 ? 0 : stop-start2+1;
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
