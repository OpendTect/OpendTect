#ifndef Vector_H
#define Vector_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		31-7-1995
 Contents:	STL-like vector implementation
 RCS:		$Id: vectoraccess.h,v 1.2 2000-02-25 10:29:13 bert Exp $
________________________________________________________________________

@$*/

#include <stdlib.h>
#include <general.h>


#define mAllocSize	1024


template <class T>
class Vector
{
public:

Vector()	: sz(0), allocsz(0), elems(0)	{}

Vector( int n )
	: sz(0), allocsz(0), elems(0)
{
    sz = allocsz = 0;
    if ( n ) elems = (T*)malloc(n*sizeof(T));
    if ( elems ) { sz = allocsz = n; }
}

Vector( int n, const T& t )
	: sz(0), allocsz(0)
{
    elems = (T*)malloc(n*sizeof(T));
    if ( elems )
    {
	sz = allocsz = n;
	for ( int idx=0; idx<n; idx++ )
	    elems[idx] = t;
    }
}

Vector( const Vector& v )
	: sz(0), allocsz(0), elems(0)
{
    *this = v;
}

Vector& operator =( const Vector& v )
{
    if ( &v != this )
    {
	if ( sz != v.sz )
	{
	    if ( elems ) free( elems );
	    elems = (T*)malloc(allocsz*sizeof(T));
	    if ( elems ) { sz = v.sz; allocsz = v.allocsz; }
	    else	 sz = allocsz = 0;
	}
	if ( elems ) memcpy( elems, v.elems, sz*sizeof(T) );
    }
    return *this;
}

~Vector()
{
    if ( elems ) free( elems );
}

    T&		operator[](int idx)		{ return elems[idx]; }
    const T&	operator[](int idx) const	{ return elems[idx]; }
    int		size() const			{ return sz; }

void push_back(const T& t)
{
    if ( ++sz >= allocsz )
    {
	allocsz += mAllocSize;
	if ( elems )	elems = (T*)realloc(elems,allocsz*sizeof(T));
	else		elems = (T*)malloc(allocsz*sizeof(T));
    }

    if ( elems ) elems[sz-1] = t;
    else	 allocsz = sz = 0;
}

void erase()
{
    mFREE(elems); allocsz = sz = 0;
}

void erase( const T& t )
{
    for ( int idx=0; idx<sz; idx++ )
	{ if ( *(elems+idx) == t ) { remove( idx ); return; } }
}

void remove( int idx )
{
    if ( idx<0 || idx>=sz ) return;
    sz--;
    if ( idx<sz ) memcpy( elems+idx, elems+idx+1, (sz-idx)*sizeof(T) );
    if ( !sz ) erase();
    else if ( sz == allocsz-mAllocSize )
    {
	allocsz = sz;
	elems = (T*)realloc(elems,allocsz*sizeof(T));
    }
}

void remove( int i1, int i2 )
{
    if ( i1 == i2 ) { remove( i1 ); return; }
    if ( i1 > i2 ) Swap( i1, i2 );
    if ( i1>=sz ) return;

    if ( i2>=sz-1 ) i2 = sz-1;
    else	    memcpy( elems+i1, elems+i2+1, (sz-i2)*sizeof(T) );

    sz -= i2 - i1 + 1;
    if ( !sz ) erase();
    else
    {
	int prevallocsz = allocsz;
	if ( sz % mAllocSize )  allocsz = ((sz/mAllocSize)+1) * mAllocSize;
	else			allocsz = sz;
	if ( allocsz != prevallocsz )
	    elems = (T*)realloc(elems,allocsz*sizeof(T));
    }
}

void swap( int i, int j )
{
    ::Swap( elems[i], elems[j] );
}

void moveAfter( const T& t, const T& aft )
{
    if ( t == aft || sz < 2 ) return;
    int tidx = -1; int aftidx = -1;
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( *(elems+idx) == t ) tidx = idx;
	if ( *(elems+idx) == aft ) aftidx = idx;
    }
    if ( tidx == -1 || aftidx == -1 || tidx == aftidx ) return;
    if ( aftidx > tidx )
	for ( int idx=tidx; idx<aftidx; idx++ )
	    swap( idx, idx+1 );
    else
	for ( int idx=tidx; idx>aftidx+1; idx-- )
	    swap( idx, idx-1 );
}

void moveToStart( const T& t )
{
    if ( sz < 2 ) return;
    int tidx = -1;
    for ( int idx=0; idx<sz; idx++ )
	if ( *(elems+idx) == t ) { tidx = idx; break; }
    for ( int idx=tidx; idx>0; idx-- )
	swap( idx, idx-1 );
}

private:

    int		sz;
    int		allocsz;
    T*		elems;

};


/*$-*/
#endif
