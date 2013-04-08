#ifndef ptrman_h
#define ptrman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "general.h"
#include "thread.h"

#define mImpPtrManPointerAccess( constvar ) \
    inline constvar T*		ptr() const		{ return this->ptr_; } \
    inline			operator constvar T*() const	{ return this->ptr_; } \
    inline constvar T*		operator ->() const	{ return this->ptr_; } \
    inline constvar T&		operator *() const	{ return *this->ptr_; }

/*! Base class for smart pointers. Don't use directly, use PtrMan, ArrPtrMan
    or RefMan instead. */

template<class T>
mClass(Basic) PtrManBase
{
public:

    inline bool			operator !() const	{ return !ptr_; }

    inline void			set(T* p, bool doerase=true);
    inline void			erase() { set( 0, true ); }

protected:

    typedef void		(*PtrFunc)(T*);
    inline			PtrManBase(PtrFunc setfunc,PtrFunc deletor,T*);
    virtual			~PtrManBase()		{ set(0,true); }
    

    Threads::AtomicPointer<T>	ptr_;

    PtrFunc			setfunc_;
    PtrFunc			deletefunc_;

};


/*!Smart pointer for normal pointers. */
template <class T>
mClass(Basic) PtrMan : public PtrManBase<T>
{
public:
    			PtrMan(const PtrMan<T>&);
			//Will give linkerror if used
    inline		PtrMan(T* = 0);
    PtrMan<T>&		operator=( T* p )
    			{ this->set( p, true ); return *this; }
    PtrMan<T>&		operator=(const PtrMan<T>&);
			//!< Will give linkerror if used
			mImpPtrManPointerAccess( )
private:

    static void		deleteFunc( T* p )    { delete p; }

};


/*!Smart pointer for pointers allocated as arrays. */
template <class T>
mClass(Basic) ArrPtrMan : public PtrManBase<T>
{
public:
				ArrPtrMan(const ArrPtrMan<T>&);
				//Will give linkerror if used
    inline			ArrPtrMan(T* = 0);
    ArrPtrMan<T>&		operator=( T* p )  { set( p ); return *this; }
    inline ArrPtrMan<T>&	operator=(const ArrPtrMan<T>& p );
				//Will give linkerror if used
			
				mImpPtrManPointerAccess( )
private:

    static void		deleteFunc( T* p )    { delete [] p; }

};


/*!Smart pointer for reference counted objects. */
template <class T>
mClass(Basic) RefMan : public PtrManBase<T>
{
public:

    inline		RefMan(const RefMan<T>&);
    inline		RefMan(T* = 0);
    inline RefMan<T>&	operator=( T* p )
    			{ this->set( p, true ); return *this; }
    inline RefMan<T>&	operator=(const RefMan<T>&);
			mImpPtrManPointerAccess( )

private:

    static void		ref(T* p) { p->ref(); }
    static void		unRef(T* p) { if ( p ) p->unRef(); }
    
};


/*!Smart pointer for reference counted objects. */
template <class T>
mClass(Basic) ConstRefMan : public PtrManBase<T>
{
public:
    inline			ConstRefMan(const ConstRefMan<T>&);
    inline			ConstRefMan(const T* = 0);
    ConstRefMan<T>&		operator=(const T* p) { set( p ); return *this; }
    inline ConstRefMan<T>&	operator=(const ConstRefMan<T>&);

				mImpPtrManPointerAccess( const )

private:
    static void		ref(T* p) { p->ref(); }
    static void		unRef(T* p) { if ( p ) p->unRef(); }
    
};

//Implementations below

template <class T> inline
PtrManBase<T>::PtrManBase( PtrFunc setfunc, PtrFunc deletor, T* p )
    : deletefunc_( deletor )
    , setfunc_( setfunc )
{
    set(p);
}


template <class T> inline
void PtrManBase<T>::set( T* p, bool doerase )
{
    if ( setfunc_ && p )
	setfunc_(p);
    
    T* oldptr = ptr_.exchange(p);
    if ( doerase )
	deletefunc_( oldptr );
}


template <class T> inline
PtrMan<T>::PtrMan( T* p )
    : PtrManBase<T>( 0, deleteFunc, p )
{}


template <class T> inline
ArrPtrMan<T>::ArrPtrMan( T* p )
    : PtrManBase<T>( 0, deleteFunc, p )
{}


template <class T> inline
RefMan<T>::RefMan( const RefMan<T>& p )
    : PtrManBase<T>( ref, unRef, p.ptr() )
{}

    
template <class T> inline
RefMan<T>::RefMan( T* p )
    : PtrManBase<T>( ref, unRef, p )
{}


template <class T> inline
RefMan<T>& RefMan<T>::operator=( const RefMan<T>& p )
{
    this->set( p.ptr() );
    return *this;
}


template <class T> inline
ConstRefMan<T>::ConstRefMan( const ConstRefMan<T>& p )
    : PtrManBase<T>( ref, unRef, const_cast<T*>(p.ptr()) )
{}

    
template <class T> inline
ConstRefMan<T>::ConstRefMan( const T* p )
    : PtrManBase<T>( ref, unRef, const_cast<T*>(p) )
{}


template <class T> inline
ConstRefMan<T>& ConstRefMan<T>::operator=( const ConstRefMan<T>& p )
{
    set( const_cast<T*>(p.ptr()) );
    return *this;
}

#endif
