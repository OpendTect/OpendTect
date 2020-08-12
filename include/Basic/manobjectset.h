#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "objectset.h"


//!Helper class to RefObjectSet and ManagedObjectSet
template <class T>
mClass(Basic) ManagedObjectSetBase : public ObjectSet<T>
{
public:

    typedef typename ObjectSet<T>::size_type	size_type;
    typedef typename ObjectSet<T>::idx_type	idx_type;

    virtual bool		isManaged() const	{ return true; }

    inline virtual T*		removeSingle(idx_type,bool kporder=true);
				/*!<Removes entry and returns 0 */
    inline virtual void		removeRange(idx_type,idx_type);
    inline virtual T*		replace(idx_type, T*);
				/*!<Deletes entry and returns 0 */
    inline virtual T*		removeAndTake(idx_type,bool kporder=true);
				/*!<Does not delete the entry. */
    inline virtual void		erase();

    inline virtual ManagedObjectSetBase<T>& operator-=(T*);

protected:

    typedef void		(*PtrFunc)(T*ptr);
				ManagedObjectSetBase(PtrFunc delfunc)
				    : ObjectSet<T>()
				    , delfunc_( delfunc )
				{}
				~ManagedObjectSetBase();
private:

    PtrFunc			delfunc_;
};



/*!
\brief ObjectSet where the objects contained are owned by this set.
*/

template <class T>
mClass(Basic) ManagedObjectSet : public ObjectSet<T>
{
public:

    typedef int			size_type;
    typedef T			object_type;

    inline			ManagedObjectSet()	{}
    inline			ManagedObjectSet(const ManagedObjectSet<T>&);
    inline virtual		~ManagedObjectSet();
    inline ManagedObjectSet<T>&	operator =(const ObjectSet<T>&);
    inline ManagedObjectSet<T>&	operator =(const ManagedObjectSet<T>&);
    virtual bool		isManaged() const	{ return true; }

    inline virtual void		erase()			{ deepErase( *this ); }
    inline virtual void		append(const ObjectSet<T>&);
    inline virtual void		removeRange(size_type,size_type);
    inline virtual T*		removeSingle( int idx, bool kporder=true );
				/*!<Deletes entry and returns 0 */
    inline virtual T*		removeAndTake(int idx, bool kporder=true );
				/*!<Does not delete the entry. */
    inline virtual T*		replace(int idx, T*);
				/*!<Deletes entry and returns 0 */
    inline virtual ManagedObjectSet<T>& operator -=(T*);

};


//ObjectSet implementation

template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( const ManagedObjectSet<T>& t )
    : ObjectSet<T>()
{ *this = t; }

template <class T> inline
ManagedObjectSet<T>::~ManagedObjectSet()
{ erase(); }


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =( const ObjectSet<T>& os )
{
    if ( &os != this )
	{ erase(); append( os ); }
    return *this;
}


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator -=( T* ptr )
{
    if ( ptr )
	{ this->vec_.erase( (void*)ptr ); delete ptr; }
    return *this;
}


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =(
					const ManagedObjectSet<T>& os )
{
    if ( &os != this )
	deepCopy( *this, os );
    return *this;
}


template <class T> inline
void ManagedObjectSet<T>::append( const ObjectSet<T>& os )
{
    const int sz = os.size();
    this->vec_.setCapacity( this->size()+sz, true );
    if ( !os.isManaged() )
	ObjectSet<T>::append( os );
    else
	for ( int idx=0; idx<sz; idx++ )
	    ObjectSet<T>::add( os[idx] ? new T( *os[idx] ) : 0 );
}


template <class T> inline
T* ManagedObjectSet<T>::removeSingle( int idx, bool kporder )
{
    delete (*this)[idx];
    ObjectSet<T>::removeSingle( idx, kporder );
    return 0; //Don't give anyone a chance to play with the deleted object
}


template <class T> inline
T* ManagedObjectSet<T>::replace( int idx , T* ptr )
{
    delete ObjectSet<T>::replace( idx, ptr );
    return 0; //Don't give anyone a chance to play with the deleted object
}


template <class T> inline
void ManagedObjectSet<T>::removeRange( size_type i1, size_type i2 )
{
    for ( int idx=(int)i1; idx<=i2; idx++ )
	delete (*this)[idx];
    ObjectSet<T>::removeRange( i1, i2 );
}

template <class T> inline
T* ManagedObjectSet<T>::removeAndTake(int idx, bool kporder )
{
    return ObjectSet<T>::removeSingle( idx, kporder );
}


/*!ObjectSet for reference counted objects. All members are referenced
   once when added to the set, and unreffed when removed from the set.
*/


template <class T>
mClass(Basic) RefObjectSet : public ManagedObjectSetBase<T>
{
public:

    typedef typename ObjectSet<T>::size_type	size_type;
    typedef typename ObjectSet<T>::idx_type	idx_type;

				RefObjectSet();
				RefObjectSet(const RefObjectSet<T>&);
				RefObjectSet(const ObjectSet<T>&);
    virtual RefObjectSet*	clone() const
				{ return new RefObjectSet(*this); }

    RefObjectSet<T>&		operator=(const ObjectSet<T>&);
    inline virtual T*		replace(idx_type,T*);
    inline virtual void		insertAt(T*,idx_type);

protected:

    virtual ObjectSet<T>&	doAdd(T*);
    static void			unRef( T* ptr ) { unRefPtr(ptr); }

};


template <class T> inline
RefObjectSet<T>::RefObjectSet()
    : ManagedObjectSetBase<T>( unRef )
{}


template <class T> inline
RefObjectSet<T>::RefObjectSet( const ObjectSet<T>& os )
    : ManagedObjectSetBase<T>( unRef )
{ *this = os; }


template <class T> inline
RefObjectSet<T>::RefObjectSet( const RefObjectSet<T>& os )
    : ManagedObjectSetBase<T>( unRef )
{ *this = os; }


template <class T> inline
RefObjectSet<T>& RefObjectSet<T>::operator =(const ObjectSet<T>& os)
{ ObjectSet<T>::operator=(os); return *this; }


template <class T> inline
T* RefObjectSet<T>::replace( idx_type vidx, T *ptr )
{
    refPtr( ptr );
    return ManagedObjectSetBase<T>::replace( vidx, ptr );
}


template <class T> inline
void RefObjectSet<T>::insertAt( T *ptr, idx_type vidx )
{
    refPtr( ptr );
    ManagedObjectSetBase<T>::insertAt( ptr, vidx );
}


template <class T> inline
ObjectSet<T>& RefObjectSet<T>::doAdd( T *ptr )
{ refPtr( ptr ); return ObjectSet<T>::doAdd(ptr); }


