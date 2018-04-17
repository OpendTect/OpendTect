#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert / many others
 Date:		Apr 1995 / Feb 2009
________________________________________________________________________

-*/

#include "odset.h"
#include "vectoraccess.h"
#ifdef __debug__
# include "debug.h"
#endif


/*!
\brief Set of pointers to objects.

  The ObjectSet does not manage the objects, it is just a collection of
  pointers to the objects. If you want management, use ManagedObjectSet.

  Note: you can use indexOf(ptr) to see whether the object is in the set. If
  you *know* that the object is in the set, and you need the index of it, then
  you can use idx(ptr) to obtain its index.

*/

template <class T>
mClass(Basic) ObjectSet : public OD::Set
{
public:

    typedef int			size_type;
    typedef T			object_type;

    inline			ObjectSet();
    inline			ObjectSet(const ObjectSet&);
    inline virtual		~ObjectSet()		{}
    inline ObjectSet&		operator=(const ObjectSet&);
    virtual bool		isManaged() const	{ return false; }

    inline bool			nullAllowed() const	{ return allow0_; }
    inline void			setNullAllowed(bool yn=true);
    inline size_type		size() const		{ return vec_.size(); }
    inline virtual od_int64	nrItems() const		{ return size(); }

    inline virtual bool		validIdx(od_int64) const;
    inline virtual bool		isPresent(const T*) const;
    inline virtual size_type	indexOf(const T*) const;
    size_type			idx( const T* t ) const	{ return t-first(); }
    inline virtual T*		get(size_type);
    inline virtual const T*	get(size_type) const;
    inline virtual T*		get(const T*) const; //!< check & unconst
    inline T*			first();
    inline const T*		first() const;
    inline T*			last();
    inline const T*		last() const;

    inline ObjectSet&		add( T* t )		{ return doAdd(t); }
    inline void			push( T* t )		{ doAdd( t ); }
    inline bool			addIfNew(T*);
    inline virtual T*		replace(size_type idx,T*);
    inline virtual void		insertAt(T* newptr,size_type);
    inline virtual void		insertAfter(T* newptr,size_type);
    inline void			swap(size_type,size_type);

    inline virtual void		copy(const ObjectSet&);
    inline virtual void		append(const ObjectSet&);
    inline virtual void		swapItems( od_int64 i1, od_int64 i2 )
				{ swap( (size_type)i1, (size_type)i2 ); }
    inline virtual void		reverse();


    inline virtual void		erase()			{ plainErase(); }
    inline virtual T*		pop();
    virtual inline T*		removeSingle(size_type,bool keep_order=true);
				/*!<\returns the removed pointer. */
    virtual void		removeRange(size_type from,size_type to);

    inline ObjectSet&		operator +=( T* t )	{ return doAdd( t ); }
    inline T*			operator[]( size_type i )	{return get(i);}
    inline const T*		operator[]( size_type i ) const	{return get(i);}
    inline const T*		operator[]( const T* t ) const	{return get(t);}
    virtual ObjectSet&		operator -=(T*);

protected:

    typedef VectorAccess<T*,size_type>   impl_type;
    impl_type			vec_;
    bool			allow0_;

    inline virtual ObjectSet&	doAdd(T*);

public:

    inline void			plainErase()	{ vec_.erase(); }
				/*!< Not virtual. Don't use casually. */
    mDeprecated inline void	allowNull( bool yn=true )
				{ setNullAllowed( yn ); }

    // Compat with std containers
    typedef T*					value_type;
    typedef value_type&				reference;
    typedef const value_type&			const_reference;
    typedef typename impl_type::iterator	iterator;
    typedef typename impl_type::const_iterator	const_iterator;
    typedef size_type				difference_type;

    iterator			begin()		{ return vec_.begin(); }
    const_iterator		begin() const	{ return vec_.cbegin(); }
    const_iterator		cbegin() const	{ return vec_.cbegin(); }
    iterator			end()		{ return vec_.end(); }
    const_iterator		end() const	{ return vec_.cend(); }
    const_iterator		cend() const	{ return vec_.cend(); }
    inline bool			operator==(const ObjectSet&) const;
    inline bool			operator!=( const ObjectSet& oth ) const
				{ return !(oth == *this); }
    inline void			swap(ObjectSet&);
    inline size_type		max_size() const { return maxIdx32(); }
    inline bool			empty() const	{ return isEmpty(); }

    // Usability
    size_type	idx( iterator it ) const	{ return vec_.idx(it); }
    size_type	idx( const_iterator it ) const	{ return vec_.idx(it); }

};


#define mObjectSetApplyToAll( os, op ) \
    mODSetApplyToAll( ObjectSet<int>::size_type, os, op )

#define mObjectSetApplyToAllFunc( fn, op, extra ) \
template <class T> \
inline void fn( ObjectSet<T>& os ) \
{ \
    mObjectSetApplyToAll( os, op ); \
    extra; \
}


//! empty the ObjectSet deleting all objects pointed to.
mObjectSetApplyToAllFunc( deepErase, delete os.removeSingle(idx),  )


//! empty the ObjectSet deleting all objects pointed to.
mObjectSetApplyToAllFunc( deepEraseArr, delete [] os.removeSingle(idx), )


//! append copies of one set's objects to another ObjectSet.
template <class T,class S>
inline void deepAppend( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    const int sz = from.size();
    for ( int idx=0; idx<sz; idx++ )
	to.add( from[idx] ? new T( *from[idx] ) : 0 );
}


//! append clones of one set's objects to another ObjectSet.
template <class T,class S>
inline void deepAppendClone( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    const int sz = from.size();
    for ( int idx=0; idx<sz; idx++ )
	to.add( from[idx] ? from[idx]->clone() : 0 );
}


//! fill an ObjectSet with copies of the objects in the other set.
template <class T,class S>
inline void deepCopy( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    if ( &to == &from ) return;
    deepErase( to );
    to.setNullAllowed( from.nullAllowed() );
    deepAppend( to, from );
}


//! fill an ObjectSet with clones of the objects in the other set.
template <class T,class S>
inline void deepCopyClone( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    if ( &to == &from ) return;
    deepErase( to );
    to.setNullAllowed( from.nullAllowed() );
    deepAppendClone( to, from );
}


//! Locate object in set
template <class T,class S>
inline typename ObjectSet<T>::size_type indexOf( const ObjectSet<T>& os,
						 const S& val )
{
    for ( int idx=0; idx<os.size(); idx++ )
    {
	if ( *os[idx] == val )
	    return idx;
    }
    return -1;
}


//! Get const object in set
template <class T,class S>
inline const T* find( const ObjectSet<T>& os, const S& val )
{
    const typename ObjectSet<T>::size_type idx = indexOf( os, val );
    return idx == -1 ? 0 : os[idx];
}


//! Get object in set
template <class T,class S>
inline T* find( ObjectSet<T>& os, const S& val )
{
    const typename ObjectSet<T>::size_type idx = indexOf( os, val );
    return idx == -1 ? 0 : os[idx];
}

//! Sort ObjectSet when nulls are allowed no need to call.
template <class T>
inline void _ObjectSet_sortWithNull( ObjectSet<T>& os )
{
    const typename ObjectSet<T>::size_type sz = os.size();
    for ( typename ObjectSet<T>::size_type d=sz/2; d>0; d=d/2 )
    {
	for ( typename ObjectSet<T>::size_type i=d; i<sz; i++ )
	{
	    for ( typename ObjectSet<T>::size_type j=i-d; j>=0; j-=d )
	    {
		T* o1 = os[j]; T* o2 = os[j+d];
		if ( !o2 || o1 == o2 || (o1 && !(*o1 > *o2) ) )
		    break;
		os.swap( j, j+d );
	    }
	}
    }
}

//! Sort ObjectSet. Must have operator > defined for elements
template <class T>
inline void sort( ObjectSet<T>& os )
{
    if ( os.nullAllowed() )
	_ObjectSet_sortWithNull(os);
    else
    {
	const typename ObjectSet<T>::size_type sz = os.size();
	for ( typename ObjectSet<T>::size_type d=sz/2; d>0; d=d/2 )
	{
	    for ( typename ObjectSet<T>::size_type i=d; i<sz; i++ )
	    {
		for ( typename ObjectSet<T>::size_type j=i-d;
		     j>=0 && *os[j]>*os[j+d]; j-=d )
		{
		    os.swap( j, j+d );
		}
	    }
	}
    }
}


//! See if all objects are equal
template <class T>
inline bool equalObjects( const ObjectSet<T>& os1, const ObjectSet<T>& os2 )
{
    typedef typename ObjectSet<T>::size_type size_type;
    const size_type sz = os1.size();
    if ( os2.size() != sz )
	return false;

    for ( size_type idx=0; idx<sz; idx++ )
	if ( os1[idx] != os2[idx] )
	    return false;

    return true;
}


//! See if all objects pointed to are equal
template <class T>
inline bool equalContents( const ObjectSet<T>& os1, const ObjectSet<T>& os2 )
{
    typedef typename ObjectSet<T>::size_type size_type;
    const size_type sz = os1.size();
    if ( os2.size() != sz )
	return false;

    for ( size_type idx=0; idx<sz; idx++ )
    {
	const T* o1 = os1[idx]; const T* o2 = os2[idx];
	if ( !o1 && !o2 )
	    continue;
	if ( !o1 || !o2 || (!(*o1 == *o2)) )
	    return false;
    }
    return true;
}


// Member function implementations
template <class T> inline
ObjectSet<T>::ObjectSet() : allow0_(false)
{}


template <class T> inline
ObjectSet<T>::ObjectSet( const ObjectSet<T>& t )
{ *this = t; }


template <class T> inline
ObjectSet<T>& ObjectSet<T>::operator =( const ObjectSet<T>& oth )
{
    allow0_ = oth.allow0_;
    copy( oth );
    return *this;
}


template <class T> inline
bool ObjectSet<T>::operator ==( const ObjectSet<T>& oth ) const
{
    if ( this == &oth )
	return true;
    const size_type sz = size();
    if ( sz != oth.size() )
	return false;

    for ( size_type idx=sz; idx!=-1; idx-- )
	if ( (*this)[idx] != oth[idx] )
	    return false;
    return true;
}


template <class T> inline
void ObjectSet<T>::setNullAllowed( bool yn )
{
    if ( allow0_ != yn )
    {
	allow0_ = yn;
	if ( !allow0_ )
	{
	    for ( size_type idx=size()-1; idx!=-1; idx-- )
	    {
		T* obj = (*this)[idx];
		if ( !obj )
		    removeSingle( idx );
	    }
	}
    }
}


template <class T> inline
bool ObjectSet<T>::validIdx( od_int64 idx ) const
{ return idx>=0 && idx<size(); }


template <class T> inline
T* ObjectSet<T>::get( size_type idx )
{
#ifdef __debug__
    if ( !validIdx(idx) )
	DBG::forceCrash(true);
#endif
    return vec_[idx];
}


template <class T> inline
const T* ObjectSet<T>::get( size_type idx ) const
{
#ifdef __debug__
    if ( !validIdx(idx) )
	DBG::forceCrash(true);
#endif
    return vec_[idx];
}


template <class T> inline
T* ObjectSet<T>::get( const T* t ) const
{
    const size_type idx = indexOf(t);
    return idx < 0 ? 0 : const_cast<T*>(t);
}


template <class T> inline
typename ObjectSet<T>::size_type ObjectSet<T>::indexOf( const T* ptr ) const
{
    return vec_.indexOf( (T*)ptr, true );
}


template <class T> inline
bool ObjectSet<T>::isPresent( const T* ptr ) const
{
    return vec_.isPresent( (T*)ptr );
}


template <class T> inline
ObjectSet<T>& ObjectSet<T>::doAdd( T* ptr )
{
    if ( ptr || allow0_ )
	vec_.push_back( ptr );
    return *this;
}


template <class T> inline
ObjectSet<T>& ObjectSet<T>::operator -=( T* ptr )
{
    if ( ptr || allow0_ )
	vec_.erase( ptr );
    return *this;
}


template <class T> inline
void ObjectSet<T>::swap( size_type idx1, size_type idx2 )
{
    if ( !validIdx(idx1) || !validIdx(idx2) )
    {
#ifdef __debug__
	DBG::forceCrash(true);
#endif
	return;
    }
    vec_.swapElems( idx1, idx2 );
}


template <class T> inline
void ObjectSet<T>::reverse()
{
    const size_type sz = size();
    const size_type hsz = sz/2;
    for ( size_type idx=0; idx<hsz; idx++ )
	swap( idx, sz-1-idx );
}


template <class T> inline
T* ObjectSet<T>::replace( size_type idx, T* newptr )
{
    if ( !validIdx(idx) )
#ifdef __debug__
	DBG::forceCrash(true);
#else
	return 0;
#endif
    T* ptr = static_cast<T*>( vec_[idx] );
    vec_[idx] = newptr;
    return ptr;
}


template <class T> inline
void ObjectSet<T>::insertAt( T* newptr, size_type idx )
{
    vec_.insert( idx, newptr );
}


template <class T> inline
void ObjectSet<T>::insertAfter( T* newptr, size_type idx )
{
    add( newptr );
    if ( idx < 0 )
	vec_.moveToStart( newptr );
    else
	vec_.moveAfter( newptr, vec_[idx] );
}


template <class T> inline
void ObjectSet<T>::copy( const ObjectSet<T>& os )
{
    if ( &os != this )
    {
	erase();
	allow0_ = os.allow0_;
	append( os );
    }
}


template <class T> inline
void ObjectSet<T>::append( const ObjectSet<T>& os )
{
    const size_type sz = os.size();
    vec_.setCapacity( size()+sz, true );
    for ( size_type idx=0; idx<sz; idx++ )
	add( const_cast<T*>( os[idx] ) );
}

template <class T> inline
T* ObjectSet<T>::pop()
{ return static_cast<T*>( vec_.pop_back() ); }


template <class T> inline
bool ObjectSet<T>::addIfNew( T* ptr )
{
    if ( isPresent(ptr) )
	return false;

    add( ptr );
    return true;
}


template <class T> inline
T* ObjectSet<T>::removeSingle( size_type idx, bool kporder )
{
    T* res = static_cast<T*>(vec_[idx]);
    if ( kporder )
	vec_.remove( idx );
    else
    {
	const size_type lastidx = size()-1;
	if ( idx!=lastidx )
	    vec_[idx] = vec_[lastidx];
	vec_.remove( lastidx );
    }
    return res;
}


template <class T> inline
void ObjectSet<T>::removeRange( size_type i1, size_type i2 )
{ vec_.remove( i1, i2 ); }
template <class T> inline T* ObjectSet<T>::first()
{ return isEmpty() ? 0 : (*this)[0]; }
template <class T> inline const T* ObjectSet<T>::first() const
{ return isEmpty() ? 0 : (*this)[0]; }
template <class T> inline T* ObjectSet<T>::last()
{ return isEmpty() ? 0 : (*this)[size()-1]; }
template <class T> inline const T* ObjectSet<T>::last() const
{ return isEmpty() ? 0 : (*this)[size()-1]; }


				//--- compat with std containers
template <class T> inline
void ObjectSet<T>::swap( ObjectSet<T>& oth )
{
    const ObjectSet<T> tmp( *this );
    *this = oth;
    oth = tmp;
}

template <class T>
mGlobal(Basic) inline void swap( ObjectSet<T>& os1, ObjectSet<T>& os2 )
{
    os1.swap( os2 );
}

				//--- useful for iterating over any OD::Set
template <class T>
mGlobal(Basic) inline T& getRef( ObjectSet<T>& objset, int i )
{ return *objset.get( i ); }
template <class T>
mGlobal(Basic) inline const T& getRef( const ObjectSet<T>& objset, int i )
{ return *objset.get( i ); }
