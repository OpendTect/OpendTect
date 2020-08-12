#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert / many others
 Date:		Apr 1995 / Feb 2009
RCS:		$Id:$
________________________________________________________________________

-*/

#ifndef odset_h
#include "odset.h"
#endif
#ifndef vectoraccess_h
#include "vectoraccess.h"
#endif


/*!\brief Base class for TypeSet, usually not used as such. */

template <class T, class I>
mClass(Basic) TypeSetBase : public OD::Set
{
public:

    typedef I			size_type;
    typedef size_type		idx_type;
    typedef T			object_type;

    inline virtual		~TypeSetBase();
    inline TypeSetBase<T,I>&	operator =( const TypeSetBase<T,I>& ts )
    				{ return copy( ts ); }

    inline size_type		size() const;
    inline virtual od_int64	nrItems() const		{ return size(); }
    inline virtual bool		setSize(size_type,T val=T());
				/*!<\param val value assigned to new items */
    inline virtual bool		setCapacity(size_type sz,bool withmargin);
				/*!<Allocates mem only, no size() change */
    inline virtual size_type	getCapacity() const;
    inline void			setAll(T);
    inline void			replace(T,T);

    inline T&			operator[](size_type);
    inline const T&		operator[](size_type) const;
    inline T&			first();
    inline const T&		first() const;
    inline T&			last();
    inline const T&		last() const;
    inline virtual bool		validIdx(od_int64) const;
    inline virtual size_type	indexOf(T,bool forward=true,
	    				  size_type start=-1) const;
    inline bool			isPresent(const T&) const;
    inline size_type		count(const T&) const;

    inline TypeSetBase<T,I>&	add(const T&);
    inline virtual void		insert(size_type,const T&);
    inline bool			push(const T&);
    inline T			pop();
    inline TypeSetBase<T,I>&	operator+=( const T& t ) { return add(t); }
    inline virtual bool		append(const T*,size_type);
    inline virtual bool		append(const TypeSetBase<T,I>&);
    inline virtual bool		addIfNew(const T&);
    inline virtual TypeSetBase<T,I>& copy(const T*,size_type);
    inline virtual TypeSetBase<T,I>& copy(const TypeSetBase<T,I>&);
    virtual inline void		createUnion(const TypeSetBase<T,I>&);
				/*!< Adds items not already there */
    virtual inline void		createIntersection(const TypeSetBase<T,I>&);
				//!< Only keeps common items
    virtual inline void		createDifference(const TypeSetBase<T,I>&,
	    				 bool must_preserve_order=false);
				//!< Removes all items present in other set.

    inline virtual void		swap(od_int64,od_int64);
    inline virtual void		move(size_type from,size_type to);
    inline virtual void		getReOrdered(const size_type*,
					     TypeSetBase<T,I>&);
				//!< Fills as per the given array of indexes.

    inline virtual void		reverse();

    inline virtual void		erase();
    inline virtual void		removeSingle(size_type,
	    				     bool preserver_order=true);
    inline TypeSetBase<T,I>&	operator -=(const T&);
    inline virtual void		removeRange(size_type from,size_type to);

				//! 3rd party access
    inline virtual T*		arr()		{ return gtArr(); }
    inline virtual const T*	arr() const	{ return gtArr(); }
    inline std::vector<T>&	vec();
    inline const std::vector<T>& vec() const;

protected:
    
    inline			TypeSetBase();
    inline			TypeSetBase(size_type nr,T typ);
    inline			TypeSetBase(const T*,size_type nr);
    inline			TypeSetBase(const TypeSetBase<T,size_type>&);

    VectorAccess<T,I>		vec_;

    inline virtual T*		gtArr() const;

};


/*!\brief Set of (small) copyable elements.

  TypeSet is meant for simple types or small objects that have a copy
  constructor. The `-=' operator will only remove the first occurrence that
  matches using the `==' operator. The requirement of the presence of that  
  operator is actually not that bad: at least you can't forget it.
  
  Do not make TypeSet<bool> (don't worry, it won't compile). Use the
  BoolTypeSet typedef just after the class definition. See vectoraccess.h for
  details on why.
*/

template <class T>
mClass(Basic) TypeSet : public TypeSetBase<T,int>
{
public:

	TypeSet()
	    : TypeSetBase<T,int>() 		{}
	TypeSet( int nr, T typ )
	    : TypeSetBase<T,int>( nr, typ )	{}
	TypeSet( const T* t, int nr )
	    : TypeSetBase<T,int>( t, nr )	{}
	TypeSet( const TypeSet<T>& t )
	    : TypeSetBase<T,int>( t )		{}

};


/*!\brief Needed because the std lib has a crazy specialisation vector<bool>. */

mClass(Basic) BoolTypeSetType
{
public:

			BoolTypeSetType( bool v=false )
			    : val_( v )		{}
    operator		bool() const		{ return (bool) val_; }
    bool  		operator=( bool v )	{ val_ = v; return v; }

protected:

    char  val_;

};

typedef TypeSet<BoolTypeSetType> BoolTypeSet;


/*!\brief Large Value Vector. */

template <class T>
mClass(Basic) LargeValVec : public TypeSetBase<T,od_int64>
{
public:

	LargeValVec()
	    : TypeSetBase<T,od_int64>() 		{}
	LargeValVec( od_int64 nr, T typ )
	    : TypeSetBase<T,od_int64>( nr, typ )	{}
	LargeValVec( const T* t, od_int64 nr )
	    : TypeSetBase<T,od_int64>( t, nr )		{}
	LargeValVec( const TypeSet<T>& t )
	    : TypeSetBase<T,od_int64>( t )		{}

};


template <class T, class I>
inline bool operator ==( const TypeSetBase<T,I>& a, const TypeSetBase<T,I>& b )
{
    if ( a.size() != b.size() ) return false;

    const I sz = a.size();
    for ( I idx=0; idx<sz; idx++ )
	if ( !(a[idx] == b[idx]) ) return false;

    return true;
}

template <class T, class I>
inline bool operator !=( const TypeSetBase<T,I>& a, const TypeSetBase<T,I>& b )
{ return !(a == b); }


//! append allowing a different type to be merged into set
template <class T, class I, class J, class S>
inline bool append( TypeSetBase<T,I>& to, const TypeSetBase<S,J>& from )
{
    const J sz = from.size();
    if ( !to.setCapacity( sz + to.size(), true ) ) return false;
    for ( J idx=0; idx<sz; idx++ )
	to.add( from[idx] );

    return true;
}


//! copy from different possibly different type into set
//! Note that there is no optimisation for equal size, as in member function.
template <class T, class I,class S>
inline void copy( TypeSetBase<T,I>& to, const TypeSetBase<S,I>& from )
{
    if ( &to == &from ) return;
    to.erase();
    append( to, from );
}


//! Sort TypeSetBase. Must have operator > defined for elements
template <class T, class I>
inline void sort( TypeSetBase<T,I>& ts )
{
    T tmp; const I sz = ts.size();
    for ( I d=sz/2; d>0; d=d/2 )
	for ( I i=d; i<sz; i++ )
	    for ( I j=i-d; j>=0 && ts[j]>ts[j+d]; j-=d )
		{ tmp = ts[j]; ts[j] = ts[j+d]; ts[j+d] = tmp; }
}


// Member function implementations
template <class T, class I> inline
TypeSetBase<T,I>::TypeSetBase()
{}

template <class T, class I> inline
TypeSetBase<T,I>::TypeSetBase( I nr, T typ )
{ setSize( nr, typ ); }

template <class T, class I> inline
TypeSetBase<T,I>::TypeSetBase( const T* tarr, I nr )
{ append( tarr, nr ); }

template <class T, class I> inline
TypeSetBase<T,I>::TypeSetBase( const TypeSetBase<T,I>& t )
    : OD::Set( t )
{ append( t ); }

template <class T, class I> inline
TypeSetBase<T,I>::~TypeSetBase() {}

template <class T, class I> inline
I TypeSetBase<T,I>::size() const
{ return vec_.size(); }

template <class T, class I> inline
bool TypeSetBase<T,I>::setSize( I sz, T val )
{ return vec_.setSize( sz, val ); }

template <class T, class I> inline
I TypeSetBase<T,I>::getCapacity() const
{ return vec_.getCapacity(); }

template <class T, class I> inline
bool TypeSetBase<T,I>::setCapacity( I sz, bool withmargin )
{ return vec_.setCapacity( sz, withmargin ); }

template <class T, class I> inline
void TypeSetBase<T,I>::setAll( T val )
{ vec_.fillWith( val ); }

template <class T, class I> inline
    void TypeSetBase<T,I>::replace( T val, T newval )
{ vec_.replace( val, newval ); }


template <class T, class I> inline
bool TypeSetBase<T,I>::validIdx( od_int64 idx ) const
{ return vec_.validIdx( (I)idx ); }

template <class T, class I> inline
T& TypeSetBase<T,I>::operator[]( I idx )
{ return vec_[idx]; }

template <class T, class I> inline
const T& TypeSetBase<T,I>::operator[]( I idx ) const
{ return vec_[idx]; }

template <class T, class I> inline
T& TypeSetBase<T,I>::first()
{ return vec_.first(); }

template <class T, class I> inline
const T& TypeSetBase<T,I>::first() const
{ return vec_.first(); }

template <class T, class I> inline
T& TypeSetBase<T,I>::last()
{ return vec_.last(); }

template <class T, class I> inline
const T& TypeSetBase<T,I>::last() const	
{ return vec_.last(); }

template <class T, class I> inline
T TypeSetBase<T,I>::pop()
{ return vec_.pop_back(); }

template <class T, class I> inline
I TypeSetBase<T,I>::indexOf( T typ, bool forward, I start ) const
{ return vec_.indexOf( typ, forward, start ); }

template <class T, class I> inline
bool TypeSetBase<T,I>::isPresent( const T& t ) const
{ return vec_.isPresent(t); }

template <class T, class I> inline
I TypeSetBase<T,I>::count( const T& typ ) const
{ return vec_.count( typ ); }

template <class T, class I> inline
TypeSetBase<T,I>& TypeSetBase<T,I>::add( const T& typ )
{ vec_.push_back( typ ); return *this; }

template <class T, class I> inline
bool TypeSetBase<T,I>::push( const T& typ )
{ return vec_.push_back( typ ); }

template <class T, class I> inline
TypeSetBase<T,I>& TypeSetBase<T,I>::operator -=( const T& typ )
{ vec_.erase( typ ); return *this; }

template <class T, class I> inline
TypeSetBase<T,I>& TypeSetBase<T,I>::copy( const TypeSetBase<T,I>& ts )
{ return this == &ts ? *this : copy( ts.arr(), ts.size() ); }

template <class T, class I> inline
void TypeSetBase<T,I>::erase()
{ vec_.erase(); }

template <class T, class I> inline
void TypeSetBase<T,I>::removeRange( I i1, I i2 )
{ vec_.remove( i1, i2 ); }

template <class T, class I> inline
void TypeSetBase<T,I>::insert( I idx, const T& typ )
{ vec_.insert( idx, typ );}

template <class T, class I> inline
std::vector<T>& TypeSetBase<T,I>::vec()
{ return vec_.vec(); }

template <class T, class I> inline
const std::vector<T>& TypeSetBase<T,I>::vec() const
{ return vec_.vec(); }

template <class T, class I> inline
T* TypeSetBase<T,I>::gtArr() const
{ return size()>0 ? const_cast<T*>(&(*this)[0]) : 0; }


template <class T, class I> inline
void TypeSetBase<T,I>::swap( od_int64 idx0, od_int64 idx1 )
{
    if ( !validIdx(idx0) || !validIdx(idx1) )
	return;

    T tmp = vec_[(I)idx0];
    vec_[(I)idx0] = vec_[(I)idx1];
    vec_[(I)idx1] = tmp;
}


template <class T, class I> inline
void TypeSetBase<T,I>::move( I idxfrom, I idxto )
{
    if ( !validIdx(idxfrom) || !validIdx(idxto) )
	return;

    T tmp = vec_[idxfrom];
    insert( idxto, tmp );
    vec_.remove( idxfrom < idxto ? idxfrom : idxfrom+1 );
}


template <class T, class I> inline
void TypeSetBase<T,I>::getReOrdered( const I* idxs, TypeSetBase<T,I>& out )
{
    const I sz = size();
    if ( !idxs || sz < 2 )
	return;

    out.erase();
    out.setCapacity( sz, true );
    for ( size_type idx=0; idx<sz; idx++ )
	out.add( vec_[idxs[idx]] );
}


template <class T, class I> inline
void TypeSetBase<T,I>::reverse()
{
    const I sz = size();
    const I hsz = sz/2;
    for ( I idx=0; idx<hsz; idx++ )
	swap( idx, sz-1-idx );
}


template <class T, class I> inline
TypeSetBase<T,I>& TypeSetBase<T,I>::copy( const T* tarr, I sz )
{
    if ( size() != sz )
	{ erase(); append(tarr,sz); }
    else
    {
	for ( I idx=0; idx<sz; idx++ )
	    (*this)[idx] = tarr[idx];
    }
    return *this;
}


template <class T, class I> inline
bool TypeSetBase<T,I>::append( const TypeSetBase<T,I>& ts )
{
    if ( this != &ts )
	return append( ts.arr(), ts.size() );

    const TypeSetBase<T,I> tscp( ts );
    return append( tscp );
}


template <class T, class I> inline
bool TypeSetBase<T,I>::append( const T* tarr, I sz )
{
    if ( !sz ) return true;

    if ( !setCapacity( sz+size(), true ) )
	return false;

    for ( I idx=0; idx<sz; idx++ )
	*this += tarr[idx];

    return true;
}


template <class T, class I>
inline void TypeSetBase<T,I>::createUnion( const TypeSetBase<T,I>& ts )
{
    const I sz = ts.size();
    const T* ptr = ts.arr();
    for ( I idx=0; idx<sz; idx++, ptr++ )
	addIfNew( *ptr );
}


template <class T, class I>
inline void TypeSetBase<T,I>::createIntersection( const TypeSetBase<T,I>& ts )
{
    for ( I idx=0; idx<size(); idx++ )
    {
	if ( ts.isPresent((*this)[idx]) )
	    continue;
	removeSingle( idx--, false );
    }
}


template <class T, class I>
inline void TypeSetBase<T,I>::createDifference( const TypeSetBase<T,I>& ts,
    bool kporder )
{
    const I sz = ts.size();
    for ( I idx=0; idx<sz; idx++ )
    {
	const T typ = ts[idx];
	for ( I idy=0; idy<size(); idy++ )
	{
	    if ( vec_[idy] == typ )
		removeSingle( idy--, kporder );
	}
    }
}


template <class T, class I> inline
bool TypeSetBase<T,I>::addIfNew( const T& typ )
{
    if ( !isPresent(typ) )
	{ *this += typ; return true; }
    return false;
}


template <class T, class I> inline
void TypeSetBase<T,I>::removeSingle( I idx, bool kporder )
{
    if ( kporder )
	vec_.remove( idx );
    else
    {
	const I lastidx = size()-1;
	if ( idx != lastidx )
	    vec_[idx] = vec_[lastidx];
	vec_.remove( lastidx );
    }
}


