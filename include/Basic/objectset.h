#ifndef objectset_h
#define objectset_h

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


/*!\brief Set of pointers to objects

The ObjectSet does not manage the objects, it is just a collection of
pointers to the the objects. If you want management, use ManagedObjectSet.

*/

template <class T>
class ObjectSet : public OD::Set
{
public:

    inline 			ObjectSet();
    inline			ObjectSet(const ObjectSet<T>&);
    inline virtual		~ObjectSet()		{}
    inline ObjectSet<T>&	operator =(const ObjectSet<T>&);
    virtual bool		isManaged() const	{ return false; }

    inline bool			nullAllowed() const	{ return allow0_; }
    inline void			allowNull(bool yn=true);
    inline int			size() const		{ return vec_.size(); }
    inline virtual od_int64	nrItems() const		{ return size(); }

    inline virtual bool		validIdx(od_int64) const;
    inline virtual bool		isPresent(const T*) const;
    inline virtual int		indexOf(const T*) const;
    inline virtual T*		operator[](int);
    inline virtual const T*	operator[](int) const;
    inline virtual T*		operator[](const T*) const; //!< check & unconst

    inline virtual T*		replace(int idx,T*);
    inline virtual void		insertAt(T* newptr,int);
    inline virtual void		insertAfter(T* newptr,int);
    inline virtual void		copy(const ObjectSet<T>&);
    inline virtual void		append(const ObjectSet<T>&);
    inline virtual void		swap(od_int64,od_int64);
    inline virtual void		reverse(); 

    inline virtual ObjectSet<T>& operator +=(T*);
    inline virtual ObjectSet<T>& operator -=(T*);
    inline virtual void		push(T* ptr);
    inline virtual T*		pop();

    inline virtual void		erase()		{ plainErase(); }
    virtual inline T*		remove(int idx,bool preserve_order=true)
    				{ return removeSingle( idx, preserve_order ); }
    				
    inline virtual void		remove(od_int64 from,od_int64 to)
    				{ removeRange( from, to ); }
    
    virtual inline T*		removeSingle(int,bool preserve_order = true );
    				/*!<\returns the removed pointer. */
    virtual void		removeRange(od_int64 from,od_int64 to);

    inline T*			first();
    inline const T*		first() const;
    inline T*			last();
    inline const T*		last() const;


protected:

    VectorAccess<void*,int>	vec_;
    bool			allow0_;

public:

    inline void			plainErase()	{ vec_.erase(); }
				/*!< Not virtual. Don't use casually. */
};


#define mObjectSetApplyToAll( os, op ) \
    for ( int idx=os.size()-1; idx>=0; idx-- ) \
	op

#define mObjectSetApplyToAllFunc( fn, op, extra ) \
template <class T> \
inline void fn( ObjectSet<T>& os ) \
{ \
    mObjectSetApplyToAll( os, op ); \
    extra; \
}


//! empty the ObjectSet deleting all objects pointed to.
mObjectSetApplyToAllFunc( deepErase, delete os.remove(idx),  )


//! empty the ObjectSet deleting all objects pointed to.
mObjectSetApplyToAllFunc( deepEraseArr, delete [] os.remove(idx), )


//! append copies of one set's objects to another ObjectSet.
template <class T,class S>
inline void deepAppend( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    const int sz = from.size();
    for ( int idx=0; idx<sz; idx++ )
	to += from[idx] ? new T( *from[idx] ) : 0;
}


//! append clones of one set's objects to another ObjectSet.
template <class T,class S>
inline void deepAppendClone( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    const int sz = from.size();
    for ( int idx=0; idx<sz; idx++ )
	to += from[idx] ? from[idx]->clone() : 0;
}


//! fill an ObjectSet with copies of the objects in the other set.
template <class T,class S>
inline void deepCopy( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    if ( &to == &from ) return;
    deepErase( to );
    to.allowNull( from.nullAllowed() );
    deepAppend( to, from );
}


//! fill an ObjectSet with clones of the objects in the other set.
template <class T,class S>
inline void deepCopyClone( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    if ( &to == &from ) return;
    deepErase( to );
    to.allowNull( from.nullAllowed() );
    deepAppendClone( to, from );
}


//! Locate object in set
template <class T,class S>
inline int indexOf( const ObjectSet<T>& os, const S& val )
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
    const int idx = indexOf( os, val );
    return idx == -1 ? 0 : os[idx];
}


//! Get object in set
template <class T,class S>
inline T* find( ObjectSet<T>& os, const S& val )
{
    const int idx = indexOf( os, val );
    return idx == -1 ? 0 : os[idx];
}

//! Sort ObjectSet when nulls are allowed no need to call.
template <class T>
inline void _ObjectSet_sortWithNull( ObjectSet<T>& os )
{
    const int sz = os.size();
    for ( int d=sz/2; d>0; d=d/2 )
    {
	for ( int i=d; i<sz; i++ )
	{
	    for ( int j=i-d; j>=0; j-=d )
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
	const int sz = os.size();
	for ( int d=sz/2; d>0; d=d/2 )
	    for ( int i=d; i<sz; i++ )
		for ( int j=i-d; j>=0 && *os[j]>*os[j+d]; j-=d )
		    os.swap( j, j+d );
    }
}


// Member function implementations
template <class T> inline
ObjectSet<T>::ObjectSet() : allow0_(false)
{}


template <class T> inline
ObjectSet<T>::ObjectSet( const ObjectSet<T>& t )
{ *this = t; }


template <class T> inline
ObjectSet<T>& ObjectSet<T>::operator =( const ObjectSet<T>& os )
{ allow0_ = os.allow0_; copy(os); return *this; }


template <class T> inline
void ObjectSet<T>::allowNull( bool yn )
{ allow0_ = yn; }


template <class T> inline
bool ObjectSet<T>::validIdx( od_int64 idx ) const
{ return idx>=0 && idx<size(); }


template <class T> inline
T* ObjectSet<T>::operator[]( int idx )
{
#ifdef __debug__
    if ( !validIdx(idx) )
	DBG::forceCrash(true);
#endif
    return (T*)vec_[idx];
}


template <class T> inline
const T* ObjectSet<T>::operator[]( int idx ) const
{
#ifdef __debug__
    if ( !validIdx(idx) )
	DBG::forceCrash(true);
#endif
    return (const T*)vec_[idx];
}


template <class T> inline
T* ObjectSet<T>::operator[]( const T* t ) const
{
    const int idx = indexOf(t);
    return idx < 0 ? 0 : const_cast<T*>(t);
}


template <class T> inline
int ObjectSet<T>::indexOf( const T* ptr ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	if ( (const T*)vec_[idx] == ptr ) return idx;
	    return -1;
}


template <class T> inline
bool ObjectSet<T>::isPresent( const T* ptr ) const
{
    return indexOf(ptr) >= 0;
}


template <class T> inline
ObjectSet<T>& ObjectSet<T>::operator +=( T* ptr )
{
    if ( ptr || allow0_ )
	vec_.push_back( (void*)ptr );
    return *this;
}


template <class T> inline
ObjectSet<T>& ObjectSet<T>::operator -=( T* ptr )
{
    if ( ptr || allow0_ )
	vec_.erase( (void*)ptr );
    return *this;
}


template <class T> inline
void ObjectSet<T>::swap( od_int64 idx0, od_int64 idx1 )
{
    if ( idx0<0 || idx0>=size() || idx1<0 || idx1>=size() )
	return;
    void* tmp = vec_[idx0];
    vec_[idx0] = vec_[idx1];
    vec_[idx1] = tmp;
}


template <class T> inline
void ObjectSet<T>::reverse()
{
    const int sz = size();
    const int hsz = sz/2;
    for ( int idx=0; idx<hsz; idx++ )
	swap( idx, sz-1-idx );
}


template <class T> inline
T* ObjectSet<T>::replace( int idx, T* newptr )
{
    if ( idx<0 || idx>=size() ) return 0;
    T* ptr = (T*)vec_[idx];
    vec_[idx] = (void*)newptr; return ptr;
}


template <class T> inline
void ObjectSet<T>::insertAt( T* newptr, int idx )
{
    vec_.insert( idx, (void*)newptr );
}


template <class T> inline
void ObjectSet<T>::insertAfter( T* newptr, int idx )
{
    *this += newptr;
    if ( idx < 0 )
	vec_.moveToStart( (void*)newptr );
    else
	vec_.moveAfter( (void*)newptr, vec_[idx] );
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
    const int sz = os.size();
    vec_.setCapacity( size()+sz );
    for ( int idx=0; idx<sz; idx++ )
	*this += const_cast<T*>( os[idx] );
}


template <class T> inline
void ObjectSet<T>::push( T* ptr )
{ *this +=ptr; }


template <class T> inline
T* ObjectSet<T>::pop()
{
    int sz = size();
    if ( !sz ) return 0;
    return remove( sz-1 );
}


template <class T> inline
T* ObjectSet<T>::removeSingle( int idx, bool kporder )
{
    T* res = (T*)vec_[idx];
    if ( kporder )
	vec_.remove( idx );
    else
    {
	const int lastidx = size()-1;
	if ( idx!=lastidx )
	    vec_[idx] = vec_[lastidx];
	vec_.remove( lastidx );
    }
    return res;
}


template <class T> inline
void ObjectSet<T>::removeRange( od_int64 i1, od_int64 i2 )
{ vec_.remove( i1, i2 ); }
template <class T> inline T* ObjectSet<T>::first()
{ return isEmpty() ? 0 : (*this)[0]; }
template <class T> inline const T* ObjectSet<T>::first() const
{ return isEmpty() ? 0 : (*this)[0]; }
template <class T> inline T* ObjectSet<T>::last()
{ return isEmpty() ? 0 : (*this)[size()-1]; }
template <class T> inline const T* ObjectSet<T>::last() const
{ return isEmpty() ? 0 : (*this)[size()-1]; }


#endif
