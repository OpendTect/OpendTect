#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "objectset.h"
#include "refcount.h"

//!Helper class to RefObjectSet and ManagedObjectSet
template <class T>
mClass(Basic) ManagedObjectSetBase : public ObjectSet<T>
{
public:

    typedef typename ObjectSet<T>::size_type	size_type;
    typedef typename ObjectSet<T>::idx_type	idx_type;

    bool		isManaged() const override	{ return true; }

    inline void		erase() override;
    inline T*		pop() override;
			/*!<Deletes entry and returns nullptr */
    inline T*		removeSingle(idx_type,bool kporder=true) override;
			/*!<Removes entry and returns nullptr */
    inline void		removeRange(idx_type,idx_type) override;
    inline T*		replace(idx_type, T*) override;
			/*!<Deletes entry and returns nullptr */
    inline virtual T*	removeAndTake(idx_type,bool kporder=true);
			/*!<Does not delete the entry. */

    inline ManagedObjectSetBase<T>& operator-=(T*) override;

protected:

    typedef void		(*PtrFunc)(T*ptr);
				ManagedObjectSetBase(PtrFunc delfunc)
				    : ObjectSet<T>()
				    , delfunc_( delfunc )
				{}
				~ManagedObjectSetBase();
private:

    PtrFunc		delfunc_;
};


/*!\brief ObjectSet where the objects contained are owned by this set. */

template <class T>
mClass(Basic) ManagedObjectSet : public ManagedObjectSetBase<T>
{
public:

    typedef typename ObjectSet<T>::size_type	size_type;
    typedef typename ObjectSet<T>::idx_type	idx_type;

    inline			ManagedObjectSet();
    ManagedObjectSet*		clone() const override
				{ return new ManagedObjectSet(*this); }

    inline			ManagedObjectSet(const ManagedObjectSet<T>&);
				//Must be implemented as default
				//copy constructor will call
				//operator= before class is fully setup and
				//append is not in virtual table

    inline			ManagedObjectSet(const ObjectSet<T>&);

    ManagedObjectSet<T>&	operator=(const ObjectSet<T>&);

    inline void			append(const ObjectSet<T>&) override;

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

    typedef typename ObjectSet<T>::size_type	size_type;
    typedef typename ObjectSet<T>::idx_type	idx_type;

				RefObjectSet();
				RefObjectSet(const RefObjectSet<T>&);
				RefObjectSet(const ObjectSet<T>&);
    RefObjectSet*		clone() const override
				{ return new RefObjectSet(*this); }

    RefObjectSet<T>&		operator=(const ObjectSet<T>&);
    inline T*			replace(idx_type,T*) override;
    inline void			insertAt(T*,idx_type) override;

protected:

    ObjectSet<T>&		doAdd(T*) override;
    static void			unRef( T* ptr ) { unRefPtr(ptr); }

};


template <class T> inline
ManagedObjectSetBase<T>::~ManagedObjectSetBase()
{ erase(); }


template <class T> inline
ManagedObjectSetBase<T>& ManagedObjectSetBase<T>::operator -=( T* ptr )
{
    if ( ptr )
    {
	this->vec_.erase( (T*)ptr );
	delfunc_( ptr );
    }

    return *this;
}


template <class T> inline
T* ManagedObjectSetBase<T>::removeSingle( idx_type vidx, bool kporder )
{
    delfunc_( ObjectSet<T>::removeSingle( vidx, kporder ) );
    return nullptr; //Don't give anyone a chance to play with the deleted object
}


template <class T> inline
T* ManagedObjectSetBase<T>::replace( idx_type vidx , T* ptr )
{
    delfunc_( ObjectSet<T>::replace( vidx, ptr ) );
    return nullptr; //Don't give anyone a chance to play with the deleted object
}


template <class T> inline
void ManagedObjectSetBase<T>::removeRange( idx_type i1, idx_type i2 )
{
    for ( idx_type vidx=i1; vidx<=i2; vidx++ )
	delfunc_( this->get(vidx) );

    ObjectSet<T>::removeRange( i1, i2 );
}


template <class T> inline
T* ManagedObjectSetBase<T>::pop()
{
    delfunc_( ObjectSet<T>::pop() );
    return nullptr; //Don't give anyone a chance to play with the deleted object
}


template <class T> inline
void ManagedObjectSetBase<T>::erase()
{
    for ( idx_type vidx=ObjectSet<T>::size()-1; vidx>=0; vidx-- )
	delfunc_( this->get(vidx) );

    ObjectSet<T>::erase();
}


template <class T> inline
T* ManagedObjectSetBase<T>::removeAndTake( idx_type vidx, bool kporder )
{
    return ObjectSet<T>::removeSingle( vidx, kporder );
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
    const size_type sz = os.size();
    this->vec_.setCapacity( this->size()+sz, true );
    if ( !os.isManaged() )
	ObjectSet<T>::append( os );
    else
	for ( idx_type vidx=0; vidx<sz; vidx++ )
	{
	    auto* obj = os.get( vidx );
	    ObjectSet<T>::add( obj ? new T(*obj) : nullptr );
	}
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
