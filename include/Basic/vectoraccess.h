#ifndef Vector_H
#define Vector_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		31-7-1995
 Contents:	STL-like vector implementation
 RCS:		$Id: vectoraccess.h,v 1.6 2001-02-13 17:15:45 bert Exp $
________________________________________________________________________

-*/

#include <stdlib.h>
#include <general.h>

extern const unsigned char V_StartBlockIdx;
extern const unsigned char V_EndBlockIdx;
extern const unsigned int V_AllocSizes[];


/*!\brief variable array a bit like std vector.

As we need only a few things from the enormous std lib (and it's always hidden
in TypeSet and ObjectSet), this simple but fast vector implementation is used.
A nice feature is that the allocation blocks grow as the size becomes larger.

*/

template <class T>
class Vector
{
public:

		Vector()
		: szdiff(0), blockszidx(V_StartBlockIdx), allocsz(0), elems(0)
						{}

    T&		operator[](int idx)		{ return elems[idx]; }
    const T&	operator[](int idx) const	{ return elems[idx]; }
    inline unsigned int size() const		{ return allocsz-szdiff; }


Vector( unsigned int n )
{
    init( n );
}

Vector( unsigned int n, const T& t )
{
    init( n );
    if ( elems )
    {
	for ( register int idx=0; idx<n; idx++ )
	    elems[idx] = t;
    }
}

Vector( const Vector& v )
	: szdiff(0), allocsz(0), blockszidx(V_StartBlockIdx), elems(0)
{
    *this = v;
}

Vector& operator =( const Vector& v )
{
    if ( &v != this )
    {
	if ( szdiff != v.szdiff || allocsz != v.allocsz )
	{
	    if ( elems ) free( elems );
	    elems = (T*)malloc(v.allocsz*sizeof(T));
	    if ( !elems )
		erase();
	    else
	    {
		szdiff = v.szdiff; allocsz = v.allocsz;
		blockszidx = v.blockszidx;
	    }
	}
	if ( elems ) memcpy( elems, v.elems, sz*sizeof(T) );
    }
    return *this;
}

~Vector()
{
    if ( elems ) free( elems );
}

void push_back( const T& t )
{
    if ( szdiff != 0 )
	szdiff--;
    else
    {
	const unsigned int newsz = size() + 1;
	if ( blockszidx < V_EndBlockIdx-1 )
	    allocsz = V_AllocSizes[++blockszidx];
	else
	{
	    allocsz += V_AllocSizes[blockszidx];
	    if ( blockszidx != V_EndBlockIdx ) blockszidx++;
	}

	szdiff = (unsigned short)(allocsz - newsz);

	if ( elems )	elems = (T*)realloc(elems,allocsz*sizeof(T));
	else		elems = (T*)malloc(allocsz*sizeof(T));
    }

    if ( elems ) elems[ size()-1 ] = t;
    else	 erase();
}

void erase()
{
    if ( elems ) free( elems );
    elems = 0; allocsz = szdiff = 0;
    blockszidx = V_StartBlockIdx;
}

void erase( const T& t )
{
    for ( int idx=size()-1; idx!=-1; idx-- )
	{ if ( *(elems+idx) == t ) { remove( idx ); return; } }
}

void remove( unsigned int idx )
{
    if ( idx<0 || idx>=size() ) return;
    szdiff++;

    if ( idx < size() )
	memcpy( elems+idx, elems+idx+1, (size()-idx)*sizeof(T) );

    if ( !blockszidx )
	erase();
    else if ( szdiff == V_AllocSizes[blockszidx-1] )
    {
	szdiff = 0;
	if ( blockszidx < V_EndBlockIdx
	  || allocsz == 2 * V_AllocSizes[V_EndBlockIdx] )
	    blockszidx--;
	allocsz -= V_AllocSizes[blockszidx];
	elems = (T*)realloc(elems,allocsz*sizeof(T));
    }
}

void remove( unsigned int i1, unsigned int i2 )
{
    if ( i1 == i2 ) { remove( i1 ); return; }
    if ( i1 > i2 ) Swap( i1, i2 );
    unsigned int sz = size();
    if ( i1 >= sz ) return;

    if ( i2 >= sz-1 ) i2 = sz-1;
    else	    memcpy( elems+i1, elems+i2+1, (sz-i2)*sizeof(T) );

    sz -= i2 - i1 + 1;
    if ( !sz )
	erase();
    else
    {
	const unsigned int prevallocsz = allocsz;
	setAllocSizeFor( sz );
	szdiff = allocsz - sz;
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
    if ( t == aft || size() < 2 ) return;
    int tidx = -1; int aftidx = -1;
    for ( int idx=size()-1; idx!=-1; idx-- )
    {
	if ( *(elems+idx) == t )
	    { tidx = idx; if ( aftidx != -1 ) break; }
	if ( *(elems+idx) == aft )
	    { aftidx = idx; if ( tidx != -1 ) break; }
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
    if ( size() < 2 ) return;
    int tidx = -1;
    for ( int idx=size()-1; idx!=-1; idx-- )
	if ( *(elems+idx) == t ) { tidx = idx; break; }
    for ( int idx=tidx; idx>0; idx-- )
	swap( idx, idx-1 );
}

private:

    unsigned char	blockszidx;
    unsigned short	szdiff;
    unsigned int	allocsz;
    T*			elems;

void init( unsigned int sz )
{
    elems = 0;
    blockszidx = V_StartBlockIdx;

    if ( sz )
    {
	setAllocSizeFor( sz );
	elems = (T*)malloc(allocsz*sizeof(T));
    }

    if ( !elems )
	erase();
    else
	szdiff = allocsz - sz;
}

void setAllocSizeFor( int sz )
{
    blockszidx = V_EndBlockIdx;
    if ( sz > V_AllocSizes[blockszidx] )
    {
	unsigned int nrblocks = sz / V_AllocSizes[blockszidx];
	if ( V_AllocSizes[blockszidx] * nrblocks != sz )
	    nrblocks++;
	allocsz = nrblocks * V_AllocSizes[blockszidx];
    }
    else
    {
	blockszidx--;
	do {

	    blockszidx--; allocsz = V_AllocSizes[blockszidx];

	} while ( allocsz >= sz && blockszidx );

	blockszidx++; allocsz = V_AllocSizes[blockszidx];
    }
}

};


#endif
