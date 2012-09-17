#ifndef ptrman_h
#define ptrman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: ptrman.h,v 1.16 2009/07/22 16:01:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "general.h"


// We have to make 3 macros because of compiler restrictions
// concerning the total length of macros

#define mDefPtrMan1(Clss,PostSet, EraseFunc) \
\
template<class T> \
class Clss \
{ \
public: \
\
				Clss( T* p=0 ) : ptr_( 0 )	{ set(p); } \
				~Clss()		{ erase(); } \
    inline Clss<T>&		operator=( T* p ) \
				{ set(p); return *this; } \
\
    inline T*			ptr()			{ return ptr_; } \
    inline			operator T*()		{ return ptr_; } \
    inline			operator const T*() const { return ptr_; } \
    inline T*			operator ->()		{ return ptr_; } \
    inline const T*		operator ->() const	{ return ptr_; } \
    inline T&			operator *()		{ return *ptr_; } \

#define mDefPtrMan2(Clss,PostSet, EraseFunc) \
    inline bool			operator !() const { return !ptr_; } \
\
    void			erase() \
				{ EraseFunc; ptr_ = 0; } \
    void			set( T* p, bool doerase=true ) \
				{ if ( doerase ) erase(); ptr_=p; PostSet; }

#define mDefPtrMan3(Clss,PostSet, EraseFunc) \
private: \
\
    T*				ptr_; \
\
    Clss<T>&			operator=(const T& p) const; \
\
};


/*!\brief a simple autopointer.

It is assigned to a pointer, and takes over
the responsibility for its deletion.
For Arrays, use the ArrPtrMan class.

*/
mDefPtrMan1(PtrMan, , delete ptr_ )
inline PtrMan<T>& operator=(const PtrMan<T>& p ); //Will give linkerror is used
mDefPtrMan2(PtrMan, , delete ptr_ )
mDefPtrMan3(PtrMan, , delete ptr_ )

/*!\brief a simple autopointer for arrays.

For Non-arrays, use the PtrMan class.

*/
mDefPtrMan1(ArrPtrMan, , delete [] ptr_)
//Will give linkerror is used
inline ArrPtrMan<T>& operator=(const ArrPtrMan<T>& p );
mDefPtrMan2(ArrPtrMan, , delete [] ptr_)
mDefPtrMan3(ArrPtrMan, , delete [] ptr_)


#endif
