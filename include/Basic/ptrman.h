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


/*! Base class for smart pointers. Don't use directly, use PtrMan, ArrPtrMan
    or RefMan instead. */

template<class T>
mClass(Basic) PtrManBase
{
public:

    inline T*			ptr() const		{ return ptr_; }
    inline			operator T*()		{ return ptr_; }
    inline			operator const T*() const { return ptr_; }
    inline T*			operator ->()		{ return ptr_; }
    inline const T*		operator ->() const	{ return ptr_; }
    inline T&			operator *()		{ return *ptr_; }
    inline bool			operator !() const	{ return !ptr_; }

    inline void			set(T* p, bool doerase=true);
    inline void			erase() { set( 0, true ); }

protected:

    typedef void		(*PtrFunc)(T*);
    inline			PtrManBase(PtrFunc setfunc,PtrFunc deletor,T*);
    virtual			~PtrManBase()		{ set(0,true); }
    
private:

    Threads::AtomicPointer<T>	ptr_;

    PtrFunc			setfunc_;
    PtrFunc			deletefunc_;

};


/*!Smart pointer for normal pointers. */
template <class T>
mClass(Basic) PtrMan : public PtrManBase<T>
{
public:

    inline		PtrMan(T* = 0);
    PtrMan<T>&		operator=( T* p )
    			{ this->set( p, true ); return *this; }
    PtrMan<T>&		operator=(const PtrMan<T>&);
			//!< Will give linkerror if used
private:

    static void		deleteFunc( T* p )    { delete p; }

};


/*!Smart pointer for pointers allocated as arrays. */
template <class T>
mClass(Basic) ArrPtrMan : public PtrManBase<T>
{
public:

    inline		ArrPtrMan(T* = 0);
    inline ArrPtrMan<T>& operator=( T* p )
    			{ this->set( p, true ); return *this; }
    inline ArrPtrMan<T>& operator=(const ArrPtrMan<T>&);
			//!< Will give linkerror if used
			
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


#endif
