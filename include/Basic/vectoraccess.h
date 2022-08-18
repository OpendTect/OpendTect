#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "debug.h"
#include "gendefs.h"
#include <algorithm>
#include <vector>
#include <stdexcept>

/*!
\brief Simple vector-based container simplifying index-based work.

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

template <class T,class IT>
mClass(Basic) VectorAccess
{
public:

    typedef IT					size_type;
    typedef size_type				idx_type;
    typedef T					object_type;
    typedef std::vector<T>			impl_type;
    typedef typename impl_type::iterator	iterator;
    typedef typename impl_type::const_iterator	const_iterator;

    inline iterator	begin()			{ return v_.begin(); }
    inline const_iterator begin() const		{ return v_.cbegin(); }
    inline const_iterator cbegin() const	{ return v_.cbegin(); }
    inline iterator	end()			{ return v_.end(); }
    inline const_iterator end() const		{ return v_.cend(); }
    inline const_iterator cend() const		{ return v_.end(); }
    inline idx_type	getIdx( iterator it ) const
			{ return (idx_type)(it-cbegin()); }
    inline idx_type	getIdx( const_iterator it ) const
			{ return (idx_type)(it-cbegin()); }

    inline		VectorAccess()			{}
    inline		VectorAccess( IT n ) : v_(n)	{}
    inline		VectorAccess( IT n, const T& t )
				: v_(n,t)		{}
    inline		VectorAccess( const VectorAccess& v2 )
				: v_(v2.v_)		{}
    inline impl_type&	 vec()			{ return v_; }
    inline const impl_type& vec() const		{ return v_; }

    inline T&		operator[](IT);
    inline const T&	operator[](IT) const;
    inline T&		first()			{ return v_.front(); }
    inline const T&	first() const		{ return v_.front(); }
    inline T&		last()			{ return v_.back(); }
    inline const T&	last() const		{ return v_.back(); }
    inline IT		size() const		{ return (IT)v_.size();}
    inline bool		setCapacity(IT sz,bool withmargin);
			/*!<Allocates mem for sz, does not change size.*/
    inline IT		getCapacity() const	{ return (IT)v_.capacity();}
			/*!<\returns max size without reallocation.*/
    inline bool		setSize(IT sz,T val);

    inline bool		validIdx( IT i ) const { return i>=0 && i<size();}
    inline IT		indexOf(const T&,bool forward,IT start=-1) const;
    inline IT		count(const T&) const;
    inline bool		isPresent(const T&) const;

    inline VectorAccess& operator =( const VectorAccess& v2 )
			{ v_ = v2.v_; return *this; }
    inline bool		push_back( const T& t );
    inline T		pop_back();
    inline void		insert( IT pos, const T& val )
			{ v_.insert(v_.begin() + pos,val); }
    inline void		erase()
			{ std::vector<T>().swap(v_); }
    inline void		erase( const T& t )
			{
			    for ( IT i=size()-1; i!=-1; i-- )
				{ if ( v_[i] == t ) { remove(i); return; } }
			}
    inline void		remove( IT i )
			{
			    if ( i < size() )
				v_.erase( v_.begin() + i );
			}
    inline void		remove( IT i1, IT i2 )
			{
			    if ( i1 == i2 ) { remove( i1 ); return; }
			    if ( i1 > i2 ) std::swap( i1, i2 );
			    const IT sz = size();
			    if ( i1 >= sz ) return;

			    if ( i2 >= sz-1 ) i2 = sz-1;
			    v_.erase( v_.begin()+i1, v_.begin()+i2+1 );
			}
    inline void		swapElems( IT i, IT j )
			{ std::swap( v_[i], v_[j] ); }
    inline void		fillWith( const T& val )
			{ std::fill( v_.begin(), v_.end(), val ); }
    inline void		replace( const T& val, const T& newval )
			{ std::replace( v_.begin(), v_.end(), val, newval ); }
    inline void		swap( VectorAccess& oth )
			{ v_.swap( oth.v_ ); }

    void moveAfter( const T& t, const T& aft )
    {
	if ( t == aft || size() < 2 ) return;
	IT tidx = -1; IT aftidx = -1;
	for ( IT vidx=size()-1; vidx!=-1; vidx-- )
	{
	    if ( v_[vidx] == t )
		{ tidx = vidx; if ( aftidx != -1 ) break; }
	    if ( v_[vidx] == aft )
		{ aftidx = vidx; if ( tidx != -1 ) break; }
	}
	if ( tidx == -1 || aftidx == -1 || tidx == aftidx ) return;
	if ( aftidx > tidx )
	    for ( IT vidx=tidx; vidx<aftidx; vidx++ )
		swapElems( vidx, vidx+1 );
	else
	    for ( IT vidx=tidx; vidx>aftidx+1; vidx-- )
		swapElems( vidx, vidx-1 );
    }

    void moveToStart( const T& t )
    {
	if ( size() < 2 ) return;
	IT tidx = -1;
	for ( IT vidx=size()-1; vidx!=-1; vidx-- )
	    if ( v_[vidx] == t ) { tidx = vidx; break; }
	for ( IT vidx=tidx; vidx>0; vidx-- )
	    swapElems( vidx, vidx-1 );
    }

protected:

    impl_type	v_;

};


#define mExportVectorAccess(mod,tp,itp) \
template mExpClass(mod) std::allocator<tp>;\
template mExpClass(mod) std::_Vector_val<tp,std::allocator<tp> >;\
template mExpClass(mod) std::vector<tp>;\
template mExpClass(mod) VectorAccess<tp,itp>;\


template<class T,class IT> inline
bool VectorAccess<T,IT>::setCapacity( IT sz, bool withmargin )
{
    if ( sz<=v_.capacity() )
	return true;

    if ( withmargin )
    {
	IT tmp = sz-1;
	sz = 1;

	while ( tmp )
	{
	    tmp >>= 1;
	    sz <<= 1;
	}
    }

    try { v_.reserve(sz); }
    catch ( std::bad_alloc )
    { return false; }
    catch ( std::length_error )
    { return false; }

    return true;
}


template<class T,class IT> inline
bool VectorAccess<T,IT>::push_back( const T& t )
{
    try
	{ v_.push_back(t); }
    catch ( std::bad_alloc )
	{ return false; }

    return true;
}


template<class T,class IT> inline
T VectorAccess<T,IT>::pop_back()
{
    const T lastelem = v_.back();
    v_.pop_back();
    return lastelem;
}


template<class T,class IT> inline
bool VectorAccess<T,IT>::setSize( IT sz, T val )
{
    try { v_.resize(sz,val); }
    catch ( std::bad_alloc )
    { return false; }

    return true;
}

#ifdef __debug__

#define mImplOperator \
    try { return v_.at(vidx); } \
    catch ( std::out_of_range ) \
    { DBG::forceCrash(true); } \
    return v_[(typename std::vector<T>::size_type)vidx]

#else

#define mImplOperator \
    return v_[(typename std::vector<T>::size_type)vidx]

#endif


template<class T,class IT> inline
T& VectorAccess<T,IT>::operator[]( IT vidx )
{
    mImplOperator;
}


template<class T,class IT> inline
const T& VectorAccess<T,IT>::operator[]( IT vidx ) const
{
    mImplOperator;
}

#undef mImplOperator


template<class T,class IT> inline
IT VectorAccess<T,IT>::indexOf( const T& t, bool forward, IT start ) const
{
    if ( forward )
    {
	typename std::vector<T>::const_iterator begit = v_.cbegin();
	const typename std::vector<T>::const_iterator endit = v_.cend();
	if ( start>0 )
	    begit += start;

	const typename std::vector<T>::const_iterator res =
						std::find( begit, endit, t );
	if ( res==endit )
	    return -1;

	return mCast( IT, res-v_.begin() );
    }

    typename std::vector<T>::const_reverse_iterator rebegit = v_.rbegin();
    const typename std::vector<T>::const_reverse_iterator rendit = v_.rend();
    if ( start>0 )
    {
	const IT nrskipped = size()-1-start;
	rebegit += nrskipped;
    }

    const typename impl_type::const_reverse_iterator res
			= std::find( rebegit, rendit, t );
    if ( res == rendit )
	return -1;

    return mCast( IT, rendit-res) - 1;
}


template<class T,class IT> inline
IT VectorAccess<T,IT>::count( const T& t ) const
{
    return mCast( IT, std::count(v_.cbegin(),v_.end(),t) );
}


template<class T,class IT> inline
bool VectorAccess<T,IT>::isPresent( const T& t ) const
{
    const_iterator endit = v_.cend();
    return std::find( v_.cbegin(), endit, t ) != endit;
}
