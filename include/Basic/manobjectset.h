#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
________________________________________________________________________

-*/

#include "objectset.h"

//!Helper class to RefObjectSet and ManagedObjectSet
template <class T>
mClass(Basic) ManagedObjectSetBase : public ObjectSet<T>
{
public:
    virtual bool		isManaged() const	{ return true; }
    inline virtual T*		removeSingle(int idx,bool kporder=true );
				/*!<Removes entry and returns 0 */
    inline virtual void		removeRange(int,int);
    inline virtual T*		replace(int idx, T*);
				/*!<Deletes entry and returns 0 */
    inline virtual T*		removeAndTake(int idx, bool kporder=true );
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
mClass(Basic) ManagedObjectSet : public ManagedObjectSetBase<T>
{
public:
    inline			ManagedObjectSet();
    				
    inline			ManagedObjectSet(const ManagedObjectSet<T>&);
    				//Must be implemented as default
				//copy constructor will call
				//operator= before class is fully setup and 
				//append is not in virtual table
    				
    inline			ManagedObjectSet(const ObjectSet<T>&);

    ManagedObjectSet<T>&	operator=(const ObjectSet<T>& os);

    inline virtual void		append(const ObjectSet<T>&);

private:

    static void			delFunc(T* ptr) { delete ptr; }
};


/*!ObjectSet for reference counted objects. All members are referenced
   once when added to the set, and unreffed when removed from the set.
*/


template <class T>
mClass(Basic) RefObjectSet : public ManagedObjectSetBase<T>
{
public:
				RefObjectSet();
				RefObjectSet(const RefObjectSet<T>&);
				RefObjectSet(const ObjectSet<T>&);

    RefObjectSet<T>&		operator=(const ObjectSet<T>& os);
    inline virtual T*		replace(int idx, T*);
    inline virtual void		insertAt(T*,int idx);

private:
    virtual ObjectSet<T>&	doAdd(T* ptr);
    static void			unRef(T* ptr) { unRefPtr(ptr); }

};


template <class T> inline
ManagedObjectSetBase<T>::~ManagedObjectSetBase()
{ erase(); }


template <class T> inline
ManagedObjectSetBase<T>& ManagedObjectSetBase<T>::operator -=( T* ptr )
{
    if ( ptr )
    {
	this->vec_.erase( (void*)ptr );
	delfunc_( ptr );
    }

    return *this;
}


template <class T> inline
T* ManagedObjectSetBase<T>::removeSingle( int idx, bool kporder )
{
    delfunc_( ObjectSet<T>::removeSingle( idx, kporder ) );
    return 0; //Don't give anyone a chance to play with the deleted object
}


template <class T> inline
T* ManagedObjectSetBase<T>::replace( int idx , T* ptr )
{
    delfunc_( ObjectSet<T>::replace( idx, ptr ) );
    return 0; //Don't give anyone a chance to play with the deleted object
}


template <class T> inline
void ManagedObjectSetBase<T>::removeRange( int i1, int i2 )
{
    for ( int idx=(int)i1; idx<=i2; idx++ )
	delfunc_((*this)[idx]);

    ObjectSet<T>::removeRange( i1, i2 );
}


template <class T> inline
void ManagedObjectSetBase<T>::erase()
{
    for ( int idx=ObjectSet<T>::size()-1; idx>=0; idx-- )
	delfunc_((*this)[idx]);

    ObjectSet<T>::erase();
}


template <class T> inline
T* ManagedObjectSetBase<T>::removeAndTake(int idx, bool kporder )
{
    return ObjectSet<T>::removeSingle( idx, kporder );
}


//ManagedObjectSet implementation

template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet()
    : ManagedObjectSetBase<T>(delFunc)
{}


template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( const ObjectSet<T>& t )
    : ManagedObjectSetBase<T>(delFunc)
{ *this = t; }


template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( const ManagedObjectSet<T>& t )
    : ManagedObjectSetBase<T>(delFunc)
{ *this = t; }


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =(const ObjectSet<T>& os)
{ ObjectSet<T>::operator=(os); return *this; }



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
T* RefObjectSet<T>::replace( int idx, T *ptr )
{
    refPtr( ptr );
    return ManagedObjectSetBase<T>::replace(idx, ptr );
}


template <class T> inline
void RefObjectSet<T>::insertAt( T *ptr, int idx )
{
    refPtr( ptr );
    ManagedObjectSetBase<T>::insertAt( ptr, idx );
}


template <class T> inline
ObjectSet<T>& RefObjectSet<T>::doAdd( T *ptr )
{ refPtr( ptr ); return ObjectSet<T>::doAdd(ptr); }
