#ifndef Vector_H
#define Vector_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		31-7-1995
 Contents:	STL-like vector implementation
 RCS:		$Id: vectoraccess.h,v 1.5 2000-03-10 13:10:15 bert Exp $
________________________________________________________________________

-*/

#include <stdlib.h>
#include <general.h>


#ifndef __prog__
extern
#endif

	unsigned int defaultAllocBlockSize

#ifdef __prog__
						= 16
#endif
						    ;


template <class T>
class Vector
{
public:

Vector()	: sz(0), allocsz(0), elems(0),
		  allocblocksize(defaultAllocBlockSize)	{}

Vector( unsigned int n )
	: sz(0), allocsz(0), elems(0), allocblocksize(defaultAllocBlockSize)
{
    sz = allocsz = 0;
    if ( n ) elems = (T*)malloc(n*sizeof(T));
    if ( elems ) { sz = allocsz = n; }
}

Vector( unsigned int n, const T& t )
	: sz(0), allocsz(0), allocblocksize(defaultAllocBlockSize)
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
	*((unsigned int*)(&allocblocksize)) = v.allocblocksize;
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
    unsigned int size() const			{ return sz; }

void push_back(const T& t)
{
    if ( ++sz >= allocsz )
    {
	allocsz += allocblocksize;
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

void remove( unsigned int idx )
{
    if ( idx<0 || idx>=sz ) return;
    sz--;
    if ( idx<sz ) memcpy( elems+idx, elems+idx+1, (sz-idx)*sizeof(T) );
    if ( !sz ) erase();
    else if ( sz == allocsz-allocblocksize )
    {
	allocsz = sz;
	elems = (T*)realloc(elems,allocsz*sizeof(T));
    }
}

void remove( unsigned int i1, unsigned int i2 )
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
	unsigned int prevallocsz = allocsz;
	allocsz = sz % allocblocksize
		? ((sz/allocblocksize)+1)*allocblocksize
		: allocsz = sz;
	if ( allocsz != prevallocsz )
	    elems = (T*)realloc(elems,allocsz*sizeof(T));
    }
}

void swap( unsigned int i, unsigned int j )
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

    unsigned int	sz;
    unsigned int	allocsz;
    const unsigned int	allocblocksize;
    T*			elems;

};


#endif
