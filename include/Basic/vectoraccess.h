#ifndef vectoraccess_h
#define vectoraccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2002
 Contents:	Access to STL vector class with extensions
 RCS:		$Id: vectoraccess.h,v 1.30 2012/01/16 14:08:53 cvskris Exp $
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
    inline		VectorAccess( unsigned int n ) : v_(n)	{}
    inline		VectorAccess( unsigned int n, const T& t )
				: v_(n,t)		{}
    inline		VectorAccess( const VectorAccess& v2 )
				: v_(v2.v_)		{}
    inline std::vector<T>&	 vec()				{ return v_; }
    inline const std::vector<T>& vec() const			{ return v_; }

    inline T&		operator[]( int idx )		{ return v_[idx]; }
    inline const T&	operator[]( int idx ) const
    			{ return (*const_cast<VectorAccess*>(this))[idx]; }
    inline unsigned int	size() const	{ return (unsigned int) v_.size(); }
    inline bool		setCapacity( int sz );
    			/*!<Allocates mem for sz, does not change size.*/
    inline void		getCapacity() const		{ return v_.capacity();}
    			/*!<\returns max size without reallocation.*/
    inline bool		setSize( int sz, T val );

    inline VectorAccess& operator =( const VectorAccess& v2 )
			{ v_ = v2.v_; return *this; }
    inline bool		push_back( const T& t );
    inline void		insert( int pos, const T& val )
					    { v_.insert(v_.begin() + pos,val); }
    inline void		erase()		    { v_.clear(); }
    inline void		erase( const T& t )
			{
			    for ( int idx=size()-1; idx!=-1; idx-- )
				{ if ( v_[idx] == t ) { remove(idx); return; } }
			}
    inline void		remove( unsigned int idx )
			{
			    if ( idx < size() )
				v_.erase( v_.begin() + idx );
			}
    inline void		remove( unsigned int i1, unsigned int i2 )
			{
			    if ( i1 == i2 ) { remove( i1 ); return; }
			    if ( i1 > i2 ) std::swap( i1, i2 );
			    const unsigned int sz = size();
			    if ( i1 >= sz ) return;

			    if ( i2 >= sz-1 ) i2 = sz-1;
			    v_.erase( v_.begin()+i1, v_.begin()+i2+1 );
			}
    inline void		swap( unsigned int i, unsigned int j )
			{ std::swap( v_[i], v_[j] ); }

    inline void		fillWith( const T& val )
			{
			    const int sz = size();
			    T* arr = sz ? &v_[0] : 0;
			    for ( int i=sz-1; i>=0; i--,arr++ )
				*arr = val;
			}

    void moveAfter( const T& t, const T& aft )
    {
	if ( t == aft || size() < 2 ) return;
	int tidx = -1; int aftidx = -1;
	for ( int idx=size()-1; idx!=-1; idx-- )
	{
	    if ( v_[idx] == t )
		{ tidx = idx; if ( aftidx != -1 ) break; }
	    if ( v_[idx] == aft )
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
	    if ( v_[idx] == t ) { tidx = idx; break; }
	for ( int idx=tidx; idx>0; idx-- )
	    swap( idx, idx-1 );
    }

protected:

    std::vector<T>	v_;

};


template<class T> inline
bool VectorAccess<T>::setCapacity( int sz )
{
    try { v_.reserve(sz); }
    catch ( std::bad_alloc )
    { return false; }
    catch ( std::length_error )
    { return false; }

    return true;
}


template<class T> inline
bool VectorAccess<T>::push_back( const T& t )
{
    try { v_.push_back(t); }
    catch ( std::bad_alloc )
    { return false; }

    return true;
}


template<class T> inline
bool VectorAccess<T>::setSize( int sz, T val )
{
    try { v_.resize(sz,val); }
    catch ( std::bad_alloc )
    { return false; }

    return true;
}
#endif
