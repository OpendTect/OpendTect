#ifndef vectoraccess_h
#define vectoraccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2002
 Contents:	Access to STL vector class with extensions
 RCS:		$Id$
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

template <class T,class I>
class VectorAccess
{
public:

    inline		VectorAccess()			{}
    inline		VectorAccess( I n ) : v_(n)	{}
    inline		VectorAccess( I n, const T& t )
				: v_(n,t)		{}
    inline		VectorAccess( const VectorAccess& v2 )
				: v_(v2.v_)		{}
    inline std::vector<T>&	 vec()				{ return v_; }
    inline const std::vector<T>& vec() const			{ return v_; }

    inline T&		operator[]( I idx )
			{ return v_[(typename std::vector<T>::size_type)idx]; }
    inline const T&	operator[]( I idx ) const
    			{ return (*const_cast<VectorAccess*>(this))[idx]; }
    inline I		size() const	{ return (I) v_.size(); }
    inline bool		setCapacity( I sz );
    			/*!<Allocates mem for sz, does not change size.*/
    inline void		getCapacity() const		{ return v_.capacity();}
    			/*!<\returns max size without reallocation.*/
    inline bool		setSize( I sz, T val );

    inline VectorAccess& operator =( const VectorAccess& v2 )
			{ v_ = v2.v_; return *this; }
    inline bool		push_back( const T& t );
    inline void		insert( I pos, const T& val )
					    { v_.insert(v_.begin() + pos,val); }
    inline void		erase()		    { v_.clear(); }
    inline void		erase( const T& t )
			{
			    for ( I idx=size()-1; idx!=-1; idx-- )
				{ if ( v_[idx] == t ) { remove(idx); return; } }
			}
    inline void		remove( I idx )
			{
			    if ( idx < size() )
				v_.erase( v_.begin() + idx );
			}
    inline void		remove( I i1, I i2 )
			{
			    if ( i1 == i2 ) { remove( i1 ); return; }
			    if ( i1 > i2 ) std::swap( i1, i2 );
			    const I sz = size();
			    if ( i1 >= sz ) return;

			    if ( i2 >= sz-1 ) i2 = sz-1;
			    v_.erase( v_.begin()+i1, v_.begin()+i2+1 );
			}
    inline void		swap( I i, I j )
			{ std::swap( v_[i], v_[j] ); }

    inline void		fillWith( const T& val )
			{
			    const I sz = size();
			    T* arr = sz ? &v_[0] : 0;
			    for ( I i=sz-1; i>=0; i--,arr++ )
				*arr = val;
			}

    void moveAfter( const T& t, const T& aft )
    {
	if ( t == aft || size() < 2 ) return;
	I tidx = -1; I aftidx = -1;
	for ( I idx=size()-1; idx!=-1; idx-- )
	{
	    if ( v_[idx] == t )
		{ tidx = idx; if ( aftidx != -1 ) break; }
	    if ( v_[idx] == aft )
		{ aftidx = idx; if ( tidx != -1 ) break; }
	}
	if ( tidx == -1 || aftidx == -1 || tidx == aftidx ) return;
	if ( aftidx > tidx )
	    for ( I idx=tidx; idx<aftidx; idx++ )
		swap( idx, idx+1 );
	else
	    for ( I idx=tidx; idx>aftidx+1; idx-- )
		swap( idx, idx-1 );
    }

    void moveToStart( const T& t )
    {
	if ( size() < 2 ) return;
	I tidx = -1;
	for ( I idx=size()-1; idx!=-1; idx-- )
	    if ( v_[idx] == t ) { tidx = idx; break; }
	for ( I idx=tidx; idx>0; idx-- )
	    swap( idx, idx-1 );
    }

protected:

    std::vector<T>	v_;

};


template<class T,class I> inline
bool VectorAccess<T,I>::setCapacity( I sz )
{
    try { v_.reserve(sz); }
    catch ( std::bad_alloc )
    { return false; }
    catch ( std::length_error )
    { return false; }

    return true;
}


template<class T,class I> inline
bool VectorAccess<T,I>::push_back( const T& t )
{
    try { v_.push_back(t); }
    catch ( std::bad_alloc )
    { return false; }

    return true;
}


template<class T,class I> inline
bool VectorAccess<T,I>::setSize( I sz, T val )
{
    try { v_.resize(sz,val); }
    catch ( std::bad_alloc )
    { return false; }

    return true;
}
#endif
