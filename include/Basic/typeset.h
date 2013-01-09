#ifndef typeset_h
#define typeset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert / many others
 Date:		Apr 1995 / Feb 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#ifndef odset_h
#include "odset.h"
#endif
#ifndef vectoraccess_h
#include "vectoraccess.h"
#endif

#ifdef __debug__
# include "debug.h"
#endif

/*!
\ingroup Basic
\brief Set of (small) copyable elements.

  The TypeSetBase is meant for simple types or small objects that have a copy
  constructor. The `-=' function will only remove the first occurrence that
  matches using the `==' operator. The requirement of the presence of that  
  operator is actually not that bad: at least you can't forget it.
  
  Do not make TypeSetBase<bool> (don't worry, it won't compile). Use the
  BoolTypeSet typedef just after the class definition. See vectoraccess.h for
  details on why.
*/

template <class T, class I>
class TypeSetBase : public OD::Set
{
public:
    inline virtual		~TypeSetBase();
    inline TypeSetBase<T,I>&	operator =(const TypeSetBase<T,I>&);

    inline I			size() const	{ return vec_.size(); }
    inline virtual od_int64	nrItems() const	{ return size(); }
    inline virtual bool		setSize(I,T val=T());
				/*!<\param val value assigned to new items */
    inline virtual bool		setCapacity( I sz );
				/*!<Allocates mem only, no size() change */
    inline void			setAll(T);

    inline T&			operator[](I);
    inline const T&		operator[](I) const;
    inline T&			first();
    inline const T&		first() const;
    inline T&			last();
    inline const T&		last() const;
    inline virtual bool		validIdx(od_int64) const;
    inline virtual I		indexOf(T,bool forward=true,I start=-1) const;
    inline bool			isPresent( const T& t ) const
    						{ return indexOf(t) >= 0; }
    inline I			count(const T&) const;

    inline TypeSetBase<T,I>&		operator +=(const T&);
    inline TypeSetBase<T,I>&		operator -=(const T&);
    inline virtual TypeSetBase<T,I>&	copy(const T*,I);
    inline virtual TypeSetBase<T,I>&	copy(const TypeSetBase<T,I>&);
    inline virtual bool		append(const T*,I);
    inline virtual bool		append(const TypeSetBase<T,I>&);
    inline bool			add(const T&);
    inline bool			push(const T& t) { return add(t); }
    inline T			pop();
    inline virtual void		swap(od_int64,od_int64);
    inline virtual void		reverse();
    virtual inline void		createUnion(const TypeSetBase<T,I>&);
				/*!< Adds items not already there */
    virtual inline void		createIntersection(const TypeSetBase<T,I>&);
				//!< Only keeps common items
    virtual inline void		createDifference(const TypeSetBase<T,I>&,
	    				 bool must_preserve_order=false);
				//!< Removes all items present in other set.

    inline virtual bool		addIfNew(const T&);
    virtual void		fillWith(const T&);

    inline virtual void		erase();

    inline virtual void		removeSingle(I,bool preserver_order=true);
    inline virtual void		removeRange(od_int64 from,od_int64 to);
    
    inline virtual void		insert(I,const T&);

				//! 3rd party access
    inline virtual T*		arr()		{ return gtArr(); }
    inline virtual const T*	arr() const	{ return gtArr(); }
    inline std::vector<T>&	vec();
    inline const std::vector<T>& vec() const;

protected:
    
    inline			TypeSetBase();
    inline			TypeSetBase(I nr,T typ);
    inline			TypeSetBase(const T*,I nr);
    inline			TypeSetBase(const TypeSetBase<T,I>&);

    VectorAccess<T,I>		vec_;

    inline virtual T*		gtArr() const;

};


/*!
\ingroup Basic
\brief Derived class of TypeSetBase.
*/

template <class T>
class TypeSet : public TypeSetBase<T,int>
{
    	typedef int size_type;
public:
	TypeSet() : TypeSetBase<T,size_type>() 				{}
	TypeSet(int nr,T typ) : TypeSetBase<T,size_type>( nr, typ )	{}
	TypeSet(const T* t,int nr) : TypeSetBase<T,size_type>( t, nr )	{}
	TypeSet(const TypeSet<T>& t) : TypeSetBase<T,size_type>( t )	{}
};


/*!
\ingroup Basic
\brief We need this because STL has a crazy specialisation of the vector<bool>.
*/

class BoolTypeSetType
{
public:
   BoolTypeSetType(bool v=false) : val_( v ){}
       operator bool () const { return (bool) val_; }
    bool  operator=(bool v) { val_ = v; return v; }
protected:
    char  val_;
};

typedef TypeSet<BoolTypeSetType> BoolTypeSet;
//!< This sux, BTW.


/*!
\ingroup Basic
\brief Large Value Vector. Publicly derived from TypeSetBase.
*/

template <class T>
class LargeValVec : public TypeSetBase<T,od_int64>
{
	typedef od_int64 size_type;
public:
	LargeValVec() : TypeSetBase<T,size_type>() 			{}
	LargeValVec(int nr,T typ) : TypeSetBase<T,size_type>( nr, typ )	{}
	LargeValVec(const T* t,int nr) : TypeSetBase<T,size_type>( t, nr ){}
	LargeValVec(const TypeSet<T>& t) : TypeSetBase<T,size_type>( t ){}

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
    if ( !to.setCapacity( sz + to.size() ) ) return false;
    for ( J idx=0; idx<sz; idx++ )
	to += from[idx];

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
TypeSetBase<T,I>& TypeSetBase<T,I>::operator =( const TypeSetBase<T,I>& ts )
{ return copy( ts ); }


template <class T, class I> inline
bool TypeSetBase<T,I>::setSize( I sz, T val )
{ return vec_.setSize( sz, val ); }


template <class T, class I> inline
bool TypeSetBase<T,I>::setCapacity( I sz )
{ return vec_.setCapacity( sz ); }

template <class T, class I> inline
void TypeSetBase<T,I>::setAll( T val )
{
    const I sz = size();
    T* v = arr();
    for ( I idx=0; idx<sz; idx++ )
	v[idx] = val;
}


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
void TypeSetBase<T,I>::reverse()
{
    const I sz = size();
    const I hsz = sz/2;
    for ( I idx=0; idx<hsz; idx++ )
	swap( idx, sz-1-idx );
}


template <class T, class I> inline
bool TypeSetBase<T,I>::validIdx( od_int64 idx ) const
{ return idx>=0 && idx<size(); }


template <class T, class I> inline
T& TypeSetBase<T,I>::operator[]( I idx )
{
#ifdef __debug__
    if ( !validIdx(idx) )
	DBG::forceCrash(true);
#endif
    return vec_[idx];
}


template <class T, class I> inline
const T& TypeSetBase<T,I>::operator[]( I idx ) const
{
#ifdef __debug__
    if ( !validIdx(idx) )
	DBG::forceCrash(true);
#endif
    return vec_[idx];
}


template <class T, class I> inline
T& TypeSetBase<T,I>::first()			{ return vec_.first(); }


template <class T, class I> inline
const T& TypeSetBase<T,I>::first() const	{ return vec_.first(); }


template <class T, class I> inline
T& TypeSetBase<T,I>::last()			{ return vec_.last(); }


template <class T, class I> inline
const T& TypeSetBase<T,I>::last() const		{ return vec_.last(); }


template <class T, class I> inline
T TypeSetBase<T,I>::pop()
{
    const T res = vec_.last();
    vec_.pop_back();
    return res;
}



template <class T, class I> inline
I TypeSetBase<T,I>::indexOf( T typ, bool forward, I start ) const
{
    const T* ptr = arr();
    if ( forward )
    {
	const I sz = size();
	if ( start<0 || start>=sz ) start = 0;
	for ( I idx=start; idx<sz; idx++ )
	    if ( ptr[idx] == typ ) return idx;
    }
    else
    {
	const I sz = size();
	if ( start<0 || start>=sz ) start = sz-1;
	for ( I idx=start; idx>=0; idx-- )
	    if ( ptr[idx] == typ ) return idx;
    }

    return -1;
}


template <class T, class I> inline
I TypeSetBase<T,I>::count( const T& typ ) const
{
    const T* ptr = arr();
    I res = 0;
    const I sz = size();
    for ( I idx=0; idx<sz; idx++ )
        if ( ptr[idx] == typ )
            res++;
    
    return res;
}



template <class T, class I> inline
TypeSetBase<T,I>& TypeSetBase<T,I>::operator +=( const T& typ )
{ vec_.push_back( typ ); return *this; }


template <class T, class I> inline
TypeSetBase<T,I>& TypeSetBase<T,I>::operator -=( const T& typ )
{ vec_.erase( typ ); return *this; }


template <class T, class I> inline
TypeSetBase<T,I>& TypeSetBase<T,I>::copy( const TypeSetBase<T,I>& ts )
{
    return this == &ts ? *this : copy( ts.arr(), ts.size() );
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
bool TypeSetBase<T,I>::add( const T& t )
{
    return vec_.push_back( t );
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

    if ( !setCapacity( sz+size() ) )
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
	if ( ts.indexOf((*this)[idx]) != -1 )
	    continue;
	removeSingle( idx--, false );
    }
}


template <class T, class I>
inline void TypeSetBase<T,I>::createDifference( const TypeSetBase<T,I>& ts, bool kporder )
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
    if ( indexOf(typ) < 0 ) { *this += typ; return true; }
    return false;
}


template <class T, class I> inline
void TypeSetBase<T,I>::fillWith( const T& t )
{ vec_.fillWith(t); }


template <class T, class I> inline
void TypeSetBase<T,I>::erase()
{ vec_.erase(); }


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


template <class T, class I> inline
void TypeSetBase<T,I>::removeRange( od_int64 i1, od_int64 i2 )
{ vec_.remove( (I)i1, (I)i2 ); }


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
{ return size() ? const_cast<T*>(&(*this)[0]) : 0; }


#endif
