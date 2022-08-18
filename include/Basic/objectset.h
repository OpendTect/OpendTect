#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
  you can use getIdx(ptr) to obtain its index.

*/

template <class T>
mClass(Basic) ObjectSet : public OD::Set
{
public:

    typedef int			size_type;
    typedef size_type		idx_type;
    typedef T			object_type;

    inline			ObjectSet();
    inline explicit		ObjectSet(T*);
    inline explicit		ObjectSet(T*,T*);
    inline explicit		ObjectSet(T*,T*,T*);
    inline			ObjectSet(const ObjectSet&);
    inline virtual		~ObjectSet()		{}
    inline ObjectSet&		operator=(const ObjectSet&);
    virtual bool		isManaged() const	{ return false; }
    ObjectSet*			clone() const override
				{ return new ObjectSet(*this); }

    inline bool			nullAllowed() const	{ return allow0_; }
    inline void			setNullAllowed(bool yn=true);
    inline size_type		size() const		{ return vec_.size(); }
    inline od_int64		nrItems() const override { return size(); }

    inline bool			validIdx(od_int64) const override;
    inline virtual bool		isPresent(const T*) const;
    inline virtual idx_type	indexOf(const T*) const;
    inline virtual T*		get(idx_type);
    inline virtual const T*	get(idx_type) const;
    inline virtual T*		get(const T*) const; //!< check & unconst
    inline T*			first();
    inline const T*		first() const;
    inline T*			last();
    inline const T*		last() const;

    inline ObjectSet&		add( T* t )		{ return doAdd(t); }
    inline void			push( T* t )		{ doAdd( t ); }
    inline bool			addIfNew(T*);
    inline virtual T*		replace(idx_type,T*);
    inline virtual void		insertAt(T* newptr,idx_type);
    inline virtual void		insertAfter(T* newptr,idx_type);
    inline void			swap(idx_type,idx_type);
    inline void			useIndexes(const idx_type*);

    inline virtual void		copy(const ObjectSet&);
    inline virtual void		append(const ObjectSet&);
    inline void			swapItems( od_int64 i1, od_int64 i2 ) override
				{ swap( (idx_type)i1, (idx_type)i2 ); }
    inline void			reverse() override;


    inline void			erase() override	{ plainErase(); }
    inline virtual T*		pop();
    virtual inline T*		removeSingle(idx_type,bool keep_order=true);
				/*!<\returns the removed pointer. */
    virtual void		removeRange(idx_type from,idx_type to);

    inline ObjectSet&		operator +=( T* t )	{ return doAdd( t ); }
    inline T*			operator[]( idx_type i )	{return get(i);}
    inline const T*		operator[]( idx_type i ) const	{return get(i);}
    inline const T*		operator[]( const T* t ) const	{return get(t);}
    virtual ObjectSet&		operator -=(T*);

protected:

    typedef VectorAccess<T*,size_type>   impl_type;
    impl_type			vec_;
    bool			allow0_		    = false;

    inline virtual ObjectSet&	doAdd(T*);

public:

    inline void			plainErase()	{ vec_.erase(); }
				/*!< Not virtual. Don't use casually. */
    inline void allowNull( bool yn=true )
				{ setNullAllowed(yn); }

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
    inline size_type		max_size() const { return maxIdx32(); }
    inline bool			empty() const	{ return isEmpty(); }
    inline bool			operator==(const ObjectSet&) const;
    inline bool			operator!=( const ObjectSet& oth ) const
				{ return !(oth == *this); }
    inline void			swap( ObjectSet& oth )
				{ vec_.swap(oth.vec_); }

    // Usability
    idx_type	getIdx( iterator it ) const	{ return vec_.getIdx(it); }
    idx_type	getIdx( const_iterator it ) const { return vec_.getIdx(it); }

};


//! empty the ObjectSet deleting all objects pointed to.
template <class T>
inline void deepErase( ObjectSet<T>& os )
{
    for ( auto* obj : os )
	delete obj;
    os.plainErase();
}

//! empty the ObjectSet deleting all array objects pointed to.
template <class T>
inline void deepEraseArr( ObjectSet<T>& os )
{
    for ( auto* obj : os )
	delete [] obj;
    os.plainErase();
}

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
    if ( &to == &from )
	return;
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
inline typename ObjectSet<T>::idx_type indexOf( const ObjectSet<T>& os,
						 const S& val )
{
    for ( typename ObjectSet<T>::idx_type idx=os.size()-1; idx>=0; idx-- )
    {
	const T* obj = os[idx];
	if ( obj && *obj == val )
	    return idx;
    }
    return -1;
}

//! Get const object in set
template <class T,class S>
inline const T* find( const ObjectSet<T>& os, const S& val )
{
    const typename ObjectSet<T>::idx_type idx = indexOf( os, val );
    return idx == -1 ? 0 : os[idx];
}

//! Get object in set
template <class T,class S>
inline T* find( ObjectSet<T>& os, const S& val )
{
    const typename ObjectSet<T>::idx_type idx = indexOf( os, val );
    return idx == -1 ? 0 : os[idx];
}

//! Sort ObjectSet when nulls are allowed no need to call.
template <class T>
inline void _ObjectSet_sortWithNull( ObjectSet<T>& os )
{
    const typename ObjectSet<T>::size_type sz = os.size();
    for ( typename ObjectSet<T>::size_type d=sz/2; d>0; d=d/2 )
    {
	for ( typename ObjectSet<T>::idx_type i=d; i<sz; i++ )
	{
	    for ( typename ObjectSet<T>::idx_type j=i-d; j>=0; j-=d )
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
	    for ( typename ObjectSet<T>::idx_type i=d; i<sz; i++ )
	    {
		for ( typename ObjectSet<T>::idx_type j=i-d;
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
    typedef typename ObjectSet<T>::size_type IType;
    const IType sz = os1.size();
    if ( os2.size() != sz )
	return false;

    for ( IType idx=0; idx<sz; idx++ )
	if ( os1[idx] != os2[idx] )
	    return false;

    return true;
}

//! See if all objects pointed to are equal
template <class T>
inline bool equalContents( const ObjectSet<T>& os1, const ObjectSet<T>& os2 )
{
    typedef typename ObjectSet<T>::size_type IType;
    const IType sz = os1.size();
    if ( os2.size() != sz )
	return false;

    for ( IType idx=0; idx<sz; idx++ )
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
ObjectSet<T>::ObjectSet()
{}

template <class T> inline
ObjectSet<T>::ObjectSet( T* t ) : allow0_(!t)
{
    add( t );
}

template <class T> inline
ObjectSet<T>::ObjectSet( T* t0, T* t1 ) : allow0_(!t0 || !t1)
{
    add( t0 ).add( t1 );
}

template <class T> inline
ObjectSet<T>::ObjectSet( T* t0, T* t1, T* t2 ) : allow0_(!t0 || !t1 || !t2)
{
    add( t0 ).add( t1 ).add( t2 );
}

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

    for ( idx_type vidx=sz-1; vidx!=-1; vidx-- )
	if ( get(vidx) != oth.get(vidx) )
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
	    for ( idx_type vidx=size()-1; vidx!=-1; vidx-- )
	    {
		T* obj = (*this)[vidx];
		if ( !obj )
		    removeSingle( vidx );
	    }
	}
    }
}

template <class T> inline
bool ObjectSet<T>::validIdx( od_int64 vidx ) const
{ return vidx>=0 && vidx<size(); }

template <class T> inline
T* ObjectSet<T>::get( idx_type vidx )
{
#ifdef __debug__
    if ( !validIdx(vidx) )
	DBG::forceCrash(true);
#endif
    return vec_[vidx];
}

template <class T> inline
const T* ObjectSet<T>::get( idx_type vidx ) const
{
#ifdef __debug__
    if ( !validIdx(vidx) )
	DBG::forceCrash(true);
#endif
    return vec_[vidx];
}

template <class T> inline
T* ObjectSet<T>::get( const T* t ) const
{
    const idx_type vidx = indexOf( t );
    return vidx < 0 ? 0 : const_cast<T*>(t);
}

template <class T> inline
typename ObjectSet<T>::idx_type ObjectSet<T>::indexOf( const T* ptr ) const
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
void ObjectSet<T>::swap( idx_type idx1, idx_type idx2 )
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
void ObjectSet<T>::useIndexes( const idx_type* idxs )
{
    const size_type sz = size();
    if ( idxs && sz > 1 )
    {
	ObjectSet<T> tmp( *this );
	for ( size_type idx=0; idx<sz; idx++ )
	    ObjectSet<T>::replace( idx, tmp.get(idxs[idx]) );
    }
}

template <class T> inline
void ObjectSet<T>::reverse()
{
    const size_type sz = size();
    const size_type hsz = sz/2;
    for ( idx_type vidx=0; vidx<hsz; vidx++ )
	swap( vidx, sz-1-vidx );
}

template <class T> inline
T* ObjectSet<T>::replace( idx_type vidx, T* newptr )
{
    if ( !validIdx(vidx) )
#ifdef __debug__
	DBG::forceCrash(true);
#else
	return 0;
#endif
    T* ptr = static_cast<T*>( vec_[vidx] );
    vec_[vidx] = newptr;
    return ptr;
}

template <class T> inline
void ObjectSet<T>::insertAt( T* newptr, idx_type vidx )
{
    vec_.insert( vidx, newptr );
}

template <class T> inline
void ObjectSet<T>::insertAfter( T* newptr, idx_type vidx )
{
    add( newptr );
    if ( vidx < 0 )
	vec_.moveToStart( newptr );
    else
	vec_.moveAfter( newptr, vec_[vidx] );
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
    for ( idx_type vidx=0; vidx<sz; vidx++ )
	add( const_cast<T*>( os[vidx] ) );
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
T* ObjectSet<T>::removeSingle( idx_type vidx, bool kporder )
{
    T* res = static_cast<T*>(vec_[vidx]);
    if ( kporder )
	vec_.remove( vidx );
    else
    {
	const idx_type lastidx = size()-1;
	if ( vidx!=lastidx )
	    vec_[vidx] = vec_[lastidx];
	vec_.remove( lastidx );
    }
    return res;
}

template <class T> inline
void ObjectSet<T>::removeRange( idx_type i1, idx_type i2 )
{ vec_.remove( i1, i2 ); }
template <class T> inline T* ObjectSet<T>::first()
{ return isEmpty() ? 0 : get( 0 ); }
template <class T> inline const T* ObjectSet<T>::first() const
{ return isEmpty() ? 0 : get( 0 ); }
template <class T> inline T* ObjectSet<T>::last()
{ return isEmpty() ? 0 : get( size()-1 ); }
template <class T> inline const T* ObjectSet<T>::last() const
{ return isEmpty() ? 0 : get( size()-1 ); }

				//--- compat with std containers

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
