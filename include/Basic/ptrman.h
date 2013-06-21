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
#include "errh.h"
#include "atomic.h"

#define mImpPtrManPointerAccess( type ) \
    inline type*		ptr() const		{ return this->ptr_; } \
    inline			operator type*() const	{ return this->ptr_; } \
    inline type*		operator ->() const	{ return this->ptr_; } \
    inline type&		operator *() const	{ return *this->ptr_; }

/*! Base class for smart pointers. Don't use directly, use PtrMan, ArrPtrMan
    or RefMan instead. */

template<class T>
mClass(Basic) PtrManBase
{
public:

    inline bool			operator !() const	{ return !ptr_; }

    inline void			set(T* p, bool doerase=true);
    inline void			erase() { set( 0, true ); }
    inline bool			setIfNull(T* p);

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
			//!<Don't use
    inline		PtrMan(T* = 0);
    PtrMan<T>&		operator=( T* p );
    PtrMan<T>&		operator=(const PtrMan<T>&);
			//!<Don't use
			mImpPtrManPointerAccess( T )
private:

    static void		deleteFunc( T* p )    { delete p; }

};


/*!Smart pointer for normal const pointers. */
template <class T>
mClass(Basic) ConstPtrMan : public PtrManBase<T>
{
public:
    			ConstPtrMan(const ConstPtrMan<T>&);
			//Don't use
    inline		ConstPtrMan(const T* = 0);
    ConstPtrMan<T>&	operator=(const T* p);
    ConstPtrMan<T>&	operator=(const ConstPtrMan<T>&);
			//!<Don't use
			mImpPtrManPointerAccess( const T )
private:

    static void		deleteFunc( T* p )    { delete p; }
};


/*!Smart pointer for pointers allocated as arrays. */
template <class T>
mClass(Basic) ArrPtrMan : public PtrManBase<T>
{
public:
				ArrPtrMan(const ArrPtrMan<T>&);
				//Don't use
    inline			ArrPtrMan(T* = 0);
    ArrPtrMan<T>&		operator=( T* p );
    inline ArrPtrMan<T>&	operator=(const ArrPtrMan<T>& p );
				//!<Don't use
			
				mImpPtrManPointerAccess( T )
private:

    static void		deleteFunc( T* p )    { delete [] p; }

};


/*!Smart pointer for const pointers allocated as arrays. */
template <class T>
mClass(Basic) ConstArrPtrMan : public PtrManBase<T>
{
public:
    			ConstArrPtrMan(const ConstArrPtrMan<T>&);
			//Don't use
    inline		ConstArrPtrMan(const T* = 0);
    ConstArrPtrMan<T>&	operator=(const T* p);
    ConstArrPtrMan<T>&	operator=(const ConstArrPtrMan<T>&);
			//!< Will give linkerror if used
			mImpPtrManPointerAccess( const T )
private:

    static void		deleteFunc( T* p )    { delete p; }
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
			mImpPtrManPointerAccess( T )

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
    ConstRefMan<T>&		operator=(const T* p);
    inline ConstRefMan<T>&	operator=(const ConstRefMan<T>&);

				mImpPtrManPointerAccess( const T )

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
    this->set(p);
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
bool PtrManBase<T>::setIfNull( T* p )
{
    if ( ptr_.setIfEqual( p, 0 ) )
    {
	if ( setfunc_ && p )
	    setfunc_(p);
	return true;
    }

    return false;
}


template <class T> inline
PtrMan<T>::PtrMan( const PtrMan<T>& )
    : PtrManBase<T>( 0, deleteFunc, 0 )
{
    pErrMsg("Should not be called");
}


template <class T> inline
PtrMan<T>& PtrMan<T>::operator=( const PtrMan<T>& )
{
    PtrManBase<T>::set( 0, true );
    pErrMsg("Should not be called");
    return *this;
}


template <class T> inline
PtrMan<T>::PtrMan( T* p )
    : PtrManBase<T>( 0, deleteFunc, p )
{}


template <class T> inline
PtrMan<T>& PtrMan<T>::operator=( T* p )
{
    this->set( p );
    return *this;
}


template <class T> inline
ConstPtrMan<T>::ConstPtrMan( const ConstPtrMan<T>& )
    : PtrManBase<T>( 0, deleteFunc, 0 )
{
    pErrMsg("Should not be called");
}


template <class T> inline
ConstPtrMan<T>& ConstPtrMan<T>::operator=( const ConstPtrMan<T>& )
{
    PtrManBase<T>::set( 0, true );
    pErrMsg("Should not be called");
    return *this;
}


template <class T> inline
ConstPtrMan<T>::ConstPtrMan( const T* p )
    : PtrManBase<T>( 0, deleteFunc, const_cast<T*>(p) )
{}


template <class T> inline
ConstPtrMan<T>& ConstPtrMan<T>::operator=( const T* p )
{
    this->set( const_cast<T*>( p ) );
    return *this;
}


template <class T> inline
ArrPtrMan<T>::ArrPtrMan( const ArrPtrMan<T>& )
    : PtrManBase<T>( 0, deleteFunc, 0 )
{
    pErrMsg("Should not be called");
}


template <class T> inline
ArrPtrMan<T>& ArrPtrMan<T>::operator=( const ArrPtrMan<T>& )
{
    PtrManBase<T>::set( 0, true );
    pErrMsg("Should not be called");
    return *this;
}


template <class T> inline
ArrPtrMan<T>::ArrPtrMan( T* p )
    : PtrManBase<T>( 0, deleteFunc, p )
{}


template <class T> inline
ArrPtrMan<T>& ArrPtrMan<T>::operator=( T* p )
{
    this->set( p );
    return *this;
}


template <class T> inline
ConstArrPtrMan<T>::ConstArrPtrMan( const ConstArrPtrMan<T>& p )
    : PtrManBase<T>( 0, deleteFunc, 0 )
{
    pErrMsg("Shold not be called");
}


template <class T> inline
ConstArrPtrMan<T>::ConstArrPtrMan( const T* p )
    : PtrManBase<T>( 0, deleteFunc, const_cast<T*>(p) )
{}


template <class T> inline
ConstArrPtrMan<T>& ConstArrPtrMan<T>::operator=( const T* p )
{
    this->set( const_cast<T*>(p) );
    return *this;
}


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
    this->set( const_cast<T*>(p.ptr()) );
    return *this;
}


template <class T> inline
ConstRefMan<T>&	ConstRefMan<T>::operator=(const T* p)
{
    this->set( const_cast<T*>( p ) );
    return *this;
}

#endif
