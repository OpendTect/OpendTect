#ifndef manobjectset_h
#define manobjectset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "objectset.h"


/*!
\brief ObjectSet where the objects contained are owned by this set.
*/

template <class T>
mClass(Basic) ManagedObjectSet : public ObjectSet<T>
{
public:

    inline 			ManagedObjectSet(bool objs_are_arrs);
    inline			ManagedObjectSet(const ManagedObjectSet<T>&);
    inline virtual		~ManagedObjectSet();
    inline ManagedObjectSet<T>&	operator =(const ObjectSet<T>&);
    inline ManagedObjectSet<T>&	operator =(const ManagedObjectSet<T>&);
    virtual bool		isManaged() const	{ return true; }

    inline virtual ManagedObjectSet<T>& operator -=( T* ptr );

    inline virtual void		append(const ObjectSet<T>&);
    inline virtual void		erase();
    inline virtual void		removeRange(od_int64,od_int64);
    inline virtual T*		removeSingle( int idx, bool kporder=true );
				/*!<Deletes entry and returns 0 */
    inline virtual T*		removeAndTake(int idx, bool kporder=true );
				/*!<Does not delete the entry. */

    inline void			setEmpty()		{ erase(); }

protected:

    bool	isarr_;

};


//ObjectSet implementation
template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( bool isarr ) : isarr_(isarr)
{}


template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( const ManagedObjectSet<T>& t )
{ *this = t; }


template <class T> inline
ManagedObjectSet<T>::~ManagedObjectSet()
{ erase(); }


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =( const ObjectSet<T>& os )
{
    if ( &os != this ) deepCopy( *this, os );
    return *this;
}

template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =(
					const ManagedObjectSet<T>& os )
{
    if ( &os != this ) { isarr_ = os.isarr_; deepCopy( *this, os ); }
    return *this;
}


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator -=( T* ptr )
{
    if ( !ptr ) return *this;
    this->vec_.erase( (void*)ptr );
    if ( isarr_ )	delete [] ptr;
    else		delete ptr;
    return *this;
}


template <class T> inline
void ManagedObjectSet<T>::append( const ObjectSet<T>& os )
{
    const int sz = os.size();
    this->vec_.setCapacity( this->size()+sz );
    if ( !os.isManaged() )
	ObjectSet<T>::append( os );
    else
	for ( int idx=0; idx<sz; idx++ )
	    *this += new T( *os[idx] );
}


template <class T> inline
void ManagedObjectSet<T>::erase()
{
    if ( isarr_ )	deepEraseArr( *this );
    else		deepErase( *this );
}


template <class T> inline
T* ManagedObjectSet<T>::removeSingle( int idx, bool kporder )
{
    if ( isarr_ )
	delete [] (*this)[idx];
    else
	delete (*this)[idx];

    ObjectSet<T>::removeSingle( idx, kporder );
    return 0; //Don't give anyone a chance to play with the deleted object
}


template <class T> inline
void ManagedObjectSet<T>::removeRange( od_int64 i1, od_int64 i2 )
{
    for ( int idx=(int)i1; idx<=i2; idx++ )
    {
	if ( isarr_ )
	    delete [] (*this)[idx];
	else
	    delete (*this)[idx];
    }
    ObjectSet<T>::removeRange( i1, i2 );
}

template <class T> inline
T* ManagedObjectSet<T>::removeAndTake(int idx, bool kporder )
{
    return ObjectSet<T>::removeSingle( idx, kporder );
}


#endif
