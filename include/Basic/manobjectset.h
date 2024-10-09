#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    inline			ManagedObjectSet(const ObjectSet<T>&);
				mOD_DisableCopy(ManagedObjectSet)

    inline ManagedObjectSet<T>&	operator=(const ObjectSet<T>& os);
				// takes over ownership of objects in os
				// and erases os

    inline void			append(const ObjectSet<T>& os) override;
				// takes over ownership of objects in os
				// and erases os
    void			append(const ManagedObjectSet<T>&) = delete;
				// use deepAppend
    inline void			copy(const ObjectSet<T>& os) override;
				// takes over ownership of objects in os
				// and erases os

private:

    static void			delFunc(T* ptr) { delete ptr; }
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
{
    *this = t;
}


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator=( const ObjectSet<T>& os )
{
    copy( os );
    return *this;
}


template <class T> inline
void ManagedObjectSet<T>::append( const ObjectSet<T>& os )
{
    const size_type sz = os.size();
    this->vec_.setCapacity( this->size()+sz, true );
    ObjectSet<T>::append( os );
    cCast(ObjectSet<T>&,os).erase();
}


template <class T> inline
void ManagedObjectSet<T>::copy( const ObjectSet<T>& os )
{
    ObjectSet<T>::setEmpty();
    append( os );
}


template <class T>
inline void deepCopy( ManagedObjectSet<T>& to,
		      const ManagedObjectSet<T>& from )
{
    if ( &to == &from )
	return;

    to.setEmpty();
    deepAppend( to, from );
}


template <class T>
inline void deepAppend( ManagedObjectSet<T>& to,
			const ManagedObjectSet<T>& from )
{
    if ( &to == &from )
	return;

    for ( const auto* obj : from )
    {
	if ( obj )
	    to.add( new T(*obj) );
	else if ( to.nullAllowed() )
	    to.add( nullptr );
    }
}
