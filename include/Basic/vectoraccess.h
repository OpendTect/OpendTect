#ifndef vectoraccess_h
#define vectoraccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2002
 Contents:	Access to STL vector class with extensions
 RCS:		$Id: vectoraccess.h,v 1.29 2011-04-20 12:25:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "general.h"
#include <algorithm>
#include <vector>
#include <stdexcept>

/*!\brief Simple vector-based container simplifying index-based work.

This class is an implementation detail of the 'sets.h' and 'sortedlist.h'
classes. Thus, this class is not meant to be used anywhere else in OpendTect!
Use TypeSet, ObjectSet or SortedList instead. If you need to have the
std::vector to pass to an external C++ object, use the TypeSet::vec() or
SortedList::vec().

NOTE: because this class is based directly upon the STL vector, we have a
problem for the bool type. In STL, they have made the vector<bool> implemented
in terms of the bit_vector. That really sucks because we cannot return a
reference to T! This is why there is a 'BoolTypeSet'.
 
 */

template <class T>
class VectorAccess
{
public:

    inline		VectorAccess()			{}
    inline		VectorAccess( unsigned int n ) : v(n)	{}
    inline		VectorAccess( unsigned int n, const T& t )
				: v(n,t)		{}
    inline		VectorAccess( const VectorAccess& v2 )
				: v(v2.v)		{}
    inline std::vector<T>&	 vec()				{ return v; }
    inline const std::vector<T>& vec() const			{ return v; }

    inline T&		operator[]( int idx )		{ return v[idx]; }
    inline const T&	operator[]( int idx ) const
    			{ return (*const_cast<VectorAccess*>(this))[idx]; }
    inline unsigned int	size() const	{ return (unsigned int) v.size(); }
    inline bool		setCapacity( int sz );
    			/*!<Allocates mem for sz, does not change size.*/
    inline void		getCapacity() const		{ return v.capacity(); }
    			/*!<\returns max size without reallocation.*/
    inline bool		setSize( int sz, T val );

    inline VectorAccess& operator =( const VectorAccess& v2 )
			{ v = v2.v; return *this; }
    inline bool		push_back( const T& t );
    inline void		insert( int pos, const T& val )
					    { v.insert(v.begin() + pos,val); }
    inline void		erase()
    			{ v.erase( v.begin(), v.end() ); }
    inline void		erase( const T& t )
			{
			    for ( int idx=size()-1; idx!=-1; idx-- )
				{ if ( v[idx] == t ) { remove(idx); return; } }
			}
    inline void		remove( unsigned int idx )
			{
			    if ( idx < size() )
				v.erase( v.begin() + idx );
			}
    inline void		remove( unsigned int i1, unsigned int i2 )
			{
			    if ( i1 == i2 ) { remove( i1 ); return; }
			    if ( i1 > i2 ) std::swap( i1, i2 );
			    const unsigned int sz = size();
			    if ( i1 >= sz ) return;

			    if ( i2 >= sz-1 ) i2 = sz-1;
			    v.erase( v.begin()+i1, v.begin()+i2+1 );
			}
    inline void		swap( unsigned int i, unsigned int j )
			{ std::swap( v[i], v[j] ); }

    inline void		fillWith( const T& val )
			{
			    const int sz = size();
			    T* arr = sz ? &v[0] : 0;
			    for ( int i=sz-1; i>=0; i--,arr++ )
				*arr = val;
			}

    void moveAfter( const T& t, const T& aft )
    {
	if ( t == aft || size() < 2 ) return;
	int tidx = -1; int aftidx = -1;
	for ( int idx=size()-1; idx!=-1; idx-- )
	{
	    if ( v[idx] == t )
		{ tidx = idx; if ( aftidx != -1 ) break; }
	    if ( v[idx] == aft )
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
	    if ( v[idx] == t ) { tidx = idx; break; }
	for ( int idx=tidx; idx>0; idx-- )
	    swap( idx, idx-1 );
    }

protected:

    std::vector<T>	v;

};


template<class T> inline
bool VectorAccess<T>::setCapacity( int sz )
{
    try { v.reserve(sz); }
    catch ( std::bad_alloc )
    { return false; }
    catch ( std::length_error )
    { return false; }

    return true;
}


template<class T> inline
bool VectorAccess<T>::push_back( const T& t )
{
    try { v.push_back(t); }
    catch ( std::bad_alloc )
    { return false; }

    return true;
}


template<class T> inline
bool VectorAccess<T>::setSize( int sz, T val )
{
    try { v.resize(sz,val); }
    catch ( std::bad_alloc )
    { return false; }

    return true;
}
#endif
